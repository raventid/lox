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
        if (expr.value == null) {
            return "nil";
        }
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
        return "AST_PRINTER_UNSUPPORTED_TOKEN";
    }

    @Override
    public String visitAssignExpr(Expr.Assign expr) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN";
    }

    // Visitor implementation for statements
    @Override
    public String visitBlockStmt(Stmt.Block stmt) {
        return "AST_PRINTER_UNSUPPORTED_Block_Statement";
    }

    @Override
    public String visitExpressionStmt(Stmt.Expression stmt) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN_Expression_Statement";
    }

    @Override
    public String visitPrintStmt(Stmt.Print stmt) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN_Print_Statement";
    }

    @Override
    public String visitVarStmt(Stmt.Var stmt) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN_Var_Statement";
    }

    @Override
    public String visitReturnStmt(Stmt.Return stmt) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN_Return_Statement";
    }

    @Override
    public String visitIfStmt(Stmt.If stmt) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN_If_Statement";
    }

    @Override
    public String visitCallExpr(Expr.Call expr) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN_Call_Expression";
    }

    @Override
    public String visitWhileStmt(Stmt.While stmt) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN_While_Statement";
    }

    @Override
    public String visitFunctionStmt(Stmt.Function stmt) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN_Function_Statement";
    }

    @Override
    public String visitLogicalExpr(Expr.Logical expr) {
        return "AST_PRINTER_UNSUPPORTED_TOKEN_Logical_Expression";
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
    // Expr expression = new Expr.Binary(
    // new Expr.Unary(
    // new Token(TokenType.MINUS, "-", null, 1),
    // new Expr.Literal(123)),
    // new Token(TokenType.STAR, "*", null, 1),
    // new Expr.Grouping(
    // new Expr.Literal(45.67)));
    // System.out.println(new AstPrinter().print(expression));
    // }
}
