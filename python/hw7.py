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

p1_a()
p1_b()
p1_c()
p2_b()
