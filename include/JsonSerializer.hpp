#pragma once

#include "TopologyGraph.hpp"
#include "DatabaseManager.hpp"
#include <string>

class JsonSerializer {
public:
    static bool importFromJsonString(const std::string& json_text,
                                     DatabaseManager& db,
                                     TopologyGraph& graph);

    static bool importFromFile(const std::string& file_path,
                               DatabaseManager& db,
                               TopologyGraph& graph);

    static std::string exportToJsonString(const TopologyGraph& graph);

    static bool exportToFile(const std::string& file_path,
                             const TopologyGraph& graph);
};
