package loxi;

import java.util.List;
import java.util.ArrayList;

import static loxi.TokenType.*;

// Grammar rules for the Parser. You can also find them in a ./Readme.org
//
//                                   Expressions
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
//
//
//
//
//               Statements
// |-------------+---------------------------|
// | program     | statement* EOF ; exprStmt |
// |-------------+---------------------------|
// | declaration | varDecl or statement;     |
// |-------------+---------------------------|
// | statement   | exprStmt or printStmt ;   |
// |-------------+---------------------------|
// | exprStmt    | expression ";" ;          |
// |-------------+---------------------------|
// | printStmt   | "print" expression ";" ;  |
// |-------------+---------------------------|


class Parser {
    private final List<Token> tokens;
    private int current = 0;

    private static class ParseError extends RuntimeException {}

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    // Ignoring parse errors for now
    List<Stmt> parse() {
        List<Stmt> statements = new ArrayList<>();
        while (!isAtEnd()) {
            statements.add(statement());
        }

        return statements;
    }

    // // // // // // // // // // // // // // // //
    // Grammar rules functions: Statement level  //
    // // // // // // // // // // // // // // // //
    private Stmt statement() {
        if (match(PRINT)) { return printStatement(); }

        return expressionStatement();
    }

    private Stmt printStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(value);
    }

    private Stmt expressionStatement() {
        Expr expr = expression();
        consume(SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }


    // // // // // // // // // // // // // // // //
    // Grammar rules functions: Expression level //
    // // // // // // // // // // // // // // // //

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

    private Expr comparison() {
      Expr expr = term();

      while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
          Token operator = previous();
          Expr right = term();
          expr = new Expr.Binary(expr, operator, right);
      }

      return expr;
    }

    private Expr term() {
        Expr expr = factor();

        while (match(MINUS, PLUS)) {
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr factor() {
        Expr expr = unary();

        while (match(SLASH, STAR)) {
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();
            Expr expr = new Expr.Unary(operator, right);
            return expr;
        }

        return primary();
    }

    private Expr primary() {
        if (match(FALSE)) { return new Expr.Literal(false); }
        if (match(TRUE)) { return new Expr.Literal(true); }
        if (match(NIL)) { return new Expr.Literal(null); }

        if (match(NUMBER, STRING)) {
            return new Expr.Literal(previous().literal);
        }

        if (match(LEFT_PAREN)) {
            Expr expr = expression();
            consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }

        throw error(peek(), "Expect expression.");
    }

    // // // // // // // // // // //
    // Parser machinery functions //
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

    private Token consume(TokenType tokenType, String message) {
        if (check(tokenType)) { return advance(); }
        throw error(peek(), message);
    }

    private Token advance() {
        if (!isAtEnd()) { current++; }
        return previous();
    }

    private void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == SEMICOLON) { return; }

            switch (peek().type) {
            case CLASS:
            case FUN:
            case VAR:
            case FOR:
            case IF:
            case WHILE:
            case PRINT:
            case RETURN:
                return;
            }

            advance();
        }
    }

    private ParseError error(Token token, String message) {
        ErrorReporter.error(token, message);
        return new ParseError();
    }

    private boolean check(TokenType type) {
        if (isAtEnd()) { return false; }
        return peek().type == type;
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
