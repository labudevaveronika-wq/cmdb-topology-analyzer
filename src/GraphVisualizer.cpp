#include "GraphVisualizer.hpp"
#include <fstream>
#include <cstdlib>
#include <unordered_set>

bool GraphVisualizer::renderToPng(const TopologyGraph& graph,
                                  const std::string& output_png,
                                  int failed_id) {
    const std::string dot_file = "topology.dot";
    std::ofstream out(dot_file);
    if (!out.is_open()) {
        return false;
    }

    std::unordered_set<int> impacted_ids;
    if (failed_id != -1) {
        std::vector<ImpactedNode> impacted = graph.analyzeImpact(failed_id);
        for (size_t i = 0; i < impacted.size(); ++i) {
            impacted_ids.insert(impacted[i].item.id);
        }
    }

    out << "digraph CMDBTopology {\n";
    out << "  rankdir=LR;\n";
    out << "  node [shape=box, style=\"rounded,filled\", fontname=\"Arial\", fontsize=10];\n";
    out << "  edge [fontname=\"Arial\", fontsize=9];\n\n";

    std::vector<ConfigurationItem> items = graph.getAllItems();
    for (size_t i = 0; i < items.size(); ++i) {
        const ConfigurationItem& item = items[i];
        std::string fill_color = "#d4edda";
        std::string border_style = "rounded,filled";

        bool no_in = graph.getInboundDependencies(item.id).empty();
        bool no_out = graph.getOutboundDependencies(item.id).empty();

        if (item.id == failed_id || item.status == "DOWN") {
            fill_color = "#f8d7da";
        } else if (impacted_ids.find(item.id) != impacted_ids.end()) {
            fill_color = "#fff3cd";
        } else if (no_in && no_out) {
            fill_color = "#e2e3e5";
            border_style = "dashed,filled";
        }

        out << "  node_" << item.id << " ["
            << "label=\"" << item.name << "\\n(" << item.ci_type << ")\\n"
            << item.ip_address << ":" << item.port << "\\n[" << item.status << "]\", "
            << "fillcolor=\"" << fill_color << "\", "
            << "style=\"" << border_style << "\""
            << "];\n";
    }

    out << "\n";

    for (size_t i = 0; i < items.size(); ++i) {
        std::vector<Dependency> deps = graph.getOutboundDependencies(items[i].id);
        for (size_t j = 0; j < deps.size(); ++j) {
            std::string color = deps[j].is_critical ? "#dc3545" : "#6c757d";
            std::string style = deps[j].is_critical ? "solid" : "dashed";
            std::string crit_text = deps[j].is_critical ? " (CRIT)" : "";

            out << "  node_" << deps[j].source_id << " -> node_" << deps[j].target_id
                << " [label=\"" << deps[j].protocol << crit_text << "\", "
                << "color=\"" << color << "\", style=\"" << style << "\"];\n";
        }
    }

    out << "}\n";
    out.close();

    std::string cmd = "dot -Tpng " + dot_file + " -o " + output_png;
    int result = std::system(cmd.c_str());
    return result == 0;
}
