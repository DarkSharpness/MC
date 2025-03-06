from fsm import FSM

def p1():
    fsm = FSM.compile("AB* | CD^")

p1()
