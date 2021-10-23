package loxi;

import java.util.List;

import static loxi.TokenType.*;

// Grammar rules for the Parser. You can also find them in a ./Readme.org
// |------------+------------------------------------------------------------------------|
// | Name       | Expr                                                                   |
// |------------+------------------------------------------------------------------------|
// | expression | equality ;                                                             |
// |------------+------------------------------------------------------------------------|
// | equality   | comparison ( ( "!=" or "==" ) comparison )* ;                          |
// |------------+------------------------------------------------------------------------|
// | comparison | term ( ( ">" or ">=" or "<" or "<=" ) term )* ;                        |
// |------------+------------------------------------------------------------------------|
// | term       | factor ( ( "-" or "+" ) factor )* ;                                    |
// |------------+------------------------------------------------------------------------|
// | factor     | unary ( ( "/" or "*" ) unary )* ;                                      |
// |------------+------------------------------------------------------------------------|
// | unary      | ("!" or "-") unary or primary ;                                        |
// |------------+------------------------------------------------------------------------|
// | primary    | NUMBER or STRING or "true" or "false" or "nil" or "(" expression ")" ; |
// |------------+------------------------------------------------------------------------|

class Parser {
    private final List<Token> tokens;
    private int current = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    private Expr expression() {
       return equality();
    }

    private Expr equality() {
        Expr expr = comparison();

        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // // // // // // // // // // //
    // Different helper functions //
    // // // // // // // // // // //
    private boolean match(TokenType... types) {
        for (TokenType type : types) {
           if (check(type)) {
               advance(); // consume token
               return true;
           }
        }

        return false;
    }

    private boolean check(TokenType type) {
        if (isAtEnd()) { return false; }
        return peek().type == type;
    }

    private Token advance() {
        if (!isAtEnd()) { current++; }
        return previous();
    }

    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    private Token peek() {
        return tokens.get(current);
    }

    private Token previous() {
        return tokens.get(current - 1);
    }
}
