#pragma once

#include <memory>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <cmath>

namespace graph {
    class AdjMat {
    public:
        /// Use 0 as self or unconnected
        explicit AdjMat(uint32_t n)
                : n_(n), data_(std::make_unique<uint16_t[]>(n * n)) {}

        AdjMat(const AdjMat &) = delete;

        AdjMat &operator=(const AdjMat &) = delete;

        AdjMat(AdjMat &&) noexcept = delete;

        AdjMat &operator=(AdjMat &&) noexcept = delete;

        void bi_set(uint32_t i, uint32_t j, uint16_t w) {
            if (!(check_bounds(i) && check_bounds(j))) {
                throw std::out_of_range("Index out of bounds");
            }
            if (i == j) {
                throw std::out_of_range("Should not modify diagonal");
            }
            data_[i * n_ + j] = w;
            data_[j * n_ + i] = w;
        }

        void set(uint32_t i, uint32_t j, uint16_t w) {
            if (!(check_bounds(i) && check_bounds(j))) {
                throw std::out_of_range("Index out of bounds");
            }
            if (i == j) {
                throw std::out_of_range("Should not modify diagonal");
            }
            data_[i * n_ + j] = w;
        }

        uint16_t operator()(uint32_t i, uint32_t j) const {
            if (!(check_bounds(i) && check_bounds(j))) {
                throw std::out_of_range("Index out of bounds");
            }

            return data_[i * n_ + j];
        }

        uint16_t &operator()(uint32_t i, uint32_t j) {
            if (!(check_bounds(i) && check_bounds(j))) {
                throw std::out_of_range("Index out of bounds");
            }
            if (i == j) {
                throw std::out_of_range("Diagonal not available in non-const mode");
            }
            return data_[i * n_ + j];
        }

        [[nodiscard]] uint32_t size() const { return n_; }

        [[nodiscard]] const uint16_t *raw() const { return data_.get(); }

    private:

        /// Used for clang/g++ branch predicting, will be constexpr true in safe code
        [[nodiscard]] [[gnu::always_inline]] inline bool check_bounds(uint32_t i) const {
            if (i >= n_) return false;
            return true;
        }

        const uint32_t n_;
        const std::unique_ptr<uint16_t[]> data_;
    };

}

#include <vector>
#include <queue>
#include <limits>
#include <cstdint>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <numeric>
#include <tuple>
#include <cstring>
#include <stack>

namespace graph::algorithms {

    using std::vector;
    using std::pair;
    using std::queue;
    using std::unordered_set;
    using std::unordered_map;

    static constexpr uint16_t NO_EDGE = 0;
    static constexpr int32_t INF = std::numeric_limits<int32_t>::max();

    class ColIter {
    public:
        using iterator_category [[maybe_unused]] = std::input_iterator_tag;
        using value_type [[maybe_unused]] = uint16_t;
        using reference [[maybe_unused]] = uint16_t;
        using pointer [[maybe_unused]] = const uint16_t *;
        using difference_type [[maybe_unused]] = std::ptrdiff_t;

        ColIter(const uint16_t *base, size_t stride, size_t fixed_col, size_t exclude)
                : base_(base), stride_(stride), col_(fixed_col), exclude_(exclude), i_(0) {}

        bool operator==(const ColIter &other) const { return i_ == other.i_; }

        bool operator!=(const ColIter &other) const { return i_ != other.i_; }

        void operator++() { ++i_; }

        uint16_t operator*() const {
            if (i_ == exclude_) return NO_EDGE;  // simulate skipping self
            return base_[i_ * stride_ + col_];
        }

        ColIter begin() { return *this; }

        ColIter end() { return {base_, stride_, col_, exclude_, stride_}; }

    private:
        ColIter(const uint16_t *base, size_t stride, size_t col, size_t exclude, size_t i)
                : base_(base), stride_(stride), col_(col), exclude_(exclude), i_(i) {}

        const uint16_t *base_;
        size_t stride_;
        size_t col_;
        size_t exclude_;
        size_t i_;
    };

    struct degree {
        uint32_t out;
        uint32_t in;

        constexpr bool operator==(const degree &) const = default;
    };

    /// Q2: Get degree
    degree get_degree(const AdjMat &mat, uint32_t node, bool directed) {
        const uint32_t n = mat.size();
        const uint16_t *data = mat.raw();

        if (node >= n) {
            throw std::out_of_range("Node index out of bounds");
        }

        auto out_deg = static_cast<uint32_t>(std::count_if(data + node * n, data + (node + 1) * n,
                                                           [](uint16_t val) { return val != NO_EDGE; }));

        uint32_t in_deg = 0;
        if (directed) {
            for (uint32_t i = 0; i < n; ++i) {
                if (i == node) [[unlikely]] continue;
                if (data[i * n + node] != NO_EDGE) ++in_deg;
            }
        } else {
            in_deg = out_deg;
        }
        return {out_deg, in_deg};
    }


    struct stats {
        double avg;
        uint32_t min_deg;
        uint32_t max_deg;
        double edge_density;

        constexpr bool operator==(const stats &) const = default;
    };

