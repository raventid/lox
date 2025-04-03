.PHONY: ga repl cbuild clox jbuild jlox

jbuild:
	javac -d target jlox/*.java
cbuild:
	mkdir -p target
	gcc clox/main.c -I clox -Wall -Wextra -g -o target/clox

clean:
	rm -rf target/

genast:
	mkdir -p target
	javac -d target tool/GenerateAst.java
	cd target && java tool/GenerateAst ../jlox

jlox: jbuild
	cd target && java jlox/Main $(if $(f),../examples/$(f),)
clox: cbuild
	./target/clox

