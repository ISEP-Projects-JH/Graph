from typing import Dict, List, Tuple, Optional, Any, Union, Callable
from graph import Graph

class AliasGraph:
    def __init__(self, filepath: str):
        self.alias: Dict[str, int] = {}
        self.reverse_alias: Dict[int, str] = {}
        self.next_id: int = 0
        self.edges: List[Tuple[int, int, int]] = []

        self._load_file(filepath)
        self.graph = Graph(size=self.next_id, bi=True, weighed=True)
        self._build_graph()

    def _get_or_create_id(self, name: str) -> int:
        if name not in self.alias:
            self.alias[name] = self.next_id
            self.reverse_alias[self.next_id] = name
            self.next_id += 1
        return self.alias[name]

    def _load_file(self, filepath: str) -> None:
        with open(filepath, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) != 3:
                    continue
                u_str, v_str, w_str = parts
                u = self._get_or_create_id(u_str)
                v = self._get_or_create_id(v_str)
                weight = int(float(w_str))
                self.edges.append((u, v, weight))

    def _build_graph(self) -> None:
        edge_dicts = [{"u": u, "v": v, "weight": w} for u, v, w in self.edges]
        self.graph.batch_edges(edge_dicts)

    def get_id(self, name: str) -> Optional[int]:
        return self.alias.get(name)

    def get_name(self, node_id: int) -> Optional[str]:
        return self.reverse_alias.get(node_id)

    def _translate_input(self, x: Union[str, int]) -> int:
        return self.get_id(x) if isinstance(x, str) else x

    def _translate_output(self, result: Any) -> Any:
        if isinstance(result, dict):
            return {
                self.get_name(k) if isinstance(k, int) and k in self.reverse_alias else k:
                    self._translate_output(v)
                for k, v in result.items()
            }

        elif isinstance(result, list):
            # Case: shortest_path returns [int] → alias
            if all(isinstance(x, int) and x in self.reverse_alias for x in result):
                return [self.get_name(x) for x in result]

            # Case: centrality returns list[float] indexed by node_id
            if all(isinstance(x, (int, float)) for x in result):
                return {
                    self.get_name(idx): val
                    for idx, val in enumerate(result)
                    if idx in self.reverse_alias
                }

        return result

    def __getattr__(self, attr: str) -> Callable:
        base_method = getattr(self.graph, attr)

        def wrapper(*args, **kwargs):
            new_args = [self._translate_input(arg) for arg in args]
            new_kwargs = {k: self._translate_input(v) for k, v in kwargs.items()}
            result = base_method(*new_args, **new_kwargs)
            return self._translate_output(result)

        return wrapper
