cmake_minimum_required(VERSION 3.14)
project(CMDBTopologyAnalyzer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

enable_testing()
add_executable(cmdb_tests tests/test_topology.cpp src/TopologyGraph.cpp src/GraphVisualizer.cpp)
add_test(NAME TopologyUnitTests COMMAND cmdb_tests)
