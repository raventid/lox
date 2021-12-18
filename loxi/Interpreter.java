package loxi;

class Interpreter implements Expr.Visitor<Object> {
    @Override
    public Object visitLiteralExpr(Expr.Literal expression) {
        return expression.value;
    }

    @Override
    public Object visitGroupingExpr(Expr.Grouping expression) {
        return evaluate(expression.expression); // not the best naming
    }

    @Override
    public Object visitUnaryExpr(Expr.Unary expression) {
        Object right = evaluate(expression.right);

        switch(expression.operator.type) {
            case MINUS:
                return -(double)right;
            case BANG:
                return !isTruthy(right);
        }

        // Unreachable
        return null;
    }

    @Override
    public Object visitBinaryExpr(Expr.Binary expression) {
        Object left = evaluate(expression.left);
        Object right = evaluate(expression.right);

        switch(expression.operator.type) {
            case MINUS:
                return (double)left - (double)right;
            case SLASH:
                return (double)left - (double)right;
            case STAR:
                return (double)left * (double)right;
            case PLUS:
                if (left instanceof Double && right instanceof Double) {
                    return (double)left + (double)right;
                }

                if (left instanceof String && right instanceof String) {
                    return (String)left + (String)right;
                }

                break;
        }

        // Unreachable
        return null;
    }

    private Object evaluate(Expr expression) {
        return expression.accept(this);
    }

    private boolean isTruthy(Object object) {
        if (object == null) { return false; }
        if (object instanceof Boolean) return (boolean)object;
        return true;
    }
}
