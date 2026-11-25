#include "Models.hpp"
#include "TopologyGraph.hpp"
#include "DatabaseManager.hpp"
#include "JsonSerializer.hpp"
#include "GraphVisualizer.hpp"
#include "ApiServer.hpp"

#include <iostream>
#include <iomanip>
#include <mutex>
#include <limits>

// Вспомогательная функция для безопасного ввода чисел с клавиатуры
int readInt() {
    int val;
    while (!(std::cin >> val)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Ошибка ввода. Введите число: ";
    }
    return val;
}

void printMenu() {
    std::cout << "\n=============================================\n";
    std::cout << "   CMDB & Topology Impact Analyzer (v0.1)\n";
    std::cout << "=============================================\n";
    std::cout << "1. Показать все серверы\n";
    std::cout << "2. Проверить связи сервера (входящие / исходящие)\n";
    std::cout << "3. Показать изолированные серверы (без связей)\n";
    std::cout << "4. Добавить сервер\n";
    std::cout << "5. Добавить связь между серверами\n";
    std::cout << "6. Изменить статус или IP сервера\n";
    std::cout << "7. Удалить сервер или связь\n";
    std::cout << "8. Импорт / Экспорт JSON\n";
    std::cout << "9. Оценить зону риска и отрисовать схему (PNG)\n";
    std::cout << "0. Выход\n";
    std::cout << "Выберите действие: ";
}

