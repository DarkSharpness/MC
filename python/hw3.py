from fsm import FSM

def p1_a():
    w = FSM(label=["a", "b"])

    w.add_edges(0, (0, "a"), (1, "b"), start=True) # <>
    w.add_edges(1, (2, "a"), (1, "b")) # <b>
    w.add_edges(2, (0, "a"), (3, "b")) # <ba>
    w.add_edges(3, (4, "a"), (1, "b")) # <bab>
    w.add_edges(4, (4, "a"), (4, "b"), final=True) # <baba>

    w.dump_to(name="p1_a_0", dir=__file__)
    w.complement().dump_to(name="p1_a_1", dir=__file__)

def p1_b():
    w = FSM(label=["a", "b"])

    w.add_edges(0, 1, 2, start=True, final=True) # <>
    w.add_edges(1, 1, 2, final=True) # <a*>
    w.add_edges(2, 3, 2, final=True) # <a*b*>
    w.add_edges(3, 3, 3) # <invalid>

    w.dump_to(name="p1_b_0", dir=__file__)
    w.complement().dump_to(name="p1_b_1", dir=__file__)

def p2_c():
    w = FSM(label=["0", "1"])
    # same as p1_a, b -> 0 and a -> 1

    w.add_edges(0, 1, 0, start=True) # <>
    w.add_edges(1, 1, 2) # <0>
    w.add_edges(2, 3, 0) # <01>
    w.add_edges(3, 1, 4) # <010>
    w.add_edges(4, 4, 4, final=True) # <0101>

    w.dump_to(name="p2_c", dir=__file__)

def p2_l():
    w = FSM(label=["0", "1"])

    w.add_edges(0, 1, 2, final=True) # <even 0, zero 1>
    w.add_edges(1, 0, 3)             # <odd 0, zero 1>
    w.add_edges(2, 3, 4, final=True) # <even 0, one 1>
    w.add_edges(3, 2, 5)             # <odd 0, one 1>
    w.add_edges(4, 5, 6, final=True) # <even 0, two 1>
    w.add_edges(5, 4, 7, final=True) # <odd 0, two 1>
    w.add_edges(6, 7, 6, final=True) # <even 0, three 1>
    w.add_edges(7, 6, 7)             # <odd 0, three 1>

    w.dump_to(
        name="p2_l", dir=__file__,
        rank_group=[list(range(0, 8, 2)), list(range(1, 8, 2))],
        edge_attr_map={
            (6, 6): {"headport": "e", "tailport": "e"},
            (7, 7): {"headport": "e", "tailport": "e"}
        },
        fsm_attr_map={
            "rankdir": "NW"
        },
        do_merge_double=True
    )

def p3():
    w = FSM(label=["0", "1"], type_="NFA")

    w.add_edges(0, (0, "0"), (0, "1"), (1, "0"))
    w.add_edges(1, (2, "0"))
    w.add_edges(2, final=True)

    w.dump_to(name="p3", dir=__file__)

def p4():
    w = FSM(label=["0", "1"], type_="NFA")

    w.add_edges(0, (1, "0"), final=True)
    w.add_edges(1, (0, "1"), (2, "0"), (3, "1"))
    w.add_edges(2, (0, "1"))
    w.add_edges(3, (0, "0"))

    w.dump_to(
        name="p4_a", dir=__file__,
        rank_group=[[0, 1], [2, 3]],
        fsm_attr_map={"rankdir": "NW"},
    )

    w.to_dfa().dump_to(
        name="p4_b", dir=__file__,
    )

import random
random.seed(42)

p1_a()
p1_b()
p2_c()
p2_l()
p3()
p4()
