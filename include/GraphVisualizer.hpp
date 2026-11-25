#pragma once

#include "TopologyGraph.hpp"
#include <string>

class GraphVisualizer {
public:
    static bool renderToPng(const TopologyGraph& graph,
                            const std::string& output_png = "topology.png",
                            int failed_id = -1);
};
