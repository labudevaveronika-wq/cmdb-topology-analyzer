#include "JsonSerializer.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>

using json = nlohmann::json;

bool JsonSerializer::importFromJsonString(const std::string& json_text,
                                          DatabaseManager& db,
                                          TopologyGraph& graph) {
    try {
        json j = json::parse(json_text);

        if (!j.contains("items") || !j["items"].is_array()) {
            return false;
        }

        db.clearAllData();
        std::unordered_map<std::string, int> ip_to_id;

        for (size_t i = 0; i < j["items"].size(); ++i) {
            const auto& obj = j["items"][i];
            ConfigurationItem item;
            item.name       = obj.value("name", "Unknown-Server");
            item.ci_type    = obj.value("ci_type", "Server");
            item.ip_address = obj.at("ip_address").get<std::string>();
            item.port       = obj.value("port", 0);
            item.status     = obj.value("status", "ACTIVE");

            int new_id = db.insertItem(item);
            ip_to_id[item.ip_address] = new_id;
        }

        if (j.contains("dependencies") && j["dependencies"].is_array()) {
            for (size_t i = 0; i < j["dependencies"].size(); ++i) {
                const auto& dep_obj = j["dependencies"][i];
                std::string src_ip = dep_obj.at("source_ip").get<std::string>();
                std::string tgt_ip = dep_obj.at("target_ip").get<std::string>();

                if (ip_to_id.find(src_ip) != ip_to_id.end() &&
                    ip_to_id.find(tgt_ip) != ip_to_id.end()) {
                    Dependency dep;
                    dep.source_id   = ip_to_id[src_ip];
                    dep.target_id   = ip_to_id[tgt_ip];
                    dep.protocol    = dep_obj.value("protocol", "TCP");
                    dep.is_critical = dep_obj.value("is_critical", true);

                    db.insertDependency(dep);
                }
            }
        }

        db.loadAllToGraph(graph);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Ошибка JSON] " << e.what() << "\n";
        return false;
    }
}

bool JsonSerializer::importFromFile(const std::string& file_path,
                                    DatabaseManager& db,
                                    TopologyGraph& graph) {
    std::ifstream in(file_path);
    if (!in.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    return importFromJsonString(buffer.str(), db, graph);
}

std::string JsonSerializer::exportToJsonString(const TopologyGraph& graph) {
    json j;
    j["items"] = json::array();
    j["dependencies"] = json::array();

    std::vector<ConfigurationItem> items = graph.getAllItems();
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        j["items"].push_back({
            {"id", item.id},
            {"name", item.name},
            {"ci_type", item.ci_type},
            {"ip_address", item.ip_address},
            {"port", item.port},
            {"status", item.status}
        });

        std::vector<Dependency> deps = graph.getOutboundDependencies(item.id);
        for (size_t k = 0; k < deps.size(); ++k) {
            auto target_opt = graph.getItemById(deps[k].target_id);
            if (target_opt.has_value()) {
                j["dependencies"].push_back({
                    {"source_ip", item.ip_address},
                    {"target_ip", target_opt->ip_address},
                    {"protocol", deps[k].protocol},
                    {"is_critical", deps[k].is_critical}
                });
            }
        }
    }

    return j.dump(2); 
}

bool JsonSerializer::exportToFile(const std::string& file_path,
                                  const TopologyGraph& graph) {
    std::ofstream out(file_path);
    if (!out.is_open()) {
        return false;
    }
    out << exportToJsonString(graph);
    return true;
}
