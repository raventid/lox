package loxi;

import java.util.List;
import java.util.ArrayList;

class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
    final Environment globals = new Environment();
    private Environment environment = globals;

    Interpreter() {
        globals.define("clock", new LoxCallable() {
                @Override
                public int arity() { return 0; }

                @Override
                public Object call(Interpreter interpreter, List<Object> arguments) {
                    return (double)System.currentTimeMillis() / 1000.0;
                }

                @Override
                public String toString() { return "<native fn>"; }
            });
    }

    void interpret(List<Stmt> statements) {
        try {
            for(Stmt statement : statements) {
                execute(statement);
            }
        } catch(RuntimeError error) {
           Main.runtimeError(error);
        }
    }

    void executeBlock(List<Stmt> statements, Environment environment) {
        Environment previous = this.environment;
        try {
            this.environment = environment;

            for (Stmt statement : statements) {
                execute(statement);
            }
        } finally {
            this.environment = previous;
        }
    }

    private void execute(Stmt stmt) {
        stmt.accept(this);
    }

    private String stringify(Object object) {
      if (object == null) return "nil";
      if (object instanceof Double) {
        String text = object.toString();
        if (text.endsWith(".0")) {
          text = text.substring(0, text.length() - 2);
        }
        return text;
      }
      return object.toString();
    }

    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        executeBlock(stmt.statements, new Environment(environment));
        return null;
    }

    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        Object value = null;

        // In case `var a;` we leave it as `a = null`
        if (stmt.initializer != null) {
            value = evaluate(stmt.initializer);
        }

        environment.define(stmt.name.lexeme, value);
        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        evaluate(stmt.expression);
        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        LoxFunction function = new LoxFunction(stmt, environment);
        environment.define(stmt.name.lexeme, function);
        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        if (isTruthy(evaluate(stmt.condition))) {
            execute(stmt.thenBranch);
        } else {
            if (stmt.elseBranch != null) {
                execute(stmt.elseBranch);
            }
        }

        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        Object value = evaluate(stmt.expression);
        System.out.println(stringify(value));
        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        Object value = null;
        if (stmt.value != null) {
            value = evaluate(stmt.value);
        }

        throw new Return(value);
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        while (isTruthy(evaluate(stmt.condition))) {
            execute(stmt.body);
        }

        return null;
    }

    @Override
    public Object visitVariableExpr(Expr.Variable expression) {
        return environment.get(expression.name);
    }

    @Override
    public Object visitAssignExpr(Expr.Assign expression) {
        Object value = evaluate(expression.value);
        environment.assign(expression.name, value);
        return value;
    }

    @Override
    public Object visitLogicalExpr(Expr.Logical expr) {
        Object left = evaluate(expr.left);

        if (expr.operator.type == TokenType.OR) {
           if (isTruthy(left)) { return left; }
        } else {
           if (!isTruthy(left)) { return left; }
        }

        return evaluate(expr.right);
    }

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
                checkNumberOperand(expression.operator, right);
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
                checkNumberOperands(expression.operator, left, right);
                return (double)left - (double)right;
            case SLASH:
                checkNumberOperands(expression.operator, left, right);
                return (double)left / (double)right;
            case STAR:
                checkNumberOperands(expression.operator, left, right);
                return (double)left * (double)right;
            case PLUS:
                if (left instanceof Double && right instanceof Double) {
                    return (double)left + (double)right;
                }

                if (left instanceof String && right instanceof String) {
                    return (String)left + (String)right;
                }

                throw new RuntimeError(expression.operator, "Operands must be two numbers or two strings.");
            case GREATER:
                checkNumberOperands(expression.operator, left, right);
                return (double)left > (double)right;
            case GREATER_EQUAL:
                checkNumberOperands(expression.operator, left, right);
                return (double)left >= (double)right;
            case LESS:
                checkNumberOperands(expression.operator, left, right);
                return (double)left < (double)right;
            case LESS_EQUAL:
                checkNumberOperands(expression.operator, left, right);
                return (double)left <= (double)right;
            case BANG_EQUAL:
                return !isEqual(left, right);
            case EQUAL_EQUAL:
                return isEqual(left, right);
        }

        // Unreachable
        return null;
    }

    @Override
    public Object visitCallExpr(Expr.Call expr) {
        Object callee = evaluate(expr.callee);

        List<Object> arguments = new ArrayList<>();
        for (Expr argument : expr.arguments) {
            arguments.add(evaluate(argument));
        }

        if (!(callee instanceof LoxCallable)) {
            throw new RuntimeError(expr.paren, "Can only call functions and classes.");
        }

        LoxCallable function = (LoxCallable)callee;
        if (arguments.size() != function.arity()) {
            throw new RuntimeError(expr.paren, "Expected" + function.arity() + " arguments but got " + arguments.size() + ".");
        }
        return function.call(this, arguments);
    }

    private Object evaluate(Expr expression) {
        return expression.accept(this);
    }

    private boolean isEqual(Object obj1, Object obj2) {
        if (obj1 == null && obj2 == null) { return true; }
        if (obj1 == null) { return false; }
        return obj1.equals(obj2);
    }

    private boolean isTruthy(Object object) {
        if (object == null) { return false; }
        if (object instanceof Boolean) return (boolean)object;
        return true;
    }

    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) { return; }
        throw new RuntimeError(operator, "Operand must be a number");
    }

    private void checkNumberOperands(Token operator, Object operand1, Object operand2) {
        if (operand1 instanceof Double && operand2 instanceof Double) {
            return;
        }

        throw new RuntimeError(operator, "Operands must be numbers");
    }
}
