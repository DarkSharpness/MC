import graphviz
from dataclasses import dataclass, field
from typing import Any, Dict, List, Literal, Set, Tuple
from collections import defaultdict

EdgeMap = Dict[Tuple[int, int], Set[str]]

@dataclass
class ASTOp():
    pass

@dataclass
class LitOp(ASTOp):
    val: str

@dataclass
class CatOp(ASTOp):
    lst: List[ASTOp]

@dataclass
class AddOp(ASTOp):
    pass

@dataclass
class MulOp(ASTOp):
    pass

@dataclass
class InfOp(ASTOp):
    pass

class BaseNode():
    pass

@dataclass
class LitNode(BaseNode):
    val: str
    def __repr__(self) -> str:
        return self.val

@dataclass
class CatNode(BaseNode):
    "e.g. ABCDEF"
    lst: List[BaseNode]
    def __repr__(self) -> str:
        return "(" + "".join(str(x) for x in self.lst) + ")"

@dataclass
class AddNode(BaseNode):
    "e.g. A | B | C"
    lst: List[BaseNode]
    def __repr__(self) -> str:
        return "(" + " | ".join(str(x) for x in self.lst) + ")"

@dataclass
class MulNode(BaseNode):
    "e.g.: A*"
    sub: BaseNode
    def __repr__(self) -> str:
        return f"{self.sub}*"

@dataclass
class InfNode(BaseNode):
    "e.g.: A^ (same as A^w)"
    sub: BaseNode
    def __repr__(self) -> str:
        return f"{self.sub}^"

def _parase_nba_grammar(rule: str) -> Tuple[BaseNode, bool]:
    # contains: <alnum>, (, ), <space>, *, +, |, ^
    # <alnum> ::= [a-zA-Z0-9]
    # <space> ::= " "
    # <binary op> ::= +
    # <unary op> ::= *, ^

    def _lexing(rule: str) -> CatOp:
        _op_map: Dict[str, type[ASTOp]] = {
            "*": MulOp,
            "^": InfOp,
            "+": AddOp,
            "|": AddOp, # alias
        }
        stack: List[List[ASTOp]] = []
        top: List[ASTOp] = []
        for c in rule:
            if c.isalnum():
                top.append(LitOp(c))
            elif c in "*+|^":
                top.append(_op_map[c]())
            elif c == "(":
                stack.append(top)
                top = []
            elif c == ")":
                sub = CatOp(top)
                top = stack.pop()
                top.append(sub)
            elif c.isspace():
                pass
            else:
                raise ValueError(f"Invalid character {c} (ASCII: {ord(c)})")
        assert len(stack) == 0, "Unmatched parenthesis"
        return CatOp(top)

    def _semantic_check(lst: List[ASTOp]):
        # all Inf | Mul must have lhs node.
        last: ASTOp = ASTOp()
        for op in lst:
            if isinstance(op, InfOp | MulOp):
                assert isinstance(last, LitOp | CatOp), "Invalid node"
            elif isinstance(op, AddOp):
                assert not isinstance(last, AddOp), "Invalid node"
            last = op
        # all Add must have rhs node
        last = ASTOp()
        for op in reversed(lst):
            if isinstance(op, AddOp):
                assert isinstance(last, LitOp | CatOp), "Invalid node"
            last = op

    def _parse_node(node: CatOp) -> BaseNode:
        _semantic_check(node.lst)
        # 1. apply all the * and ^ operator to its left node
        new_op_lst: List[BaseNode | None] = []
        for op in node.lst:
            if isinstance(op, MulOp):
                assert len(new_op_lst) > 0 and new_op_lst[-1] is not None, \
                    "This should not happen, as should fail in semantic check"
                new_op_lst[-1] = MulNode(new_op_lst[-1])
            elif isinstance(op, InfOp):
                assert len(new_op_lst) > 0 and new_op_lst[-1] is not None, \
                    "This should not happen, as should fail in semantic check"
                new_op_lst[-1] = InfNode(new_op_lst[-1])
            elif isinstance(op, CatOp):
                new_op_lst.append(_parse_node(op))
            elif isinstance(op, LitOp):
                new_op_lst.append(LitNode(op.val))
            elif isinstance(op, AddOp):
                new_op_lst.append(None)
            else:
                assert False, "This should not happen"

        # 2. apply all the + operator (which is None) in the list
        add_list: List[BaseNode] = []
        cur_list: List[BaseNode] = []
        for op in new_op_lst:
            if op is not None:
                cur_list.append(op)
            else:
                add_list.append(CatNode(cur_list))
                cur_list = []
        assert len(cur_list) > 0, "Invalid node"
        add_list.append(CatNode(cur_list))
        if len(add_list) == 1:
            return add_list[0]
        return AddNode(add_list)

    def _post_check(node: BaseNode) -> bool:
        "check whether there is infinity node in the tree"
        if isinstance(node, InfNode):
            return True
        elif isinstance(node, MulNode):
            assert not _post_check(node.sub), \
                "Sub node of MulNode should not be infinite" \
                f"\n {node = }"
        elif isinstance(node, CatNode):
            assert len(node.lst) > 0, "Invalid node"
            result = [_post_check(x) for x in node.lst]
            assert not any(result[:-1]), \
                "Only the last node in CatNode can be infinite" \
                f"\n {node = }, {result = }"
            return result[-1]
        elif isinstance(node, AddNode):
            assert len(node.lst) > 0, "Invalid node"
            cnt = sum([_post_check(x) for x in node.lst]) 
            assert cnt in [0, len(node.lst)], \
                "Either all or none of the sub node should be infinite:" \
                f"\n {node = }, {cnt = }"
            return cnt > 0
        elif isinstance(node, LitNode):
            pass
        else:
            assert False, "This should not happen"
        return False

    result = _parse_node(_lexing(rule))
    return result, _post_check(result)

