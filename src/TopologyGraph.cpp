#include "TopologyGraph.hpp"
#include <queue>
#include <unordered_set>
#include <algorithm>

void TopologyGraph::clear() {
    items_by_id_.clear();
    id_by_ip_.clear();
    outbound_.clear();
    inbound_.clear();
}

void TopologyGraph::addItem(const ConfigurationItem& item) {
    items_by_id_[item.id] = item;
    id_by_ip_[item.ip_address] = item.id;
}

bool TopologyGraph::removeItem(int id) {
    if (items_by_id_.find(id) == items_by_id_.end()) {
        return false;
    }
    std::string old_ip = items_by_id_[id].ip_address;
    id_by_ip_.erase(old_ip);
    items_by_id_.erase(id);
    outbound_.erase(id);
    inbound_.erase(id);

    for (auto it = outbound_.begin(); it != outbound_.end(); ++it) {
        std::vector<Dependency> clean_list;
        for (size_t i = 0; i < it->second.size(); ++i) {
            if (it->second[i].target_id != id) {
                clean_list.push_back(it->second[i]);
            }
        }
        it->second = clean_list;
    }
};

bool TopologyGraph::updateItemStatus(int id, const std::string& new_status) {
    if (items_by_id_.find(id) == items_by_id_.end()) {
        return false;
    }
    items_by_id_[id].status = new_status;
    return true;
}

bool TopologyGraph::updateItemIp(int id, const std::string& new_ip) {
    if (items_by_id_.find(id) == items_by_id_.end()) {
        return false;
    }
    std::string old_ip = items_by_id_[id].ip_address;
    id_by_ip_.erase(old_ip);

    items_by_id_[id].ip_address = new_ip;
    id_by_ip_[new_ip] = id;
    return true;
}


void TopologyGraph::addDependency(const Dependency& dep) {
    outbound_[dep.source_id].push_back(dep);
    inbound_[dep.target_id].push_back(dep);
}

bool TopologyGraph::removeDependency(int source_id, int target_id) {
    bool removed = false; 
    if (outbound_.find(source_id) != outbound_.end()) {
        std::vector<Dependency> new_list;
        for (size_t i = 0; i < outbound_[source_id].size(); ++i) {
            if (outbound_[source_id][i].target_id == target_id) {
                removed = true; 
            } else {
                new_list.push_back(outbound_[source_id][i]); 
            }
        }
        outbound_[source_id] = new_list; 
    }
    if (inbound_.find(target_id) != inbound_.end()) {
        std::vector<Dependency> new_list;
        for (size_t i = 0; i < inbound_[target_id].size(); ++i) {
            if (inbound_[target_id][i].source_id != source_id) {
                new_list.push_back(inbound_[target_id][i]); 
            } 
        }
        inbound_[target_id] = new_list;
    }
    return removed; 
}

std::optional<ConfigurationItem> TopologyGraph::getItemById(int id) const {
    auto it = items_by_id_.find(id);
    if (it == items_by_id_.end()) return std::nullopt;
    return it->second;
}


std::optional<ConfigurationItem> TopologyGraph::getItemByIp(const std::string& ip) const {
    auto it = id_by_ip_.find(ip);
    if (it == id_by_ip_.end()) return std::nullopt;
    return getItemById(it->second);
}

std::vector<ConfigurationItem> TopologyGraph::getAllItems() const {
    std::vector<ConfigurationItem> result;
    result.reserve(items_by_id_.size());
    for (const auto& [id, item] : items_by_id_) {
        result.push_back(item);
    }
    return result;
}

std::vector<Dependency> TopologyGraph::getInboundDependencies(int target_id) const {
    auto it = inbound_.find(target_id);
    if (it == inbound_.end()) return {};
    return it->second;
}

std::vector<Dependency> TopologyGraph::getOutboundDependencies(int source_id) const {
    auto it = outbound_.find(source_id);
    if (it == outbound_.end()) return {};
    return it->second;
}

std::vector<ConfigurationItem> TopologyGraph::getIsolatedItems() const {
    std::vector<ConfigurationItem> isolated;
    for (const auto& [id, item] : items_by_id_) {
        bool has_in = inbound_.count(id) && !inbound_.at(id).empty();
        bool has_out = outbound_.count(id) && !outbound_.at(id).empty();
        if (!has_in && !has_out) {
            isolated.push_back(item);
        }
    }
    return isolated;
}

std::vector<ImpactedNode> TopologyGraph::analyzeImpact(int failed_item_id) const {
    std::vector<ImpactedNode> impacted;
    if (items_by_id_.find(failed_item_id) == items_by_id_.end()) {
        return impacted;
    }

    struct QueueState {
        int id;
        int depth;
        bool critical;
    };

    std::queue<QueueState> q;
    std::unordered_set<int> visited;

    visited.insert(failed_item_id);
    q.push({failed_item_id, 0, true});

    while (!q.empty()) {
        auto curr = q.front();
        q.pop();

        auto it = inbound_.find(curr.id);
        if (it == inbound_.end()) continue;

        for (const auto& dep : it->second) {
            if (visited.find(dep.source_id) == visited.end()) {
                visited.insert(dep.source_id);
                bool next_critical = curr.critical && dep.is_critical;
                auto item_opt = getItemById(dep.source_id);
                if (item_opt) {
                    impacted.push_back({*item_opt, next_critical, curr.depth + 1});
                    q.push({dep.source_id, curr.depth + 1, next_critical});
                }
            }
        }
    }
    return impacted;
}
