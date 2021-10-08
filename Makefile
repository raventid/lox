.PHONY: loxi loxbi ga

loxi:
	javac loxi/Main.java && java loxi/Main
loxbi:
	javac loxbi/Main.java && java loxbi/Main
ga:
	javac tool/GenerateAst.java && java tool/GenerateAst $(dir)
