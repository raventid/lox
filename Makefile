.PHONY: loxi loxbi

loxi:
	javac lox/Main.java && java lox/Main
loxbi:
	javac loxbi/Main.java && java loxbi/Main
