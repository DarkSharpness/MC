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

p1()
p3()
