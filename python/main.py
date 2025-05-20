import os
from typing import Optional, List, Dict

from alias_graph import AliasGraph
from graph import Graph, GraphClient

MAX_NODE_ID = 4096
BATCH_SIZE = 500

def create_graph_from_all_edges(directory: str, client: Optional[GraphClient] = None) -> Optional[Graph]:
    edge_files = [f for f in os.listdir(directory) if f.endswith(".edges")]
    if not edge_files:
        print("[Error] No .edges files found.")
        return None

    # === First pass: find max node ID across all files ===
    max_node_id = 0
    for file in edge_files:
        with open(os.path.join(directory, file), 'r') as f:
            for line in f:
                if line.startswith("#") or not line.strip():
                    continue
                try:
                    u, v = map(int, line.strip().split())
                    max_node_id = max(max_node_id, u, v)
                except ValueError:
                    continue

    if max_node_id > MAX_NODE_ID:
        print(f"[Error] Graph size too large ({max_node_id} > {MAX_NODE_ID})")
        return None

    # === Create the graph after size is known ===
    graph = Graph(size=max_node_id + 1, bi=False, weighed=False, client=client)

    # === Second pass: stream and batch-upload edges file by file ===
    total_edges = 0
    for file in edge_files:
        buffer: List[Dict] = []
        path = os.path.join(directory, file)
        with open(path, 'r') as f:
            for line in f:
                if line.startswith("#") or not line.strip():
                    continue
                try:
                    u, v = map(int, line.strip().split())
                    buffer.append({"u": u, "v": v})
                    if len(buffer) >= BATCH_SIZE:
                        graph.batch_edges(buffer)
                        total_edges += len(buffer)
                        buffer.clear()
                except ValueError:
                    continue
            if buffer:
                graph.batch_edges(buffer)
                total_edges += len(buffer)

    print(f"[Success] Created graph with {max_node_id + 1} nodes and {total_edges} edges.")
    return graph

# Example usage
if __name__ == "__main__":
    import time


    def timeit(label: str, func, *args, **kwargs):
        start = time.perf_counter()
        result = func(*args, **kwargs)
        elapsed = time.perf_counter() - start
        print(f"{label} ({elapsed:.4f}s): {result}")
        return result

    fb_directory = os.path.join(os.path.dirname(os.path.abspath(__file__)),"dataset/facebook")
    graph_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),"dataset/graph.txt")

    facebook_graph = create_graph_from_all_edges(fb_directory)
    timeit("Stats", facebook_graph.stats)
    timeit("Triangle Count", facebook_graph.triangle_count)
    timeit("Centrality", facebook_graph.centrality)

    g = AliasGraph(graph_path)
    print(g.get_id("A"))
    print(g.stats())
    print(g.shortest_path("A"))
    print(g.neighbours("A"))
    print(g.centrality())
    print(g.isolated())
