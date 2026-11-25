#include "TopologyGraph.hpp"
#include "GraphVisualizer.hpp"
#include <cassert>
#include <iostream>

int main() {
    TopologyGraph graph;

    graph.addItem({1, "Frontend-Server", "Server", "10.0.0.1", 443, "ACTIVE"});
    graph.addItem({2, "Backend-Server",  "Server", "10.0.0.2", 8080, "ACTIVE"});
    graph.addItem({3, "DB-Server",       "Server", "10.0.0.3", 5432, "ACTIVE"});
    graph.addItem({4, "Backup-Server",   "Server", "10.0.0.4", 22, "ACTIVE"});

    // Frontend -> Backend -> DB
    graph.addDependency({1, 1, 2, "HTTPS", true});
    graph.addDependency({2, 2, 3, "TCP:5432", true});

    // Тест 1: Поиск по IP
    auto node = graph.getItemByIp("10.0.0.3");
    assert(node.has_value() && node->name == "DB-Server");

    // Тест 2: Поиск одинокого сервера
    auto isolated = graph.getIsolatedItems();
    assert(isolated.size() == 1 && isolated[0].name == "Backup-Server");

    // Тест 3: Расчет зоны риска при падении DB-Server (id = 3)
    auto impact = graph.analyzeImpact(3);
    assert(impact.size() == 2);
    assert(impact[0].item.name == "Backend-Server" && impact[0].distance == 1 && impact[0].is_critical_impact);
    assert(impact[1].item.name == "Frontend-Server" && impact[1].distance == 2 && impact[1].is_critical_impact);

    // Тест 4: Отрисовка схемы в картинку
    bool rendered = GraphVisualizer::renderToPng(graph, "test_topology.png", 3);
    assert(rendered);

    std::cout << "[OK] All tests passed and test_topology.png generated!\n";
    return 0;
}
