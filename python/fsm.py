import graphviz
from dataclasses import dataclass, field
from typing import Any, Dict, List, Literal, Set, Tuple
from collections import defaultdict

EdgeMap = Dict[Tuple[int, int], Set[str]]

@dataclass
class FSM:
    label: List[str]
    type_: Literal["DFA"] | Literal["NFA"] = "DFA"

    def __post_init__(self):
        self.nodes: Set[int] = set()
        self.edges: EdgeMap  = defaultdict(set)
        self.final: Set[int] = set()
        self.start: Set[int] = set()
        self._visited: Set[int] = set()
        self._name_map: None | Dict[int, str] = None

    def _add_edge(self, start: int, end: int, label: str):
        self.nodes.add(start)
        self.nodes.add(end)
        self.edges[(start, end)].add(label)

    def _check_input(self, pairs: List[Tuple[int, str] | int]) -> List[Tuple[int, str]]:
        result: List[Tuple[int, str]] = []
        for i, elem in enumerate(pairs):
            if isinstance(elem, int):
                assert self.type_ == "DFA", "In NFA, the input should be a tuple"
                end = elem
                label = self.label[i]
            else:
                end, label = elem
                assert label in self.label, "Invalid label"
            result.append((end, label))
        return result

    def add_edges(self, src: int, *pairs: Tuple[int, str] | int,
                  start: bool = False, final: bool = False):
        assert src not in self._visited, "Node already defined"
        self._visited.add(src)

        for end, label in self._check_input(list(pairs)):
            self._add_edge(src, end, label)
        if final:
            self.final.add(src)
        if start:
            self.start.add(src)

    def name(self, index: int) -> str:
        if self._name_map is None:
            return f"q<sub>{index}</sub>"
        else:
            return self._name_map[index]

    def _check_edge_node_defined(self):
        for start, end in self.edges.keys():
            if start not in self.nodes or end not in self.nodes:
                raise ValueError("Undefined node")

    def _check_rank_group(self, rank_group: List[List[int]] | None):
        if rank_group is None:
            return
        assert sum(len(r) for r in rank_group) == len(self.nodes), "Invalid rank group"
        s = sorted([x for r in rank_group for x in r])
        assert s == list(range(len(s))), "Invalid rank group"

    def _merge_edges(self) -> EdgeMap:
        # for edge (a, b, c) and (b, a, c), merge them into (a, b, c)
        assert self.type_ == "DFA", "Merge double edges now only works for DFA"
        new_edges: EdgeMap = {}
        for start, end in list(self.edges.keys()):
            if (end, start) in self.edges and start != end and \
                self.edges[(start, end)] == self.edges[(end, start)]:
                label = self.edges[(start, end)]
                del self.edges[(end, start)]
                del self.edges[(start, end)]
                new_edges[(start, end)] = label
        return new_edges

    def _check_start(self) -> int:
        assert len(self.start) <= 1, "Multiple start states not supported"
        if len(self.start) == 0:
            assert 0 in self.nodes, "Start state not defined"
            return 0
        return list(self.start)[0]

    def dump_to(
        self, name: str, dir: str, *_unused,
        rank_group: List[List[int]] | None = None,
        node_attr_map: Dict[int, Dict[str, Any]] = {},
        edge_attr_map: Dict[Tuple[int, int], Dict[str, Any]] = {},
        fsm_attr_map: Dict[str, Any] = {},
        do_merge_double: bool | None = False
    ):
        assert len(_unused) == 0, "Invalid argument list"
        if do_merge_double is None:
            do_merge_double = self.type_ == "DFA"

        self._check_edge_node_defined()
        self._check_rank_group(rank_group)
        double_edges = self._merge_edges() if do_merge_double else {}

        dir = dir.split("/")[-1].split(".")[0]
        fsm = graphviz.Digraph(format='pdf')

        def _comb(*maps: Dict[str, Any]) -> Dict[str, Any]:
            result = {}
            for m in maps:
                result.update(m)
            return result

        def _insert_start(s: graphviz.Digraph):
            start = self._check_start()
            s.node("", ordering="out", shape="point", style="invis")
            s.edge("", str(start), ordering="out")

        def _insert_node(s: graphviz.Digraph, id_list: List[int]):
            for id in id_list:
                attrs = _comb({
                    "ordering": "out",
                    "shape": "doublecircle" if id in self.final else "circle"
                }, node_attr_map.get(id, {}))
                s.node(str(id), f"<{self.name(id)}>", **attrs)

        _insert_start(fsm)
        if rank_group is None:
            _insert_node(fsm, sorted(list(self.nodes)))
        else:
            for group in rank_group:
                subgraph = fsm.subgraph()
                assert len(group) > 0 and subgraph is not None
                with subgraph as s:
                    s.attr(rank="same")
                    _insert_node(s, group)

        for (start, end), labels in self.edges.items():
            attrs = _comb({"ordering": "out"}, edge_attr_map.get((start, end), {}))
            for label in labels:
                fsm.edge(str(start), str(end), label, **attrs)

        for (start, end), label in double_edges.items():
            attrs = _comb(
                {"ordering": "out", "dir": "both"},
                edge_attr_map.get((start, end), {}),
                edge_attr_map.get((end, start), {}),
            )
            for label in label:
                fsm.edge(str(start), str(end), label, **attrs)

        fsm_map = {"ordering": "out", "rankdir": "LR"}
        fsm_map.update(fsm_attr_map)
        fsm.attr(**fsm_map)
        fsm.render(name, directory=f"generated/{dir}", cleanup=True)

    def complement(self) -> "FSM":
        w = FSM(label=self.label)
        w.nodes = self.nodes
        w.edges = self.edges
        w.start = self.start
        w.final = w.nodes - self.final
        return w

    def to_dfa(self) -> "FSM":
        assert self.type_ == "NFA", "Only NFA can be converted to DFA"
        assert sorted(list(self.nodes)) == list(range(len(self.nodes))), \
            "Currently, we only support node index from 0 to n-1"

        # use powerset construction
        def _make_powerset_name_map() -> Dict[int, str]:
            x = len(self.nodes)
            mapping: Dict[int, str] = {}
            for i in range(0, 1 << x):
                name_list = [self.name(j) for j in range(x) if i & (1 << j)]
                mapping[i] = ', '.join(name_list)
                mapping[i] = "{" + mapping[i] + "}"
            return mapping

        def _make_edge_traverse() -> Dict[str, Dict[int, List[int]]]:
            result: Dict[str, Dict[int, List[int]]] = defaultdict(lambda: defaultdict(list))
            for (start, end), labels in self.edges.items():
                for label in labels:
                    assert label in self.label, "Invalid label"
                    result[label][start].append(end)
            return result

        def _convert2list(node: int) -> List[int]:
            return [i for i in range(len(self.nodes)) if node & (1 << i)]

        new_worker = FSM(label=self.label)
        new_worker.type_ = "DFA"
        new_worker._name_map = _make_powerset_name_map()
        new_worker.start = {1 << self._check_start()}

        queue: List[int] = [list(new_worker.start)[0]]
        visit: Set[int] = set()
        edge_traverse = _make_edge_traverse()
        final_flag = sum(1 << i for i in range(len(self.nodes)) if i in self.final)

        while len(queue) > 0:
            cur_state = queue.pop(0)
            if cur_state in visit:
                continue
            visit.add(cur_state)
            is_final = (cur_state & final_flag) != 0

            dests: List[Tuple[int, str]] = []
            for label in self.label:
                new_state = 0
                for node in _convert2list(cur_state):
                    for end in edge_traverse[label][node]:
                        new_state |= 1 << end
                if new_state != 0:
                    dests.append((new_state, label))
            
            new_worker.add_edges(cur_state, *dests, final=is_final)
            for end, _ in dests:
                if end not in visit:
                    queue.append(end)

        return new_worker
