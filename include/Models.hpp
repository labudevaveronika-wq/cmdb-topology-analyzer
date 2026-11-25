#pragma once

#include <string>
#include <vector>


struct ConfigurationItem
{
    int id = 0;
    std::string name;
    std::string ci_type = "Server";
    std::string ip_address;
    int port = 0;
    std::string status = "ACTIVE";
};

struct Dependency {
    int id = 0;
    int source_id = 0;
    int target_id = 0;
    std::string protocol = "TCP";
    bool is_critical = true;
};

struct ImpactedNode {
    ConfigurationItem item;
    bool is_critical_impact; 
    int distance;   
};