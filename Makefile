.PHONY: ga repl cbuild clox jbuild jlox

build: jbuild cbuild
clean:
	rm -rf target/

jbuild:
	javac -d target jlox/*.java
cbuild:
	mkdir -p target
	gcc clox/*.c -I clox -Wall -Wextra -g -o target/clox

mca: /opt/homebrew/Cellar/llvm/20.1.3/bin/llvm-mca target/clox.s -skip-unsupported-instructions=parse-failure

genast:
	mkdir -p target
	javac -d target tool/GenerateAst.java
	cd target && java tool/GenerateAst ../jlox

jlox: jbuild
	cd target && java jlox/Main $(if $(f),../examples/$(f).lox,)
clox: cbuild
	./target/clox $(if $(f),examples/$(f).lox,)