    /// Q2: Degree statistics
    stats degree_stats(const AdjMat &mat, bool directed) {
        const uint32_t n = mat.size();

        vector<uint32_t> degrees;
        degrees.reserve(n);

        for (uint32_t i = 0; i < n; ++i) {
            auto deg = static_cast<uint32_t>(std::count_if(mat.raw() + i * n, mat.raw() + (i + 1) * n,
                                                           [](uint16_t v) { return v != NO_EDGE; }));
            degrees.emplace_back(deg);
        }

        uint32_t total = std::accumulate(degrees.begin(), degrees.end(), uint32_t(0));
        uint32_t min_deg = *std::min_element(degrees.begin(), degrees.end());
        uint32_t max_deg = *std::max_element(degrees.begin(), degrees.end());
        uint32_t edge_count = directed ? total : total / 2;
        double edge_density = static_cast<double>(edge_count) /
                              static_cast<double>(directed ? (n * (n - 1)) : (n * (n - 1) / 2));

        return {static_cast<double>(total) / static_cast<double>(n), min_deg, max_deg, edge_density};
    }


    /// Q2: Isolated nodes
    vector<uint32_t> isolated_nodes(const AdjMat &mat, bool directed) {
        const uint32_t n = mat.size();
        const uint16_t *data = mat.raw();
        vector<uint32_t> candidates;

        // Step 1: collect all with no outbound
        for (uint32_t i = 0; i < n; ++i) {
            const uint16_t *row = data + i * n;
            bool no_out = std::none_of(row, row + n, [](uint16_t v) { return v != NO_EDGE; });
            if (no_out) candidates.push_back(i);
        }

        if (!directed) {
            return candidates;
        }

        vector<uint32_t> result;

        // Step 2: for candidates, check inbound using ColIter
        for (size_t i: candidates) {
            ColIter col(data, n, i, i);
            if (std::none_of(col.begin(), col.end(), [](uint16_t v) { return v != NO_EDGE; })) {
                result.push_back(i);
            }
        }

        return result;
    }

    /// Q3: Triangle counting (undirected or directed)
    uint64_t count_triangles(const AdjMat &mat, bool directed) {
        const uint64_t n = mat.size();
        const uint16_t *data = mat.raw();
        uint64_t count = 0;

        if (!directed) {
            for (uint64_t i = 0; i < n; ++i) {
                for (uint64_t j = i + 1; j < n; ++j) {
                    if (mat(i, j) == NO_EDGE) continue;

                    const uint16_t *row_i = data + i * n;
                    const uint16_t *row_j = data + j * n;

                    uint64_t k = j + 1;
                    count += static_cast<uint64_t>(std::count_if(
                            row_j + k, row_j + n,
                            [&](uint16_t val_jk) mutable {
                                bool valid = row_i[k] != NO_EDGE && val_jk != NO_EDGE;
                                ++k;
                                return valid;
                            }
                    ));
                }
            }

        } else {
            for (uint64_t i = 0; i < n; ++i) {
                for (uint64_t j = 0; j < n; ++j) {
                    if (i == j) [[unlikely]] continue;
                    if (mat(i, j) == NO_EDGE) continue;

                    const uint16_t *row_j = data + j * n;
                    ColIter col(data, n, i, i);

                    uint64_t k = 0;
                    count += static_cast<uint64_t>(std::count_if(
                            col.begin(), col.end(),
                            [&](uint16_t v_k_i) mutable {
                                if (k == i || k == j) {
                                    ++k;
                                    return false;
                                }
                                bool valid = row_j[k] != NO_EDGE && v_k_i != NO_EDGE;
                                ++k;
                                return valid;
                            }
                    ));
                }
            }
            count /= 3;
        }

        return count;
    }

    /// Q4: Shortest path using BFS or Dijkstra
    vector<int32_t> shortest_path(const AdjMat &mat, uint32_t start, bool weighed) {
        const uint32_t n = mat.size();
        if (start >= n) {
            throw std::out_of_range("Node index out of bounds");
        }
        vector<int32_t> dist(n, INF);
        dist[start] = 0;

        if (!weighed) {
            /// BFS
            queue<uint32_t> q;
            q.push(start);
            while (!q.empty()) {
                uint32_t u = q.front();
                q.pop();
                for (uint32_t v = 0; v < n; ++v) {
                    if (u == v) [[unlikely]] continue;
                    if (mat(u, v) != NO_EDGE && dist[v] == INF) {
                        dist[v] = dist[u] + 1;
                        q.push(v);
                    }
                }
            }
        } else {
            /// DIJKSTRA
            using Node = pair<int32_t, uint32_t>; // dist, node
            std::priority_queue<Node, vector<Node>, std::greater<>> pq;
            pq.emplace(0, start);
            while (!pq.empty()) {
                auto [d, u] = pq.top();
                pq.pop();
                if (d > dist[u]) [[unlikely]] continue;
                for (uint32_t v = 0; v < n; ++v) {
                    if (u == v) [[unlikely]] continue;
                    if (mat(u, v) == NO_EDGE) continue;
                    int32_t alt = dist[u] + mat(u, v);
                    if (alt < dist[v]) [[likely]] {
                        dist[v] = alt;
                        pq.emplace(alt, v);
                    }
                }
            }
        }
        return dist;
    }

