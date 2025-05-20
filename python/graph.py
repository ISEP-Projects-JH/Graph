import atexit
import logging

import requests
from typing import List, Dict, Optional, Final

host_url: str = "http://localhost:8080"  # Default host URL for the graph server

def set_host(host: str) -> None:
    """
    Set the host URL for the GraphClient.
    """
    global host_url
    host_url = host.rstrip("/")
    logging.debug(f"Host URL set to: {host_url}")

class GraphClient:
    def __init__(self, host: str = host_url):
        self.host = host.rstrip("/")

    def create(self, size: int) -> int:
        if size > 4096:
            raise ValueError("Graph size exceeds maximum allowed size of 4096.")
        resp = requests.post(f"{self.host}/graph/create", json={"size": size})
        resp.raise_for_status()
        return resp.json()["id"]

    def exists(self, graph_id: int) -> bool:
        resp = requests.get(f"{self.host}/graph/exists", params={"id": graph_id})
        resp.raise_for_status()
        return resp.json()["exists"]

    def destroy(self, graph_id: int) -> bool:
        resp = requests.delete(f"{self.host}/graph/destroy", params={"id": graph_id})
        resp.raise_for_status()
        return resp.json()["deleted"]

    def list_ids(self) -> List[int]:
        resp = requests.get(f"{self.host}/graph/list_ids")
        resp.raise_for_status()
        return resp.json()["ids"]

    def add_edge(self, graph_id: int, u: int, v: int, weight: int = 1, bi: bool = True) -> Dict:
        data = {"id": graph_id, "u": u, "v": v, "weight": weight, "bi": bi}
        resp = requests.post(f"{self.host}/graph/add-edge", json=data)
        resp.raise_for_status()
        return resp.json()

    def batch_edges(self, graph_id: int, lines: List[Dict], bi: bool = True) -> Dict:
        data = {"id": graph_id, "bi": bi, "lines": lines}
        resp = requests.post(f"{self.host}/graph/batch-edges", json=data)
        resp.raise_for_status()
        return resp.json()

    def degree(self, graph_id: int, node: int, directed: bool = False) -> Dict:
        params = {"id": graph_id, "node": node, "directed": int(directed)}
        resp = requests.get(f"{self.host}/graph/degree", params=params)
        resp.raise_for_status()
        return resp.json()

    def degree_stats(self, graph_id: int, directed: bool = False) -> Dict:
        params = {"id": graph_id, "directed": int(directed)}
        resp = requests.get(f"{self.host}/graph/degree_stats", params=params)
        resp.raise_for_status()
        return resp.json()

    def isolated_nodes(self, graph_id: int, directed: bool = False) -> List[int]:
        params = {"id": graph_id, "directed": int(directed)}
        resp = requests.get(f"{self.host}/graph/isolated_nodes", params=params)
        resp.raise_for_status()
        return resp.json()["nodes"]

    def count_triangles(self, graph_id: int, directed: bool = False) -> int:
        params = {"id": graph_id, "directed": int(directed)}
        resp = requests.get(f"{self.host}/graph/count_triangles", params=params)
        resp.raise_for_status()
        return resp.json()["count"]

    def shortest_path(self, graph_id: int, start: int, weighed: bool = False) -> List[Optional[int]]:
        params = {"id": graph_id, "start": start, "weighed": int(weighed)}
        resp = requests.get(f"{self.host}/graph/shortest_path", params=params)
        resp.raise_for_status()
        return resp.json()["path"]

    def betweenness_centrality(self, graph_id: int, weighed: bool = False) -> Dict[int, float]:
        params = {"id": graph_id, "weighed": int(weighed)}
        resp = requests.get(f"{self.host}/graph/betweenness_centrality", params=params)
        resp.raise_for_status()
        return resp.json()["centrality"]

    def get_from(self, graph_id: int, node: int) -> List[int]:
        params = {"id": graph_id, "node": node}
        resp = requests.get(f"{self.host}/graph/get_from", params=params)
        resp.raise_for_status()
        return resp.json()["nodes"]

    def get_to(self, graph_id: int, node: int) -> List[int]:
        params = {"id": graph_id, "node": node}
        resp = requests.get(f"{self.host}/graph/get_to", params=params)
        resp.raise_for_status()
        return resp.json()["nodes"]

    def get_neighbours(self, graph_id: int, node: int, directed: bool = True) -> List[int]:
        params = {"id": graph_id, "node": node, "directed": int(directed)}
        resp = requests.get(f"{self.host}/graph/get_neighbours", params=params)
        resp.raise_for_status()
        return resp.json()["nodes"]

