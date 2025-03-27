from operator import not_
from typing import List
from fsm import FSM

def p1_a():
    a = "a"
    not_a = "¬a"
    a_not_b = "a∧¬b"
    not_a_not_b = "¬a∧¬b"
    labels = [a, not_a, a_not_b, not_a_not_b]
    fsm = FSM(label=labels, type_="NFA")
    fsm.add_edges(0, (0, a), (1, not_a), start=True, final=True)
    fsm.add_edges(1, (0, a_not_b), (1, not_a_not_b), final=True)
    fsm.dump_to(name="p1_a", dir=__file__)

def p1_b():
    a = "a"
    b = "b"
    not_b = "¬b"
    true = "true"
    labels = [a, b, not_b, true]
    fsm = FSM(label=labels, type_="NFA")
    fsm.add_edges(0, (0, not_b), (1, b), (2, a), start=True, final=True)
    fsm.add_edges(1, (0, not_b), (1, b), (2, a))
    fsm.add_edges(2, (2, true), final=True)
    fsm.dump_to(name="p1_b", dir=__file__)

def p1_c():
    a = "a"
    b = "b"
    true = "true"
    labels = [a, b, true]
    fsm = FSM(label=labels, type_="NFA")
    fsm.add_edges(0, (1, true), start=True)
    fsm.add_edges(1, (2, true))
    fsm.add_edges(2, (3, a), (4, true))
    fsm.add_edges(3, (3, true), final=True)
    fsm.add_edges(4, (4, true), (5, b))
    fsm.add_edges(5, (5, b), final=True)
    fsm.dump_to(name="p1_c", dir=__file__)

def p2_b():
    a = "a"
    a_and_b = "a∧b"
    true = "true"
    labels = [a, a_and_b, true]
    fsm = FSM(label=labels, type_="NFA")
    fsm.add_edges(0, (0, true), (1, true), start=True)
    fsm.add_edges(1, (1, a), (2, a_and_b))
    fsm.add_edges(2, (1, a), (2, a_and_b), final=True)
    fsm.dump_to(name="p2_b", dir=__file__)

def p3_d():
    a_and_not_b = "a∧¬b"
    a_and_b = "a∧b"
    not_b = "¬b"
    not_a_and_b = "¬a∧b"
    true = "true"
    labels = [a_and_not_b, a_and_b, not_b, not_a_and_b, true]
    fsm = FSM(label=labels, type_="NFA")
    fsm.add_edges(0, (0, true), (1, a_and_not_b), start=True)
    fsm.add_edges(1, (1, not_b), (2, a_and_b), (3, not_a_and_b), final=True)
    fsm.add_edges(2, (2, true))
    fsm.add_edges(3, (3, true), final=True)
    fsm.dump_to(name="p3_d", dir=__file__)

def p3_e():
    non = ""
    labels = [non]
    fsm = FSM(label=labels, type_="NFA")
    fsm.add_edges(0, (1, non), (3, non), (4, non), start=True)
    fsm.add_edges(1, (2, non))
    fsm.add_edges(3, (0, non))
    fsm.add_edges(2, (3, non), (2, non), (4, non))
    fsm.add_edges(4, (5, non), final=True)
    fsm.add_edges(5, (4, non), (7, non), final=True)
    fsm.add_edges(6, (7, non), (9, non), final=True)
    fsm.add_edges(7, (8, non), final=True)
    fsm.add_edges(9, (6, non), final=True)
    fsm.add_edges(8, (9, non), (8, non), final=True)
    _name_map = {
        0: "(s{0}0{1}, q{0}0{1})",
        1: "(s{0}1{1}, q{0}0{1})",
        2: "(s{0}2{1}, q{0}0{1})",
        3: "(s{0}3{1}, q{0}0{1})",
        4: "(s{0}3{1}, q{0}1{1})",
        5: "(s{0}0{1}, q{0}1{1})",
        6: "(s{0}0{1}, q{0}3{1})",
        7: "(s{0}1{1}, q{0}3{1})",
        8: "(s{0}2{1}, q{0}3{1})",
        9: "(s{0}3{1}, q{0}3{1})"
    }

    _new_map = {}
    for k, v  in _name_map.items():
        # surround number with <sub> </sub> tag
        _new_map[k] = v.format("<sub>", "</sub>")
    fsm._name_map = _new_map

    fsm.dump_to(
        name="p3_e",
        dir=__file__,
        rank_group=[
            [0, 1], [2, 3], [4, 5], [6, 7], [8, 9]
        ],
        edge_attr_map={
            (2, 2): {"headport": "nw", "tailport": "sw"},
            (8, 8): {"headport": "nw", "tailport": "sw"},
        },
        fsm_attr_map={
            "rankdir": "NW"
        },
    )

p1_a()
p1_b()
p1_c()
p2_b()
p3_d()
p3_e()
