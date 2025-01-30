.PHONY: loxi loxbi ga repl

build:
	javac loxi/Main.java

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
ex_forloop:
	javac loxi/Main.java && java loxi/Main examples/for_loop.lox


repl: loxi
