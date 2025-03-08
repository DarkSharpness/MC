from dataclasses import dataclass
from typing import List, Set, Tuple, Literal

ROOT_RULE="$"
Terminal = Literal[
    "a", "b", "c", "d", "e", "f", "g", "h", "i",
    "j", "k", "l", "m", "n", "o", "p", "q", "r",
    "s", "t", "u", "v", "w", "x", "y", "z"
]
NonTerminal = Literal[
    "A", "B", "C", "D", "E", "F", "G", "H", "I",
    "J", "K", "L", "M", "N", "O", "P", "Q", "R",
    "S", "T", "U", "V", "W", "X", "Y", "Z", "$"
]

def is_terminal(symbol: str) -> Terminal | None:
    return symbol if symbol.islower() else None # type: ignore

def is_nonterminal(symbol: str) -> NonTerminal | None:
    if symbol.isupper() or symbol == ROOT_RULE:
        return symbol # type: ignore
    return None

# Upper case -> non-terminal
# Lower case -> terminal

@dataclass(frozen=True)
class Grammar:
    rules: Tuple[Tuple[NonTerminal, str], ...]

    @staticmethod
    def parse(rule: str) -> "Grammar":
        results = []
        for line in rule.split("\n"):
            if not line.strip():
                continue
            lhs, rhs = line.replace(" ", "").split("::=")
            assert len(lhs) == 1 and is_nonterminal(lhs)
            results += [(lhs, rule) for rule in rhs.split("|")]
        return Grammar(tuple(results))

    def __getitem__(self, symbol: str) -> List[Tuple[NonTerminal, str]]:
        return [r for r in self.rules if r[0] == symbol]

@dataclass(frozen=True)
class State:
    name: NonTerminal
    expr: str
    pos: int = 0
    start: int = 0

    def terminated(self) -> bool:
        return self.pos >= len(self.expr)

    def symbol(self) -> str | None:
        return None if self.terminated() else self.expr[self.pos]

    def nonterminal_symbol(self) -> NonTerminal | None:
        if sym := self.symbol():
            return is_nonterminal(sym)
        return None

    def __next__(self) -> "State":
        return State(self.name, self.expr, self.pos + 1, self.start)

    def __repr__(self) -> str:
        return f'[{self.start}] {self.name} -> ' + \
            f'{self.expr[:self.pos]}•{self.expr[self.pos:]}'

def parse(grammar: Grammar, text: str):
    state_set: List[Set[State]] = [set() for _ in range(len(text) + 1)]
    state_set[0].add(State(*grammar[ROOT_RULE][0]))

    def _complete(state: State) -> List[State]:
        results: List[State] = []
        for r in state_set[state.start]:
            if state.name == r.symbol():
                results.append(next(r))
        return results

    def _predict(start: int, symbol: str) -> List[State]:
        return [
            State(*r, start=start)
            for r in grammar[symbol]
        ]

    def _scan(state: State, start: int, token: str):
        if state.symbol() == token:
            state_set[start + 1].add(next(state))

    END_SYMBOL = "."
    for i, input_symbol in enumerate(text + END_SYMBOL):
        queue = list(state_set[i])
        state_set[i] = set()
        while queue:
            state = queue.pop(0)
            if state in state_set[i]:
                continue
            state_set[i].add(state)
            if state.terminated():
                queue += _complete(state)
            elif nt := state.nonterminal_symbol():
                queue += _predict(i, nt)
            else:
                _scan(state, i, input_symbol)

    for i, state in enumerate(state_set):
        accept = any(s.name == ROOT_RULE and s.terminated() for s in state)
        print(f"State {i}: {text[:i]}•{text[i:]} {accept=}")
        print("\n".join(f"  {s}" for s in state))

grammar = Grammar.parse(
    """
    $ ::= A
    A ::= a A B | b | c
    B ::= A | a |
    """
)
print(grammar)
parse(grammar, "ab")
parse(grammar, "aac")
