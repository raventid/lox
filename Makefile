.PHONY: loxi loxbi ga repl

loxi:
	javac loxi/Main.java && java loxi/Main
loxbi:
	javac loxbi/Main.java && java loxbi/Main
genexp:
	javac tool/GenerateAst.java && java tool/GenerateAst loxi

repl: loxi
