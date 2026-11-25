#pragma once

#include "TopologyGraph.hpp"
#include "DatabaseManager.hpp"
#include <httplib.h>
#include <mutex>
#include <thread>
#include <string>

class ApiServer {
public:
    ApiServer(TopologyGraph& graph, DatabaseManager& db, std::mutex& mtx);
    ~ApiServer();

    // Запуск HTTP-сервера в фоновом потоке
    void start(int port = 8080);

    // Остановка сервера при выходе из программы
    void stop();

private:
    bool checkToken(const httplib::Request& req);

    TopologyGraph& graph_;
    DatabaseManager& db_;
    std::mutex& mtx_;

    httplib::Server svr_;
    std::thread server_thread_;
    bool is_running_ = false;
};
