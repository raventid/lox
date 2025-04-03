.PHONY: ga repl cbuild clox jbuild jlox

build: jbuild cbuild
clean:
	rm -rf target/

jbuild:
	javac -d target jlox/*.java
cbuild:
	mkdir -p target
	gcc clox/main.c -I clox -Wall -Wextra -g -o target/clox

genast:
	mkdir -p target
	javac -d target tool/GenerateAst.java
	cd target && java tool/GenerateAst ../jlox

jlox: jbuild
	cd target && java jlox/Main $(if $(f),../examples/$(f).lox,)
clox: cbuild
	./target/clox

