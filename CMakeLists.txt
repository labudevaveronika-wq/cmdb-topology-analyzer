cmake_minimum_required(VERSION 3.14)
project(CMDBTopologyAnalyzer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
pkg_check_modules(HTTPLIB REQUIRED cpp-httplib)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PQXX_INCLUDE_DIRS}
    ${HTTPLIB_INCLUDE_DIRS}
)

set(CORE_SOURCES
    src/TopologyGraph.cpp
    src/GraphVisualizer.cpp
    src/DatabaseManager.cpp
    src/JsonSerializer.cpp
    src/ApiServer.cpp
)

# Основная консольная программа + REST API
add_executable(cmdb_app src/main.cpp ${CORE_SOURCES})
target_link_libraries(cmdb_app PRIVATE ${PQXX_LIBRARIES} ${HTTPLIB_LIBRARIES} Threads::Threads)

# Юнит-тесты
enable_testing()
add_executable(cmdb_tests tests/test_topology.cpp ${CORE_SOURCES})
target_link_libraries(cmdb_tests PRIVATE ${PQXX_LIBRARIES} ${HTTPLIB_LIBRARIES} Threads::Threads)
add_test(NAME TopologyUnitTests COMMAND cmdb_tests)