def _collect_symbol(tree: BaseNode) -> Set[str]:
    if isinstance(tree, LitNode):
        return {tree.val}
    elif isinstance(tree, CatNode):
        return set().union(*[_collect_symbol(x) for x in tree.lst])
    elif isinstance(tree, AddNode):
        return set().union(*[_collect_symbol(x) for x in tree.lst])
    elif isinstance(tree, MulNode):
        return _collect_symbol(tree.sub)
    elif isinstance(tree, InfNode):
        return _collect_symbol(tree.sub)
    else:
        assert False, "This should not happen"



@dataclass
class _PartialNode:
    @staticmethod
    def _init_dict():
        return defaultdict(set)
    iedge: Dict[str, Set["_PartialNode"]] = field(default_factory=_init_dict)
    oedge: Dict[str, Set["_PartialNode"]] = field(default_factory=_init_dict)
    sedge: Set[str] = field(default_factory=set)

def _partial_connect(a: _PartialNode, b: _PartialNode, label: str):
    assert label != "", "Empty label is not allowed"
    if a is b:
        a.sedge.add(label)
    else:
        a.oedge[label].add(b)
        b.iedge[label].add(a)

def _partial_replace(a: _PartialNode, b: _PartialNode):
    "Replace all the edges that points to b to a"
    "Note that a and b must be disjoint before calling this function"
    assert a is not b, "Cannot merge self node"

    a.sedge.update(b.sedge)
    for label, nodes in b.iedge.items():
        for node in nodes:
            node.oedge[label].remove(b)
            node.oedge[label].add(a)
        a.iedge[label].update(nodes)

    for label, nodes in b.oedge.items():
        for node in nodes:
            node.iedge[label].remove(b)
            node.iedge[label].add(a)
        a.oedge[label].update(nodes)

    b.sedge = set()
    b.iedge = defaultdict(set)

@dataclass
class _PartialFSM:
    start: _PartialNode = field(default_factory=_PartialNode)
    final: _PartialNode = field(default_factory=_PartialNode)

    @staticmethod
    def _cat(lhs: "_PartialFSM", rhs: "_PartialFSM") -> "_PartialFSM":
        _partial_replace(lhs.final, rhs.start)
        if rhs.final == rhs.start:
            rhs.final = lhs.final
        return _PartialFSM(start=lhs.start, final=rhs.final)

    @staticmethod
    def from_tree(node: BaseNode) -> "_PartialFSM":
        raise NotImplementedError

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

    @staticmethod
    def _compile_infinite(tree: BaseNode) -> "FSM":
        fsm = FSM(label=sorted(list(_collect_symbol(tree))), type_="NFA")
        print(fsm.label)

        def visit(node: BaseNode):
            if isinstance(node, LitNode):
                return node.val
            elif isinstance(node, CatNode):
                for i in range(len(node.lst) - 1):
                    visit(node.lst[i])
                    visit(node.lst[i+1])
                return ""
            elif isinstance(node, AddNode):
                for sub in node.lst:
                    visit(sub)
                return ""
            elif isinstance(node, MulNode):
                visit(node.sub)
                return ""
            elif isinstance(node, InfNode):
                visit(node.sub)
                return ""
            else:
                assert False, "This should not happen"

        raise NotImplementedError

    @staticmethod
    def _compile_finite(tree: BaseNode) -> "FSM":
        raise NotImplementedError

    @staticmethod
    def compile(rule: str) -> "FSM":
        tree, is_inf = _parase_nba_grammar(rule)
        print(tree, is_inf)
        if is_inf:
            return FSM._compile_infinite(tree)
        else:
            return FSM._compile_finite(tree)

if __name__ == "__main__":
    try:
        fsm = FSM.compile("(AB + C)*((AA + B)C)^ + (A*C)^")
    except NotImplementedError:
        pass