class Graph:
    def __init__(self, size: int, bi: bool = True, weighed: bool = False, client: Optional[GraphClient] = None):
        self.client = client if client else GraphClient()
        if not self.client:
            raise ValueError("GraphClient is not initialized.")
        self.id: Final[int] = self.client.create(size)
        self.bi: Final[bool] = bi
        self.weighed: Final[bool] = weighed
        atexit.register(self._cleanup)

    def add_edge(self, u: int, v: int, weight: int = 1) -> Dict:
        return self.client.add_edge(self.id, u, v, weight, bi=self.bi)

    def batch_edges(self, lines: List[Dict]) -> Dict:
        if not self.weighed:
            # no weight, set all weights to 1
            fixed_lines = [{"u": line["u"], "v": line["v"], "weight": 1} for line in lines]
        else:
            # weighed graph, ensure all edges have a weight field
            fixed_lines = []
            for line in lines:
                if "weight" not in line:
                    raise ValueError("Weighted graph requires 'weight' field for all edges.")
                fixed_lines.append(line)
        return self.client.batch_edges(self.id, fixed_lines, bi=self.bi)

    def degree(self, node: int) -> Dict:
        return self.client.degree(self.id, node, directed=not self.bi)

    def stats(self) -> Dict:
        return self.client.degree_stats(self.id, directed=not self.bi)

    def isolated(self) -> List[int]:
        return self.client.isolated_nodes(self.id, directed=not self.bi)

    def triangle_count(self) -> int:
        return self.client.count_triangles(self.id, directed=not self.bi)

    def shortest_path(self, start: int, unreachable_marker: Optional[int] = 2 ** 31 - 1) -> List[Optional[int]]:
        raw = self.client.shortest_path(self.id, start, weighed=self.weighed)
        return [None if v == unreachable_marker else v for v in raw]

    def centrality(self) -> Dict[int, float]:
        return self.client.betweenness_centrality(self.id, weighed=self.weighed)

    def out_nodes(self, node: int) -> List[int]:
        return self.client.get_from(self.id, node)

    def in_nodes(self, node: int) -> List[int]:
        return self.client.get_to(self.id, node)

    def neighbours(self, node: int) -> List[int]:
        return self.client.get_neighbours(self.id, node, directed=not self.bi)

    def _destroy(self) -> None:
        if self.client is None:
            return  # Already destroyed
        self.client.destroy(self.id)
        self.client = None

    def _cleanup(self):
        if not hasattr(self, "client") or not self.client:
            return  # No cleanup possible if client is missing or already destroyed
        try:
            self._destroy()
        except Exception as e:
            # Handle any other exceptions that may arise
            logging.error(f"Unexpected error during __del__: {str(e)}")
            pass

    def __del__(self):
        # Cleanup the graph when the object is deleted, avoiding server pressure
        if not hasattr(self, "client") or not self.client:
            return  # No cleanup possible if client is missing
        if not hasattr(self, "id"):
            return  # ID was never assigned
        try:
            self._destroy()  # Attempt to delete the resource from the server
        except Exception as e:
            # Handle any other exceptions that may arise
            logging.error(f"Unexpected error during __del__: {str(e)}")
            pass