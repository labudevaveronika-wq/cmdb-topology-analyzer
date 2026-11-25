#pragma once

#include "Models.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>


class TopologyGraph {
public:
    void clear();

    void addItem(const ConfigurationItem& item);
    bool removeItem(int id);
    bool updateItemStatus(int id, const std::string& new_status);
    bool updateItemIp(int id, const std::string& new_ip);

    void addDependency(const Dependency& dep);
    bool removeDependency(int source_id, int target_id);

    std::optional<ConfigurationItem> getItemById(int id) const;
    std::optional<ConfigurationItem> getItemByIp(const std::string& ip) const;
    std::vector<ConfigurationItem> getAllItems() const;

    std::vector<Dependency> getInboundDependencies(int target_id) const;
    std::vector<Dependency> getOutboundDependencies(int source_id) const;
    std::vector<ConfigurationItem> getIsolatedItems() const;

    std::vector<ImpactedNode> analyzeImpact(int failed_item_id) const;

private:
    std::unordered_map<int, ConfigurationItem> items_by_id_;
    std::unordered_map<std::string, int> id_by_ip_;
    std::unordered_map<int, std::vector<Dependency>> outbound_;
    std::unordered_map<int, std::vector<Dependency>> inbound_;
};
