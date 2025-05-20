#include "views.hpp"
#include "bulgogi.hpp"
#include "template.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <iostream>

namespace json = boost::json;
using bulgogi::Request; /// @brief HTTP request
using bulgogi::Response; /// @brief HTTP response
using bulgogi::check_method; /// @brief Check HTTP method
using bulgogi::set_json; /// @brief Set JSON response

#include "../Application/GraphManager.hpp"
#include <regex>

/// @brief Global function map for registered urls
std::unordered_map<std::string, views::HandlerFunc> views::function_map;

/// @brief Atomic boolean to signal server shutdown
extern std::atomic<bool> g_should_exit;

/// @brief Global acceptor for TCP connections
extern std::unique_ptr<boost::asio::ip::tcp::acceptor> global_acceptor;

/// @brief Default root view for the server
REGISTER_ROOT_VIEW(default_root) {
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    bulgogi::set_html(res, default_page::html, 200);
}

void views::init() {
    (void)GraphManager::instance(); // Hot load the singleton
}

void views::atexit() {
    GraphManager::instance().atexit();
    // Clean the resources held by GraphManager
}

void views::check_head([[maybe_unused]] const bulgogi::Request &req) {
    auto auth_header = req[bulgogi::http::field::authorization];

    // Convert auth_header to std::string (if it's not already)
    std::string auth_str(auth_header); // Works for string_view, char*, etc.

    // Check if the format is "Bearer <alphanumeric>"
    std::regex bearer_pattern(R"(^Bearer [a-zA-Z0-9\-_.]+$)");

    if (auth_str.empty() || !std::regex_match(auth_str, bearer_pattern)) {
        throw std::runtime_error("Unauthorized: Invalid token format");
    }
}


REGISTER_VIEW(ping) {
    if (!check_method(req, bulgogi::http::verb::get, res)) return;
    set_json(res, {{"status", "alive"}});
}

REGISTER_VIEW(shutdown_server) {
    if (!check_method(req, bulgogi::http::verb::post, res)) return;

    if (!g_should_exit.exchange(true)) {
        std::cout << "Called Exit\n";

        if (global_acceptor && global_acceptor->is_open()) {
            boost::system::error_code ec;
            global_acceptor->cancel(ec); // NOLINT
        }

        try {
            boost::asio::io_context ioc;
            boost::asio::ip::tcp::socket s(ioc);
            s.connect({boost::asio::ip::address_v4::loopback(), PORT});
        } catch (...) {}
    }

    set_json(res, {{"status", "server_shutdown_requested"}});
}


REGISTER_VIEW_URLS(graph_create, "graph/create") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::post, res)) return;
    auto json = bulgogi::get_json_obj(req);
    if (!json.contains("size")) {
        bulgogi::set_json(res, {{"error", "missing size"}}, 400);
        return;
    }
    uint32_t size = json["size"].as_int64();
    uint64_t id = GraphManager::instance().create(size);
    bulgogi::set_json(res, {{"id", id}});
}


