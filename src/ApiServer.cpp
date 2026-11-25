#include "ApiServer.hpp"
#include "JsonSerializer.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

ApiServer::ApiServer(TopologyGraph& graph, DatabaseManager& db, std::mutex& mtx)
    : graph_(graph), db_(db), mtx_(mtx) {}

ApiServer::~ApiServer() {
    stop();
}

bool ApiServer::checkToken(const httplib::Request& req) {
    if (!req.has_header("Authorization")) {
        return false;
    }
    std::string auth = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    if (auth.rfind(prefix, 0) != 0) {
        return false;
    }
    std::string token = auth.substr(prefix.length());

    std::lock_guard<std::mutex> lock(mtx_);
    return db_.isTokenValid(token);
}

void ApiServer::start(int port) {
    // 1. Получить всю топологию
    svr_.Get("/api/v1/topology", [this](const httplib::Request& req, httplib::Response& res) {
        if (!checkToken(req)) {
            res.status = 401;
            res.set_content("{\"error\": \"Unauthorized: invalid or missing API token\"}", "application/json");
            return;
        }
        std::lock_guard<std::mutex> lock(mtx_);
        std::string body = JsonSerializer::exportToJsonString(graph_);
        res.set_content(body, "application/json");
    });

    // 2. Получить входящие и исходящие взаимодействия сервера по IP
    svr_.Get(R"(/api/v1/nodes/([0-9\.]+))", [this](const httplib::Request& req, httplib::Response& res) {
        if (!checkToken(req)) {
            res.status = 401;
            res.set_content("{\"error\": \"Unauthorized\"}", "application/json");
            return;
        }
        std::string ip = req.matches[1];
        std::lock_guard<std::mutex> lock(mtx_);

        auto item_opt = graph_.getItemByIp(ip);
        if (!item_opt.has_value()) {
            res.status = 404;
            res.set_content("{\"error\": \"Server not found\"}", "application/json");
            return;
        }

        json out;
        out["server"] = {
            {"id", item_opt->id},
            {"name", item_opt->name},
            {"ip_address", item_opt->ip_address},
            {"port", item_opt->port},
            {"status", item_opt->status}
        };

        out["inbound"] = json::array();
        auto in_deps = graph_.getInboundDependencies(item_opt->id);
        for (size_t i = 0; i < in_deps.size(); ++i) {
            auto src = graph_.getItemById(in_deps[i].source_id);
            if (src.has_value()) {
                out["inbound"].push_back({
                    {"from_name", src->name},
                    {"from_ip", src->ip_address},
                    {"protocol", in_deps[i].protocol},
                    {"is_critical", in_deps[i].is_critical}
                });
            }
        }

        out["outbound"] = json::array();
        auto out_deps = graph_.getOutboundDependencies(item_opt->id);
        for (size_t i = 0; i < out_deps.size(); ++i) {
            auto tgt = graph_.getItemById(out_deps[i].target_id);
            if (tgt.has_value()) {
                out["outbound"].push_back({
                    {"to_name", tgt->name},
                    {"to_ip", tgt->ip_address},
                    {"protocol", out_deps[i].protocol},
                    {"is_critical", out_deps[i].is_critical}
                });
            }
        }

        res.set_content(out.dump(2), "application/json");
    });

    // 3. Оценить зону риска при падении сервера по IP
    svr_.Get(R"(/api/v1/impact/([0-9\.]+))", [this](const httplib::Request& req, httplib::Response& res) {
        if (!checkToken(req)) {
            res.status = 401;
            res.set_content("{\"error\": \"Unauthorized\"}", "application/json");
            return;
        }
        std::string ip = req.matches[1];
        std::lock_guard<std::mutex> lock(mtx_);

        auto item_opt = graph_.getItemByIp(ip);
        if (!item_opt.has_value()) {
            res.status = 404;
            res.set_content("{\"error\": \"Server not found\"}", "application/json");
            return;
        }

        auto impacted = graph_.analyzeImpact(item_opt->id);
        json out;
        out["failed_server"] = item_opt->name;
        out["failed_ip"] = ip;
        out["affected_count"] = impacted.size();
        out["affected_servers"] = json::array();

        for (size_t i = 0; i < impacted.size(); ++i) {
            out["affected_servers"].push_back({
                {"name", impacted[i].item.name},
                {"ip_address", impacted[i].item.ip_address},
                {"distance", impacted[i].distance},
                {"is_critical_impact", impacted[i].is_critical_impact}
            });
        }

        res.set_content(out.dump(2), "application/json");
    });

    // 4. Импорт JSON через API
    svr_.Post("/api/v1/import", [this](const httplib::Request& req, httplib::Response& res) {
        if (!checkToken(req)) {
            res.status = 401;
            res.set_content("{\"error\": \"Unauthorized\"}", "application/json");
            return;
        }
        std::lock_guard<std::mutex> lock(mtx_);
        if (JsonSerializer::importFromJsonString(req.body, db_, graph_)) {
            res.set_content("{\"status\": \"ok\", \"message\": \"Topology imported\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\": \"Invalid JSON format\"}", "application/json");
        }
    });

    is_running_ = true;
    server_thread_ = std::thread([this, port]() {
        svr_.listen("0.0.0.0", port);
    });
}

void ApiServer::stop() {
    if (is_running_) {
        svr_.stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        is_running_ = false;
    }
}
