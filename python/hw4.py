from typing import List
from fsm import FSM

def p1():
    fsm = FSM(label="ABC", type_="NFA")

    fsm.add_edges(0, (0, "A"), (1, "C"), start=True)
    fsm.add_edges(1, (0, "A"), (1, "C"), final=True)

    fsm.add_edges(2, (2, "C"), (3, "A"), (4, "A"), (5, "B"), start=True)
    fsm.add_edges(3, (2, "B"))
    fsm.add_edges(4, (5, "A"))
    fsm.add_edges(5, (6, "C"))
    fsm.add_edges(6, (5, "B"), (4, "A"), final=True)

    fsm.dump_to(name="p1", dir=__file__)


def p3():
    fsm = FSM(label="AB", type_="NFA")

    fsm.add_edges(0, (1, "A"), start=True)
    fsm.add_edges(1, (0, "B"), (1, "B"), (2, "B"))
    fsm.add_edges(2, (0, "A"), final=True)

    fsm.dump_to(name="p3", dir=__file__)

def p4_a():
    not_w1 = "<not w<sub>1</sub>>"
    w1 = "<w<sub>1</sub>>"
    c1_and_not_w1 = "<c<sub>1</sub> and not w<sub>1</sub>>"
    not_c1_or_w1 = "<not c<sub>1</sub> or w<sub>1</sub>>"

    fsm = FSM(label=[
        not_w1, w1, c1_and_not_w1, not_c1_or_w1
    ], type_="NFA")

    fsm.add_edges(0, (0, not_w1), (1, w1), start=True, final=True)
    fsm.add_edges(1, (1, not_c1_or_w1), (0, c1_and_not_w1))
    fsm.dump_to(name="p4_a", dir=__file__)

    true = "true"
    not_c1 = "<not c1>"

    fsm = FSM(label=[true, w1, not_c1], type_="NFA")

    fsm.add_edges(0, (0, true), (1, w1), start=True)
    fsm.add_edges(1, (1, not_c1), final=True)
    fsm.dump_to(name="p4_b", dir=__file__)

def p4_b():
    fsm = FSM(label=[""], type_="NFA") # a normal graph
    q0 = "q<sub>0</sub> | "
    q1 = "q<sub>1</sub> | "

    fsm._name_map = {
        0o00: q0 + "nn1",
        0o01: q0 + "wn1",
        0o02: q0 + "nw1",
        0o03: q0 + "cn0",
        0o04: q0 + "ww1",
        0o05: q0 + "nc0",
        0o06: q0 + "cw0",
        0o07: q0 + "wc0",
        # q1 can't appear with c in the first position
        0o10: q1 + "wn1",
        0o11: q1 + "ww1",
        0o12: q1 + "wc1",
    }

    def make_edge(start: int, to: List[int]):
        fsm.add_edges(start, *[(to_, "") for to_ in to])

    make_edge(0o00, [0o01, 0o02, 0o10])
    make_edge(0o01, [0o03, 0o04, 0o11])
    make_edge(0o02, [0o04, 0o05, 0o11])
    make_edge(0o03, [0o00, 0o06])
    make_edge(0o04, [0o06, 0o07, 0o12])
    make_edge(0o05, [0o00, 0o07, 0o12])
    make_edge(0o06, [0o02])
    make_edge(0o07, [0o01, 0o10])

    make_edge(0o10, [0o11])
    make_edge(0o11, [0o12])
    make_edge(0o12, [0o10])

    fsm.dump_to(
        name="p4_c", dir=__file__,
        rank_group=[
            [0],
            [1, 2],
            [3, 4, 5],
            [6, 7],
            [8, 9, 10],
        ],
        fsm_attr_map={
            "rankdir": "NW"
        },
        edge_attr_map={
            (0o11, 0o12): {"headport": "s", "tailport": "s"}
        }
    )

p1()
p3()
p4_a()
p4_b()
