#include "DatabaseManager.hpp"
#include <iostream>

DatabaseManager::DatabaseManager(const std::string& conn_str)
    : conn_(conn_str) {
    if (!conn_.is_open()) {
        throw std::runtime_error("Не удалось подключиться к PostgreSQL!");
    }
}

void DatabaseManager::loadAllToGraph(TopologyGraph& graph) {
    graph.clear();
    pqxx::work txn(conn_);

    pqxx::result items_res = txn.exec(
        "SELECT id, name, ci_type, ip_address, port, status FROM configuration_items ORDER BY id;"
    );

    for (size_t i = 0; i < items_res.size(); ++i) {
        ConfigurationItem item;
        item.id         = items_res[i]["id"].as<int>();
        item.name       = items_res[i]["name"].as<std::string>();
        item.ci_type    = items_res[i]["ci_type"].as<std::string>();
        item.ip_address = items_res[i]["ip_address"].as<std::string>();
        item.port       = items_res[i]["port"].as<int>();
        item.status     = items_res[i]["status"].as<std::string>();

        graph.addItem(item);
    }

    // 2. Читаем все связи из таблицы dependencies
    pqxx::result deps_res = txn.exec(
        "SELECT id, source_id, target_id, protocol, is_critical FROM dependencies ORDER BY id;"
    );

    for (size_t i = 0; i < deps_res.size(); ++i) {
        Dependency dep;
        dep.id          = deps_res[i]["id"].as<int>();
        dep.source_id   = deps_res[i]["source_id"].as<int>();
        dep.target_id   = deps_res[i]["target_id"].as<int>();
        dep.protocol    = deps_res[i]["protocol"].as<std::string>();
        dep.is_critical = deps_res[i]["is_critical"].as<bool>();

        graph.addDependency(dep);
    }

    txn.commit();
}

void DatabaseManager::clearAllData() {
    pqxx::work txn(conn_);
    txn.exec("TRUNCATE TABLE dependencies, configuration_items RESTART IDENTITY CASCADE;");
    txn.commit();
}

int DatabaseManager::insertItem(const ConfigurationItem& item) {
    pqxx::work txn(conn_);

    std::string sql =
        "INSERT INTO configuration_items (name, ci_type, ip_address, port, status) VALUES (" +
        txn.quote(item.name) + ", " +
        txn.quote(item.ci_type) + ", " +
        txn.quote(item.ip_address) + ", " +
        std::to_string(item.port) + ", " +
        txn.quote(item.status) +
        ") RETURNING id;";

    pqxx::result res = txn.exec(sql);
    txn.commit();

    return res[0][0].as<int>(); // Возвращаем ID, который присвоила база
}

bool DatabaseManager::updateItemStatus(int id, const std::string& new_status) {
    pqxx::work txn(conn_);
    std::string sql =
        "UPDATE configuration_items SET status = " + txn.quote(new_status) +
        " WHERE id = " + std::to_string(id) + ";";

    pqxx::result res = txn.exec(sql);
    txn.commit();

    return res.affected_rows() > 0;
}

bool DatabaseManager::updateItemIp(int id, const std::string& new_ip) {
    pqxx::work txn(conn_);
    std::string sql =
        "UPDATE configuration_items SET ip_address = " + txn.quote(new_ip) +
        " WHERE id = " + std::to_string(id) + ";";

    pqxx::result res = txn.exec(sql);
    txn.commit();

    return res.affected_rows() > 0;
}

bool DatabaseManager::deleteItem(int id) {
    pqxx::work txn(conn_);
    std::string sql =
        "DELETE FROM configuration_items WHERE id = " + std::to_string(id) + ";";

    pqxx::result res = txn.exec(sql);
    txn.commit();

    return res.affected_rows() > 0;
}

int DatabaseManager::insertDependency(const Dependency& dep) {
    pqxx::work txn(conn_);
    std::string crit_str = dep.is_critical ? "TRUE" : "FALSE";

    std::string sql =
        "INSERT INTO dependencies (source_id, target_id, protocol, is_critical) VALUES (" +
        std::to_string(dep.source_id) + ", " +
        std::to_string(dep.target_id) + ", " +
        txn.quote(dep.protocol) + ", " +
        crit_str +
        ") RETURNING id;";

    pqxx::result res = txn.exec(sql);
    txn.commit();

    return res[0][0].as<int>();
}

bool DatabaseManager::deleteDependency(int source_id, int target_id) {
    pqxx::work txn(conn_);
    std::string sql =
        "DELETE FROM dependencies WHERE source_id = " + std::to_string(source_id) +
        " AND target_id = " + std::to_string(target_id) + ";";

    pqxx::result res = txn.exec(sql);
    txn.commit();

    return res.affected_rows() > 0;
}

bool DatabaseManager::isTokenValid(const std::string& token) {
    pqxx::work txn(conn_);
    std::string sql =
        "SELECT id FROM api_tokens WHERE token = " + txn.quote(token) +
        " AND is_active = TRUE;";

    pqxx::result res = txn.exec(sql);
    txn.commit();

    return !res.empty();
}
