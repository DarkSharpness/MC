grammar LTL;

prog: formula EOF;

formula:
	Atomic										# Atomic
	| op = '(' only = formula ')'				# Paren
	| op = '!' only = formula					# Not
	| op = 'X' only = formula					# Next
	| op = 'G' only = formula					# Always
	| op = 'F' only = formula					# Eventually
	| lhs = formula op = 'U' rhs = formula		# Until
	| lhs = formula op = '->' rhs = formula		# Implication
	| lhs = formula op = '/\\' rhs = formula	# Conjunction
	| lhs = formula op = '\\/' rhs = formula	# Disjunction;

Atomic: [a-z]+;
WS: [ \t\r\n]+ -> skip;
