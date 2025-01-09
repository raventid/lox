package loxi;

class AstPrinter implements Expr.Visitor<String>, Stmt.Visitor<String> {
    String print(Expr expr) {
        return expr.accept(this);
    }

    String print_statement(Stmt statement) {
        return statement.accept(this);
    }

    // Visitor implementation for expressions
    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return parenthesize(expr.operator.lexeme, expr.right);
    }

    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null) { return "nil"; }
        return expr.value.toString();
    }

    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return parenthesize("group", expr.expression);
    }

    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override
    public String visitVariableExpr(Expr.Variable expr) {
        return "UNSUPPORTED_TOKEN";
    }

    @Override
    public String visitAssignExpr(Expr.Assign expr) {
        return "UNSUPPORTED_TOKEN";
    }

    // Visitor implementation for statements
    @Override
    public String visitBlockStmt(Stmt.Block stmt) {
        return "UNSUPPORTED_Block_Statement";
    }

    @Override
    public String visitExpressionStmt(Stmt.Expression stmt) {
        return "UNSUPPORTED_TOKEN_Expression_Statement";
    }

    @Override
    public String visitPrintStmt(Stmt.Print stmt) {
        return "UNSUPPORTED_TOKEN_Print_Statement";
    }

    @Override
    public String visitVarStmt(Stmt.Var stmt) {
        return "UNSUPPORTED_TOKEN_Var_Statement";
    }

    private String parenthesize(String name, Expr... exprs) {
        StringBuilder builder = new StringBuilder();
        builder.append("(").append(name);
        for (Expr expr : exprs) {
            builder.append(" ");
            builder.append(expr.accept(this));
        }
        builder.append(")");
        return builder.toString();
    }

    // TODO: Move to a separate runner
    // public static void main(String[] args) {
    //     Expr expression = new Expr.Binary(
    //         new Expr.Unary(
    //             new Token(TokenType.MINUS, "-", null, 1),
    //             new Expr.Literal(123)),
    //         new Token(TokenType.STAR, "*", null, 1),
    //         new Expr.Grouping(
    //             new Expr.Literal(45.67)));
    //     System.out.println(new AstPrinter().print(expression));
    // }
}
