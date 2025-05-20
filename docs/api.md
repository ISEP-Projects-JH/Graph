# 📘 API Reference: Graph Server

This document lists all available routes in the Graph HTTP server and demonstrates how to use them via `curl`.

---

## 🔍 Metadata

* **Graph ID (`id`)** is assigned by the server upon graph creation.
* **Directionality (`bi`)** and **Weight flag (`weighed`)** are held by the client. The server does **not store** this metadata.
* Deletion must be handled explicitly by the caller (`/graph/destroy?id=...`); **mass deletion is not allowed** to prevent unintentional interruption.
* Server will clean up all graphs on shutdown.
* The `/` page is a minimal REPL-like demo, not suitable for production use. Consider using `Python` or `Node.js` for robust clients.

---

## 📍 `/ping`

* **Method:** `GET`
* **Description:** Health check

**Example:**

```bash
curl -X GET http://localhost:8080/ping
```

**Response:**

```json
{"status": "alive"}
```

---

## 📍 `/shutdown_server`

* **Method:** `POST`
* **Description:** Gracefully shuts down the server

**Example:**

```bash
curl -X POST http://localhost:8080/shutdown_server
```

**Response:**

```json
{"status": "server_shutdown_requested"}
```

---

## 📍 `/graph/create`

* **Method:** `POST`
* **Body:** `application/json`

```json
{ "size": 100 }
```

**Example:**

```bash
curl -X POST http://localhost:8080/graph/create \
  -H "Content-Type: application/json" \
  -d '{"size":100}'
```

**Response:**

```json
{ "id": 1 }
```

---

## 📍 `/graph/exists?id=<id>`

* **Method:** `GET`
* **Query Parameter:** `id`

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/exists?id=1"
```

**Response:**

```json
{ "exists": true }
```

---

## 📍 `/graph/add-edge`

* **Method:** `POST`
* **Body:** `application/json`

```json
{
  "id": 1,
  "u": 0,
  "v": 1,
  "weight": 10,
  "bi": true
}
```

**Example:**

```bash
curl -X POST http://localhost:8080/graph/add-edge \
  -H "Content-Type: application/json" \
  -d '{"id":1, "u":0, "v":1, "weight":10, "bi":true}'
```

---

## 📍 `/graph/batch-edges`

* **Method:** `POST`
* **Body:** `application/json`

```json
{
  "id": 1,
  "bi": false,
  "lines": [
    { "u": 0, "v": 2, "weight": 3 },
    { "u": 1, "v": 2, "weight": 5 }
  ]
}
```

**Example:**

```bash
curl -X POST http://localhost:8080/graph/batch-edges \
  -H "Content-Type: application/json" \
  -d '{"id":1,"bi":false,"lines":[{"u":0,"v":2,"weight":3},{"u":1,"v":2,"weight":5}]}'
```

---

## 📍 `/graph/degree?id=<id>&node=<node>&directed=1`

* **Method:** `GET`
* **Query Parameters:** `id`, `node`, `directed`

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/degree?id=1&node=0&directed=1"
```

**Response:**

```json
{ "in": 2, "out": 3 }
```

---

## 📍 `/graph/degree_stats?id=<id>&directed=1`

* **Method:** `GET`
* **Query Parameters:** `id`, `directed`

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/degree_stats?id=1&directed=1"
```

**Response:**

```json
{
  "min": 0,
  "max": 6,
  "density": 0.14,
  "avg": 2.3
}
```

---

## 📍 `/graph/isolated_nodes?id=<id>&directed=0`

* **Method:** `GET`
* **Query Parameters:** `id`, `directed`

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/isolated_nodes?id=1&directed=0"
```

**Response:**

```json
{ "nodes": [3, 5] }
```

---

## 📍 `/graph/count_triangles?id=<id>&directed=1`