REGISTER_VIEW_URLS(graph_exists, "graph/exists") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    if (!id_opt) {
        bulgogi::set_json(res, {{"error", "missing id"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    bool exists = GraphManager::instance().exists(id);
    bulgogi::set_json(res, {{"exists", exists}});
}


REGISTER_VIEW_URLS(graph_set, "graph/add-edge") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::post, res)) return;
    auto json = bulgogi::get_json_obj(req);
    if (!json.contains("id") || !json.contains("u") || !json.contains("v") || !json.contains("weight")) {
        bulgogi::set_json(res, {{"error", "missing params"}}, 400);
        return;
    }
    uint64_t id = json["id"].as_int64();
    GraphManager::line l{static_cast<uint16_t>(json["u"].as_int64()),
                         static_cast<uint16_t>(json["v"].as_int64()),
                         static_cast<uint16_t>(json["weight"].as_int64())};
    bool bi = json["bi"].as_bool();
    try {
        GraphManager::instance().set(id, l, bi);
        bulgogi::set_json(res, {{"status", "ok"}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}


REGISTER_VIEW_URLS(graph_bash_set, "graph/batch-edges") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::post, res)) return;
    auto json = bulgogi::get_json_obj(req);
    if (!json.contains("id") || !json.contains("lines")) {
        bulgogi::set_json(res, {{"error", "missing params"}}, 400);
        return;
    }
    uint64_t id = json["id"].as_int64();
    bool bi = json["bi"].as_bool();
    std::vector<GraphManager::line> lines;
    for (const auto& item : json["lines"].as_array()) {
        auto it = item.as_object();
        if (!it.contains("u") || !it.contains("v") || !it.contains("weight")) continue;
        lines.push_back({static_cast<uint16_t>(it["u"].as_int64()),
                         static_cast<uint16_t>(it["v"].as_int64()),
                         static_cast<uint16_t>(it["weight"].as_int64())});
    }
    try {
        GraphManager::instance().bash_set(id, lines, bi);
        bulgogi::set_json(res, {{"status", "ok"}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_degree, "graph/degree") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    auto node_opt = bulgogi::get_query_param(req, "node");
    bool directed = bulgogi::get_query_param(req, "directed").value_or("0") == "1";
    if (!id_opt || !node_opt) {
        bulgogi::set_json(res, {{"error", "missing id or node"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    uint32_t node = std::stoul(*node_opt);
    try {
        auto deg = GraphManager::instance().get_degree(id, node, directed);
        bulgogi::set_json(res, {{"in", deg.in}, {"out", deg.out}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_degree_stats, "graph/degree_stats") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    bool directed = bulgogi::get_query_param(req, "directed").value_or("0") == "1";
    if (!id_opt) {
        bulgogi::set_json(res, {{"error", "missing id"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    try {
        auto stats = GraphManager::instance().degree_stats(id, directed);
        bulgogi::set_json(res, {
            {"min", stats.min_deg},
            {"max", stats.max_deg},
            {"density", stats.edge_density},
            {"avg", stats.avg}
        });
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_isolated_nodes, "graph/isolated_nodes") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    bool directed = bulgogi::get_query_param(req, "directed").value_or("0") == "1";
    if (!id_opt) {
        bulgogi::set_json(res, {{"error", "missing id"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    try {
        auto nodes = GraphManager::instance().isolated_nodes(id, directed);
        boost::json::array json_arr;
        for (const auto val : nodes) {
            json_arr.emplace_back(val);
        }
        bulgogi::set_json(res, {{"nodes",json_arr}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_count_triangles, "graph/count_triangles") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    bool directed = bulgogi::get_query_param(req, "directed").value_or("0") == "1";
    if (!id_opt) {
        bulgogi::set_json(res, {{"error", "missing id"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    try {
        auto count = GraphManager::instance().count_triangles(id, directed);
        bulgogi::set_json(res, {{"count", count}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_shortest_path, "graph/shortest_path") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    auto start_opt = bulgogi::get_query_param(req, "start");
    bool weighed = bulgogi::get_query_param(req, "weighed").value_or("0") == "1";
    if (!id_opt || !start_opt) {
        bulgogi::set_json(res, {{"error", "missing id or start"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    uint32_t start = std::stoul(*start_opt);
    try {
        auto path = GraphManager::instance().shortest_path(id, start, weighed);
        boost::json::array json_arr;
        for (const auto val : path) {
            json_arr.emplace_back(val);
        }
        bulgogi::set_json(res, {{"path", json_arr}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_betweenness_centrality, "graph/betweenness_centrality") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    bool weighed = bulgogi::get_query_param(req, "weighed").value_or("0") == "1";
    if (!id_opt) {
        bulgogi::set_json(res, {{"error", "missing id"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    try {
        auto centrality = GraphManager::instance().betweenness_centrality(id, weighed);
        boost::json::array json_arr;
        for (const auto val : centrality) {
            json_arr.emplace_back(val);
        }
        bulgogi::set_json(res, {{"centrality", json_arr}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_get_from, "graph/get_from") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    auto node_opt = bulgogi::get_query_param(req, "node");
    if (!id_opt || !node_opt) {
        bulgogi::set_json(res, {{"error", "missing id or node"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    uint32_t node = std::stoul(*node_opt);
    try {
        auto nodes = GraphManager::instance().get_from(id, node);
        boost::json::array json_arr;
        for (const auto val : nodes) {
            json_arr.emplace_back(val);
        }
        bulgogi::set_json(res, {{"nodes", json_arr}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_get_to, "graph/get_to") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    auto node_opt = bulgogi::get_query_param(req, "node");
    if (!id_opt || !node_opt) {
        bulgogi::set_json(res, {{"error", "missing id or node"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    uint32_t node = std::stoul(*node_opt);
    try {
        auto nodes = GraphManager::instance().get_to(id, node);
        boost::json::array json_arr;
        for (const auto val : nodes) {
            json_arr.emplace_back(val);
        }
        bulgogi::set_json(res, {{"nodes", json_arr}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_get_neighbours, "graph/get_neighbours") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    auto node_opt = bulgogi::get_query_param(req, "node");
    bool bi = bulgogi::get_query_param(req, "directed").value_or("1") == "1";
    if (!id_opt || !node_opt) {
        bulgogi::set_json(res, {{"error", "missing id or node"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    uint32_t node = std::stoul(*node_opt);
    try {
        auto nodes = GraphManager::instance().get_neighbours(id, node, bi);
        boost::json::array json_arr;
        for (const auto val : nodes) {
            json_arr.emplace_back(val);
        }
        bulgogi::set_json(res, {{"nodes", json_arr}});
    } catch (const std::exception &e) {
        bulgogi::set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW_URLS(graph_destroy, "graph/destroy") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::delete_, res)) return;
    auto id_opt = bulgogi::get_query_param(req, "id");
    if (!id_opt) {
        bulgogi::set_json(res, {{"error", "missing id"}}, 400);
        return;
    }
    uint64_t id = std::stoull(*id_opt);
    bool ok = GraphManager::instance().destroy(id);
    bulgogi::set_json(res, {{"deleted", ok}});
}

REGISTER_VIEW_URLS(graph_list_ids, "graph/list_ids") {
    if (!bulgogi::check_method(req, bulgogi::http::verb::get, res)) return;
    auto ids = GraphManager::instance().list_ids();
    boost::json::array json_arr;
    for (const auto val : ids) {
        json_arr.emplace_back(val);
    }
    bulgogi::set_json(res, {{"ids", json_arr}});
}
