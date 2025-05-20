
<div align="center">
  <img src="https://raw.githubusercontent.com/bulgogi-framework/.github/main/res/img/Bulgogi.svg" alt="bulgogi logo" style="max-width: 100%; max-height: 200px;">
</div>

<h1 align="center">Graph Server Project</h1>

<p align="center">
  <b>A high-performance containerized graph computation service in modern C++20, backed by Clang and powered by bulgogi.</b><br/>
  <em>Includes Python SDK for easy interaction and automation.</em>
</p>

---

## 🧠 Features

- Implemented in **C++20** with **LLVM/Clang** and `-march=native` optimizations
- Uses a custom **adjacency matrix** for SIMD-friendly memory layout
- Graph algorithms supported:
  - Degree, degree statistics
  - Triangle counting
  - Shortest paths (BFS & Dijkstra)
  - Betweenness centrality
  - Isolated node detection
- HTTP interface via **[bulgogi](https://github.com/bulgogi-framework/bulgogi)** — a lightweight Boost.Beast wrapper
- Includes Python SDK with automatic cleanup and alias support
- Containerized with **Docker** for reproducibility and testing

---

## 🚀 Quick Start

### 1. Build the Docker image

```bash
docker build -t graph-server -f docker/Dockerfile .
````

### 2. Run the server

```bash
docker run -p 8080:8080 graph-server
```

### 3. Use the Python client

> Requires Python 3.10+. All Python client files are in the `python/` directory.

```python
from client import Graph, AliasGraph

g = Graph(size=100, bi=True, weighed=True)
g.add_edge(0, 1, weight=3)
print(g.stats())
```

### 4. Load graph from text file with aliases

```python
g = AliasGraph("graph.txt")
print(g.stats())
print(g.centrality())
```

---

## 📑 API

The full HTTP API is documented in [`docs/api.md`](docs/api.md).
All endpoints support JSON input/output and `curl`-friendly queries.

---

## 📦 Dependencies

### Development (C++20 build only)

- C++20 compiler (Clang ≥ 13 preferred)
- [Boost](https://www.boost.org) 1.80+ with JSON module (Docker uses 1.88.0)
- [JH-Toolkit](https://github.com/JeongHan-Bae/JH-Toolkit) `1.3.x-LTS`
- CMake ≥ 3.27 (Docker builds from source)
- Docker (for consistent build & deployment)

### Runtime (Python client)

- Python 3.10+
- `requests` library (automatically used by the Python SDK)

---

## 🛠 Design Highlights

* SIMD-friendly layout: adjacency matrix avoids pointer chains
* `ColIter` enables efficient column traversal for triangle & centrality calc
* `GraphManager` supports graph ID registration and locking
* Python-side cleanup uses `atexit` + `__del__` to reduce memory leaks

---

## 📂 Folder Structure

```
.
├── docker/           # Dockerfile and build config
├── Application/      # C++ GraphManager and core logic
├── Algo/             # C++ Graph algorithms
├── Web/              # bulgogi-based HTTP server
├── python/           # Python SDK (GraphClient, AliasGraph)
└── docs/             # API documentation
```

---

## 📊 Attribution

* Demo graph data sourced from [Stanford SNAP](https://snap.stanford.edu/data/)
* HTTP framework forked and customized from [bulgogi](https://github.com/bulgogi-framework/bulgogi)

---

## 🧑‍💻 Author

Developed and maintained by **JH / JeongHan-Bae**

* GitHub: [@JeongHan-Bae](https://github.com/JeongHan-Bae)
* Framework: [bulgogi-framework](https://github.com/bulgogi-framework/bulgogi)