    /// Q5: Betweenness centrality
    vector<double> betweenness_centrality(const AdjMat &mat, bool weighed) {
        const uint32_t n = mat.size();
        vector<double> centrality(n, 0.0);

        for (uint32_t s = 0; s < n; ++s) {
            vector<int32_t> dist(n, INF);
            vector<vector<uint32_t>> preds(n);
            vector<uint32_t> sigma(n, 0);
            vector<double> delta(n, 0.0);
            std::stack<uint32_t> stack;

            dist[s] = 0;
            sigma[s] = 1;

            if (!weighed) {
                queue<uint32_t> q;
                q.push(s);
                while (!q.empty()) {
                    uint32_t v = q.front();
                    q.pop();
                    stack.push(v);
                    for (uint32_t w = 0; w < n; ++w) {
                        if (v == w) [[unlikely]] continue;
                        if (mat(v, w) == NO_EDGE) continue;
                        if (dist[w] == INF) [[likely]] {
                            dist[w] = dist[v] + 1;
                            q.push(w);
                        }
                        if (dist[w] == dist[v] + 1) [[likely]] {
                            sigma[w] += sigma[v];
                            preds[w].push_back(v);
                        }
                    }
                }
            } else {
                using Node = pair<int32_t, uint32_t>; // dist, node
                std::priority_queue<Node, vector<Node>, std::greater<>> pq;
                pq.emplace(0, s);
                while (!pq.empty()) {
                    auto [d, v] = pq.top();
                    pq.pop();
                    if (d > dist[v]) [[unlikely]] continue;
                    stack.push(v);
                    for (uint32_t w = 0; w < n; ++w) {
                        if (v == w) [[unlikely]] continue;
                        if (mat(v, w) == NO_EDGE) continue;
                        int32_t weight = mat(v, w);
                        int32_t alt = dist[v] + weight;
                        if (alt < dist[w]) [[likely]] {
                            dist[w] = alt;
                            pq.emplace(alt, w);
                            sigma[w] = sigma[v];
                            preds[w].clear();
                            preds[w].push_back(v);
                        } else if (alt == dist[w]) [[likely]] {
                            sigma[w] += sigma[v];
                            preds[w].push_back(v);
                        }
                    }
                }
            }

            while (!stack.empty()) {
                uint32_t w = stack.top();
                stack.pop();
                for (uint32_t v: preds[w]) {
                    delta[v] += (static_cast<double>(sigma[v]) / sigma[w]) * (1.0 + delta[w]);
                }
                if (w != s) [[likely]] {
                    centrality[w] += delta[w];
                }
            }
        }

        for (auto &val: centrality) val /= 2.0;
        return centrality;
    }

    /// === Helper funcs ===

    std::vector<uint32_t> get_from(const graph::AdjMat &mat, uint32_t node) {
        const uint32_t n = mat.size();
        if (node >= n) {
            throw std::out_of_range("Node index out of bounds");
        }
        const uint16_t *row = mat.raw() + node * n;

        std::vector<uint32_t> result;
        result.reserve(std::max<size_t>(64, n / 2));

        for (uint32_t j = 0; j < n; ++j) {
            if (j == node) [[unlikely]] continue;
            if (row[j] == graph::algorithms::NO_EDGE) continue;
            result.emplace_back(j);
        }

        return result;
    }

    std::vector<uint32_t> get_to(const graph::AdjMat &mat, uint32_t node) {
        const uint32_t n = mat.size();
        if (node >= n) {
            throw std::out_of_range("Node index out of bounds");
        }
        const uint16_t *data = mat.raw();

        std::vector<uint32_t> result;
        result.reserve(std::max<size_t>(64, n / 2));

        for (uint32_t i = 0; i < n; ++i) {
            if (i == node) [[unlikely]] continue;
            if (data[i * n + node] == graph::algorithms::NO_EDGE) continue;
            result.emplace_back(i);
        }

        return result;
    }

    std::vector<uint32_t> get_neighbours(const graph::AdjMat &mat, uint32_t node, bool bi = false) {
        const uint32_t n = mat.size();
        if (node >= n) {
            throw std::out_of_range("Node index out of bounds");
        }
        const uint16_t *data = mat.raw();
        const uint16_t *row = data + node * n;

        std::vector<uint32_t> result;
        result.reserve(std::max<size_t>(64, n / 2));

        for (uint32_t i = 0; i < n; ++i) {
            if (i == node) [[unlikely]] continue;

            if (bi) {
                if (row[i] == graph::algorithms::NO_EDGE) continue;
                result.emplace_back(i);
            } else {
                if (row[i] == graph::algorithms::NO_EDGE &&
                    data[i * n + node] == graph::algorithms::NO_EDGE)
                    continue;
                result.emplace_back(i);
            }
        }

        return result;
    }

} // namespace graph::algorithms
