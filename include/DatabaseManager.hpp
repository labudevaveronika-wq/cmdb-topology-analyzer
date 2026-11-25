#pragma once

#include "Models.hpp"
#include "TopologyGraph.hpp"
#include <pqxx/pqxx>
#include <string>

class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& conn_str);

    void loadAllToGraph(TopologyGraph& graph);

    void clearAllData();

    int insertItem(const ConfigurationItem& item);
    bool updateItemStatus(int id, const std::string& new_status);
    bool updateItemIp(int id, const std::string& new_ip);
    bool deleteItem(int id);

    int insertDependency(const Dependency& dep);
    bool deleteDependency(int source_id, int target_id);

    bool isTokenValid(const std::string& token);

private:
    pqxx::connection conn_;
};
