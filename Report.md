# Graph Server Project

## Tutorial Course 6 - Report

**Author:** Zhenghan PEI (JeongHan-Bae)  
**Languages:** C++20 (server) + Python 3.10+ (client)  
**Framework:** [bulgogi](https://github.com/bulgogi-framework/bulgogi)

---

## 🧭 Project Summary

This project implements a modern high-performance graph computation service written in **C++20**. The server provides a RESTful API for uploading and querying graphs, and is accessed via a lightweight **Python client**. The entire backend is containerized using **Docker**, with build-time optimizations via `clang` and `-march=native`.

GitHub Repository: [https://github.com/ISEP-Projects-JH/Graph](https://github.com/ISEP-Projects-JH/Graph)  
Pre-built Docker image (multi-arch, `amd64`/`arm64`) is included in the submission.  
All Python files are bundled and require only Python 3.10+ and `requests`.

> 📝 **Note:** For maximum compatibility across `amd64` and `arm64`, the submitted Docker image does **not** use `-march=native` at runtime. Instead, build targets fallback to platform-specific flags (e.g., `-march=armv8-a` or `-march=x86-64-v3`), as set in the project's `CMakeLists.txt`.

---

## Q1 - Graph File Parsing

- Implemented in `AliasGraph`, which supports `str ↔ int` aliasing for graph nodes.
- Reads edge list files in the form of:
```

nodeA nodeB weight

```
- Supports weighted undirected graphs.
- Internally converted to a compact **adjacency matrix**.

---

## Q2 - Degree Statistics

- `degree(node)` returns:
- in-degree
- out-degree
- `degree_stats()` computes:
- min, max, average degree
- edge density

### Example: ego-Facebook (4039 nodes, 170174 edges)

```

Stats (0.0106s): {'min': 0, 'max': 293, 'density': 0.0103, 'avg': 41.7}

```

- Graph is **sparse**, as expected in social networks.
- Isolated nodes are nodes with no incoming or outgoing edges.

---

## Q3 - Triangle Counting

- Undirected triangle counting uses **symmetry** and `std::count_if`.
- Directed version uses `ColIter` for cache-efficient column access.

### Output:

```

Triangle Count (0.5502s): 3,057,168

```

---

## Q4 - Shortest Path

- BFS used for unweighted graphs
- Dijkstra used for weighted graphs (via min-heap)
- Time complexity is linear or O(E log V) depending on graph type

API:  
`/graph/shortest_path?id=<id>&start=<node>&weighed=0|1`

Client returns full distance list or alias-decoded path.

---

## Q5 - Betweenness Centrality

- Implements **Brandes' algorithm** with Python-friendly JSON output
- Uses backward propagation (`stack`, `sigma`, `delta`)

### Result on ego-Facebook:

```

Centrality (43.2s): \[0.0, 6293.45, 9.71, ...]

````

- Output is a float value per node
- Result can be auto-decoded by alias in Python

---

## 🧰 Extra Features

- **Multi-arch Docker Build** (`buildx`) for both `amd64` and `arm64`
- Full server built with `-O3 -march=native -std=c++20`
- Modern API routing with [bulgogi](https://github.com/bulgogi-framework/bulgogi)
- Python client includes:
  - `Graph` class for ID-based graphs
  - `AliasGraph` for name-to-ID binding
  - Auto cleanup with `atexit` or `__del__`

---

## ✅ How to Use

### Run the prebuilt Docker server:

```bash
docker run -p 8080:8080 isep/graph-server:latest
````

### Python client (in `python/`):

```bash
from client import AliasGraph
g = AliasGraph("facebook.txt")
print(g.stats())
print(g.centrality())
```

No extra setup is needed except for `requests`.

---

## 🔖 Conclusion

This project exceeds the baseline of Tutorial Course 6:

* Implements all required algorithms
* Provides HTTP API + Python SDK
* Optimized C++20 implementation
* Clean separation of frontend/backend
* Multi-platform delivery via Docker

### Potential Extensions

* Dynamic graph updates via PATCH
* Export graph as image (via external graphviz)
* GPU acceleration (e.g., via CUDA, Thrust)

---

## 📎 Repository & Submission

* Source code and Dockerfile:
  [https://github.com/ISEP-Projects-JH/Graph](https://github.com/ISEP-Projects-JH/Graph)
* Runtime image submitted as Docker multi-platform build
* All Python files and `graph.txt` sample included
