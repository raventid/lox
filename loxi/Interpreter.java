package loxi;

class Interpreter implements Expr.Visitor<Object> {
    @Override
    public Object visitLiteralExpression(Expr.Literal expression) {
        return expression.value;
    }

    @Override
    public Object visitGroupingExpression(Expr.Grouping expression) {
        return evaluate(expression.expression); // not the best naming
    }

    private Object evaluate(Expr expression) {
        return expression.accept(this);
    }
}
