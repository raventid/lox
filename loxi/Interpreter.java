package loxi;

class Interpreter implements Expr.Visitor<Object> {
    @Override
    public Object visitLiteralExpression(Expr.Literal expression) {
        return expression.value;
    }
}