int main() {
    const std::string conn_str =
        "host=127.0.0.1 port=5434 dbname=cmdb_db user=cmdb_user password=cmdb_pass";

    try {
        DatabaseManager db(conn_str);
        TopologyGraph graph;
        std::mutex mtx;

        // Загружаем актуальное состояние из PostgreSQL в граф
        db.loadAllToGraph(graph);

        // Запускаем фоновый REST API на порту 8080
        ApiServer api(graph, db, mtx);
        api.start(8080);
        std::cout << "[INFO] Подключено к PostgreSQL (порт 5434).\n";
        std::cout << "[INFO] Фоновый REST API запущен на http://localhost:8080\n";

        while (true) {
            printMenu();
            int choice = readInt();

            if (choice == 0) {
                std::cout << "Остановка REST API и выход...\n";
                api.stop();
                break;
            }

            if (choice == 1) {
                std::lock_guard<std::mutex> lock(mtx);
                auto items = graph.getAllItems();
                if (items.empty()) {
                    std::cout << "Список серверов пуст. Загрузите JSON (пункт 8) или добавьте сервер (пункт 4).\n";
                    continue;
                }
                std::cout << "\n" << std::left
                          << std::setw(5)  << "ID"
                          << std::setw(20) << "Имя сервера"
                          << std::setw(18) << "IP-адрес"
                          << std::setw(8)  << "Порт"
                          << std::setw(10) << "Статус" << "\n";
                std::cout << std::string(61, '-') << "\n";
                for (size_t i = 0; i < items.size(); ++i) {
                    std::cout << std::left
                              << std::setw(5)  << items[i].id
                              << std::setw(20) << items[i].name
                              << std::setw(18) << items[i].ip_address
                              << std::setw(8)  << items[i].port
                              << std::setw(10) << items[i].status << "\n";
                }
            }
            else if (choice == 2) {
                std::cout << "Введите IP-адрес сервера: ";
                std::string ip;
                std::cin >> ip;

                std::lock_guard<std::mutex> lock(mtx);
                auto item_opt = graph.getItemByIp(ip);
                if (!item_opt.has_value()) {
                    std::cout << "[!] Сервер с IP " << ip << " не найден.\n";
                    continue;
                }

                std::cout << "\nСервер: " << item_opt->name << " (" << item_opt->ip_address
                          << ":" << item_opt->port << ") | Статус: " << item_opt->status << "\n";

                auto in_deps = graph.getInboundDependencies(item_opt->id);
                std::cout << "├── ВХОДЯЩИЕ (кто обращается к нему):\n";
                if (in_deps.empty()) {
                    std::cout << "│   └── (нет входящих подключений)\n";
                } else {
                    for (size_t i = 0; i < in_deps.size(); ++i) {
                        auto src = graph.getItemById(in_deps[i].source_id);
                        if (src.has_value()) {
                            std::cout << "│   ├── <-- " << src->name << " (" << src->ip_address << ") ["
                                      << in_deps[i].protocol << " | "
                                      << (in_deps[i].is_critical ? "CRITICAL" : "NON-CRITICAL") << "]\n";
                        }
                    }
                }

                auto out_deps = graph.getOutboundDependencies(item_opt->id);
                std::cout << "└── ИСХОДЯЩИЕ (к кому обращается он сам):\n";
                if (out_deps.empty()) {
                    std::cout << "    └── (нет исходящих подключений)\n";
                } else {
                    for (size_t i = 0; i < out_deps.size(); ++i) {
                        auto tgt = graph.getItemById(out_deps[i].target_id);
                        if (tgt.has_value()) {
                            std::cout << "    └── --> " << tgt->name << " (" << tgt->ip_address << ") ["
                                      << out_deps[i].protocol << " | "
                                      << (out_deps[i].is_critical ? "CRITICAL" : "NON-CRITICAL") << "]\n";
                        }
                    }
                }
            }
            else if (choice == 3) {
                std::lock_guard<std::mutex> lock(mtx);
                auto isolated = graph.getIsolatedItems();
                if (isolated.empty()) {
                    std::cout << "Изолированных серверов нет — все участвуют в обмене данными.\n";
                } else {
                    std::cout << "\nИзолированные серверы (без входящих и исходящих связей):\n";
                    for (size_t i = 0; i < isolated.size(); ++i) {
                        std::cout << "- [" << isolated[i].id << "] " << isolated[i].name
                                  << " (" << isolated[i].ip_address << ")\n";
                    }
                }
            }
            else if (choice == 4) {
                ConfigurationItem item;
                item.ci_type = "Server";
                std::cout << "Имя сервера (без пробелов, например Web-Node-2): ";
                std::cin >> item.name;
                std::cout << "IP-адрес (например 10.0.1.50): ";
                std::cin >> item.ip_address;
                std::cout << "Основной порт (например 80): ";
                item.port = readInt();
                item.status = "ACTIVE";

                try {
                    std::lock_guard<std::mutex> lock(mtx);
                    item.id = db.insertItem(item);
                    graph.addItem(item);
                    std::cout << "[OK] Сервер добавлен с ID = " << item.id << "\n";
                } catch (const std::exception& e) {
                    std::cout << "[Ошибка БД] " << e.what() << "\n";
                }
            }
            else if (choice == 5) {
                std::string src_ip, tgt_ip, proto;
                int crit_input;
                std::cout << "IP источника (кто обращается): ";
                std::cin >> src_ip;
                std::cout << "IP цели (к кому обращаются): ";
                std::cin >> tgt_ip;
                std::cout << "Протокол (например TCP:5432 или HTTPS): ";
                std::cin >> proto;
                std::cout << "Связь критическая? (1 - да, 0 - нет): ";
                crit_input = readInt();

                std::lock_guard<std::mutex> lock(mtx);
                auto src = graph.getItemByIp(src_ip);
                auto tgt = graph.getItemByIp(tgt_ip);
                if (!src.has_value() || !tgt.has_value()) {
                    std::cout << "[!] Один из указанных IP-адресов не найден в базе.\n";
                    continue;
                }

                try {
                    Dependency dep;
                    dep.source_id = src->id;
                    dep.target_id = tgt->id;
                    dep.protocol = proto;
                    dep.is_critical = (crit_input == 1);
                    dep.id = db.insertDependency(dep);
                    graph.addDependency(dep);
                    std::cout << "[OK] Связь " << src->name << " -> " << tgt->name << " добавлена!\n";
                } catch (const std::exception& e) {
                    std::cout << "[Ошибка БД] " << e.what() << "\n";
                }
            }
            else if (choice == 6) {
                std::cout << "Введите IP-адрес сервера для изменения: ";
                std::string ip;
                std::cin >> ip;

                std::lock_guard<std::mutex> lock(mtx);
                auto item_opt = graph.getItemByIp(ip);
                if (!item_opt.has_value()) {
                    std::cout << "[!] Сервер не найден.\n";
                    continue;
                }

                std::cout << "Что изменить? (1 - Статус ACTIVE/DOWN, 2 - Новый IP-адрес): ";
                int sub = readInt();
                if (sub == 1) {
                    std::cout << "Новый статус (ACTIVE или DOWN): ";
                    std::string st;
                    std::cin >> st;
                    if (db.updateItemStatus(item_opt->id, st)) {
                        graph.updateItemStatus(item_opt->id, st);
                        std::cout << "[OK] Статус обновлен.\n";
                    }
                } else if (sub == 2) {
                    std::cout << "Новый IP-адрес: ";
                    std::string new_ip;
                    std::cin >> new_ip;
                                        try {
                        if (db.updateItemIp(item_opt->id, new_ip)) {
                            graph.updateItemIp(item_opt->id, new_ip);
                            std::cout << "[OK] IP-адрес обновлен.\n";
                        }
                    } catch (const std::exception& e) {
                        std::cout << "[Ошибка БД] " << e.what() << "\n";
                    }
                }
            }
            else if (choice == 7) {
                std::cout << "Что удалить? (1 - Сервер целиком, 2 - Связь между серверами): ";
                int sub = readInt();
                std::lock_guard<std::mutex> lock(mtx);

                if (sub == 1) {
                    std::cout << "Введите IP удаляемого сервера: ";
                    std::string ip;
                    std::cin >> ip;
                    auto item_opt = graph.getItemByIp(ip);
                    if (!item_opt.has_value()) {
                        std::cout << "[!] Сервер не найден.\n";
                        continue;
                    }
                    if (db.deleteItem(item_opt->id)) {
                        graph.removeItem(item_opt->id);
                        std::cout << "[OK] Сервер и все его связи удалены.\n";
                    }
                } else if (sub == 2) {
                    std::string src_ip, tgt_ip;
                    std::cout << "IP источника: ";
                    std::cin >> src_ip;
                    std::cout << "IP цели: ";
                    std::cin >> tgt_ip;
                    auto src = graph.getItemByIp(src_ip);
                    auto tgt = graph.getItemByIp(tgt_ip);
                    if (src.has_value() && tgt.has_value() && db.deleteDependency(src->id, tgt->id)) {
                        graph.removeDependency(src->id, tgt->id);
                        std::cout << "[OK] Связь удалена.\n";
                    } else {
                        std::cout << "[!] Связь не найдена.\n";
                    }
                }
            }
            else if (choice == 8) {
                std::cout << "1 - Загрузить из JSON-файла\n2 - Сохранить в JSON-файл\nВыбор: ";
                int sub = readInt();
                std::cout << "Путь к файлу (например ../data/sample_topology.json или export.json): ";
                std::string path;
                std::cin >> path;

                std::lock_guard<std::mutex> lock(mtx);
                if (sub == 1) {
                    if (JsonSerializer::importFromFile(path, db, graph)) {
                        std::cout << "[OK] Топология успешно загружена в PostgreSQL!\n";
                    } else {
                        std::cout << "[!] Не удалось загрузить JSON-файл.\n";
                    }
                } else if (sub == 2) {
                    if (JsonSerializer::exportToFile(path, graph)) {
                        std::cout << "[OK] Топология сохранена в файл " << path << "\n";
                    } else {
                        std::cout << "[!] Ошибка записи файла.\n";
                    }
                }
            }
            else if (choice == 9) {
                std::cout << "Введите IP упавшего сервера для расчета зоны риска (или 0 для обычной схемы): ";
                std::string ip;
                std::cin >> ip;

                std::lock_guard<std::mutex> lock(mtx);
                int failed_id = -1;

                if (ip != "0") {
                    auto item_opt = graph.getItemByIp(ip);
                    if (!item_opt.has_value()) {
                        std::cout << "[!] Сервер с IP " << ip << " не найден.\n";
                        continue;
                    }
                    failed_id = item_opt->id;
                    auto impacted = graph.analyzeImpact(failed_id);

                    std::cout << "\n=== АНАЛИЗ ЗОНЫ РИСКА ПРИ ПАДЕНИИ " << item_opt->name << " (" << ip << ") ===\n";
                    if (impacted.empty()) {
                        std::cout << "Ни один другой сервер не зависит от этого узла.\n";
                    } else {
                        for (size_t i = 0; i < impacted.size(); ++i) {
                            std::cout << "[Шаг каскада " << impacted[i].distance << "] "
                                      << impacted[i].item.name << " (" << impacted[i].item.ip_address << ") -> "
                                      << (impacted[i].is_critical_impact ? "КРИТИЧЕСКИЙ ОТКАЗ" : "Частичная деградация")
                                      << "\n";
                        }
                    }
                }

                if (GraphVisualizer::renderToPng(graph, "topology.png", failed_id)) {
                    std::cout << "[OK] Визуальная схема сохранена в файл topology.png!\n";
                } else {
                    std::cout << "[!] Ошибка генерации схемы через Graphviz.\n";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Критическая ошибка] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