* **Method:** `GET`
* **Query Parameters:** `id`, `directed`

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/count_triangles?id=1&directed=1"
```

**Response:**

```json
{ "count": 8 }
```

---

## 📍 `/graph/shortest_path?id=<id>&start=<node>&weighed=1`

* **Method:** `GET`
* **Query Parameters:** `id`, `start`, `weighed`

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/shortest_path?id=1&start=0&weighed=1"
```

**Response:**

```json
{ "path": [0, 2, 5, 7] }
```

---

## 📍 `/graph/betweenness_centrality?id=<id>&weighed=0`

* **Method:** `GET`
* **Query Parameters:** `id`, `weighed`

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/betweenness_centrality?id=1&weighed=0"
```

**Response:**

```json
{ "centrality": [0.1, 0.3, 0.0, 0.25] }
```

---



## 📍 `/graph/get_from?id=<id>&node=<node>`

* **Method:** `GET`

* **Query Parameters:**

    * `id`: Graph ID
    * `node`: Source node (0-based index)

* **Description:**
  Returns all nodes that the specified `node` has outgoing edges to — i.e., direct successors in a directed graph.

  This is equivalent to the "out-neighbors" of a node.

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/get_from?id=1&node=2"
```

**Response:**

```json
{ "nodes": [3, 4, 7] }
```

---

## 📍 `/graph/get_to?id=<id>&node=<node>`

* **Method:** `GET`

* **Query Parameters:**

    * `id`: Graph ID
    * `node`: Target node (0-based index)

* **Description:**
  Returns all nodes that have edges pointing to the specified `node` — i.e., direct predecessors in a directed graph.

  This is equivalent to the "in-neighbors" of a node.

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/get_to?id=1&node=2"
```

**Response:**

```json
{ "nodes": [0, 5] }
```

---

## 📍 `/graph/get_neighbours?id=<id>&node=<node>&directed=1`

* **Method:** `GET`

* **Query Parameters:**

    * `id`: Graph ID
    * `node`: Target node (0-based index)
    * `directed`: Optional; set to `1` (default) for directed graphs or `0` for undirected graphs

* **Description:**
  Returns the set of **mutual neighbors** of a node.

    * If `directed=1` (default):
      Only nodes with **both** incoming and outgoing connections to `node` are returned.
      This is equivalent to the **intersection of `/graph/get_from` and `/graph/get_to`**.

    * If `directed=0`:
      The server assumes the graph is **undirected** (edges are symmetric), and only checks `node → x` for neighbors.
      ⚠️ The server **does not enforce or verify symmetry** — clients are responsible for maintaining undirected structure.

**Example:**

```bash
curl -X GET "http://localhost:8080/graph/get_neighbours?id=1&node=2&directed=1"
```

**Response:**

```json
{ "nodes": [3, 4] }
```

---

### 🧠 Summary Table

| API                     | Meaning                                    | Mode (`directed`)                                                        |
|-------------------------|--------------------------------------------|--------------------------------------------------------------------------|
| `/graph/get_from`       | Nodes `node` can reach (`node → x`)        | Directional                                                              |
| `/graph/get_to`         | Nodes that can reach `node` (`x → node`)   | Directional                                                              |
| `/graph/get_neighbours` | Nodes where both `node → x` and `x → node` | Directed<br>(intersection of from + to)<br>Or unilateral if `directed=0` |

---

## 📍 `/graph/destroy?id=<id>`

* **Method:** `DELETE`
* **Query Parameter:** `id`

⚠️ Call this only when you're sure the graph is no longer used.

**Example:**

```bash
curl -X DELETE "http://localhost:8080/graph/destroy?id=1"
```

**Response:**

```json
{ "deleted": true }
```

---

## 📍 `/graph/list_ids`

* **Method:** `GET`
* **Description:** List all active graph IDs

**Example:**

```bash
curl -X GET http://localhost:8080/graph/list_ids
```

**Response:**

```json
{ "ids": [1, 2, 3] }
```
