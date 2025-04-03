.PHONY: loxi loxbi ga repl cbuild clox

build:
	javac loxi/Main.java
cbuild:
	mkdir -p target
	gcc clox/main.c -I clox -Wall -Wextra -g -o target/clox

clean:
	rm -rf target/

loxi:
	javac loxi/Main.java && java loxi/Main
clox: cbuild
	./target/clox
genast:
	javac tool/GenerateAst.java && java tool/GenerateAst loxi

ex_closure:
	javac loxi/Main.java && java loxi/Main examples/closure.lox
ex_arith:
	javac loxi/Main.java && java loxi/Main examples/arithmetic.lox
ex_forloop:
	javac loxi/Main.java && java loxi/Main examples/for_loop.lox
ex_hi_fun:
	javac loxi/Main.java && java loxi/Main examples/say_hi_function.lox
ex_breaking_resolver:
	javac loxi/Main.java && java loxi/Main examples/breaking_resolver.lox


repl: loxi
