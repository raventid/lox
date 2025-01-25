.PHONY: loxi loxbi ga repl

loxi:
	javac loxi/Main.java && java loxi/Main
loxbi:
	javac loxbi/Main.java && java loxbi/Main
genast:
	javac tool/GenerateAst.java && java tool/GenerateAst loxi

ex_closure:
	javac loxi/Main.java && java loxi/Main examples/closure.lox
ex_arith:
	javac loxi/Main.java && java loxi/Main examples/arithmetic.lox


repl: loxi
