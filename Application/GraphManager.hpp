#pragma once

#include "../Algo/Graph.hpp"
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <shared_mutex>
#include <vector>
#include <atomic>
#include <stdexcept>

class GraphManager {
public:
    struct line {
        std::uint16_t u;
        std::uint16_t v;
        std::uint16_t weight;
    };

    static GraphManager &instance() {
        static GraphManager inst;
        return inst;
    }

    uint64_t create(const uint32_t size) {
        std::lock_guard lock(mutex_);
        uint64_t id = next_id_++;
        registry_[id] = std::make_unique<graph::AdjMat>(size);
        return id;
    }

    bool exists(const uint64_t id) {
        std::shared_lock lock(mutex_);
        return registry_.contains(id);
    }

    void set(const uint64_t id, const line &l, const bool bi) {
        std::lock_guard lock(mutex_);
        auto it = registry_.find(id);
        if (it == registry_.end()) {
            throw std::out_of_range("Invalid graph ID");
        }
        if (bi) {
            it->second->bi_set(l.u, l.v, l.weight);
        } else {
            it->second->set(l.u, l.v, l.weight);
        }
    }

    void bash_set(const uint64_t id, const std::vector<line> &lines, const bool bi) {
        std::unique_lock lock(mutex_);
        auto it = registry_.find(id);
        if (it == registry_.end()) {
            throw std::out_of_range("Invalid graph ID");
        }
        for (const auto &l: lines) {
            if (bi) {
                it->second->bi_set(l.u, l.v, l.weight);
            } else {
                it->second->set(l.u, l.v, l.weight);
            }
        }
    }

    graph::algorithms::degree get_degree(const uint64_t id, const uint32_t node, const bool directed) {
        std::shared_lock lock(mutex_);
        return graph::algorithms::get_degree(*get_graph(id), node, directed);
    }

    graph::algorithms::stats degree_stats(const uint64_t id, const bool directed) {
        std::shared_lock lock(mutex_);
        return graph::algorithms::degree_stats(*get_graph(id), directed);
    }

    std::vector<uint32_t> isolated_nodes(const uint64_t id, const bool directed) {
        std::shared_lock lock(mutex_);
        return graph::algorithms::isolated_nodes(*get_graph(id), directed);
    }

    uint64_t count_triangles(const uint64_t id, const bool directed) {
        std::shared_lock lock(mutex_);
        return graph::algorithms::count_triangles(*get_graph(id), directed);
    }

    std::vector<int32_t> shortest_path(const uint64_t id, const uint32_t start, const bool weighed) {
        std::shared_lock lock(mutex_);
        return graph::algorithms::shortest_path(*get_graph(id), start, weighed);
    }

    std::vector<double> betweenness_centrality(const uint64_t id, const bool weighed) {
        std::shared_lock lock(mutex_);
        return graph::algorithms::betweenness_centrality(*get_graph(id), weighed);
    }

    std::vector<uint32_t> get_from(uint64_t id, uint32_t node) {
        std::shared_lock lock(mutex_);
        return graph::algorithms::get_from(*get_graph(id), node);
    }

    std::vector<uint32_t> get_to(uint64_t id, uint32_t node) {
        std::shared_lock lock(mutex_);
        return graph::algorithms::get_to(*get_graph(id), node);
    }

    std::vector<uint32_t> get_neighbours(uint64_t id, uint32_t node, bool bi = false) {
        std::shared_lock lock(mutex_);
        return graph::algorithms::get_neighbours(*get_graph(id), node, bi);
    }

    bool destroy(uint64_t id) {
        std::lock_guard lock(mutex_);
        return registry_.erase(id) > 0;
    }

    void atexit() {
        std::lock_guard lock(mutex_);
        registry_.clear();
    }

    std::vector<uint64_t> list_ids() {
        std::shared_lock lock(mutex_);
        std::vector<uint64_t> ids;
        ids.reserve(registry_.size());
        for (const auto &[id, _]: registry_) {
            ids.push_back(id);
        }
        return ids;
    }

    GraphManager(const GraphManager &) = delete;

    GraphManager &operator=(const GraphManager &) = delete;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<uint64_t, std::unique_ptr<graph::AdjMat>> registry_;
    std::atomic<uint64_t> next_id_{1};

    GraphManager() = default;

    ~GraphManager() = default;

    graph::AdjMat *get_graph(uint64_t id) const {
        auto it = registry_.find(id);
        if (it == registry_.end()) {
            throw std::out_of_range("Graph not found");
        }
        return it->second.get();
    }
};
