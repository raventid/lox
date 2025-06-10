#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct
{
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // OR
    PREC_AND,        // AND
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY,
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct
{
    Token name;
    int depth;
} Local;

typedef enum
{
    TYPE_FUNCTION,
    TYPE_SCRIPT
} FunctionType;

typedef struct
{
    // struct keyword here is a must, because we have not yet defined a Compiler struct name
    struct Compiler *enclosing; // Pointer to the enclosing compiler, if any
    ObjFunction *function;      // Pointer to the function we are in
    FunctionType type;          // Helps to understand if it is toplevel or funtion body

    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler; // maybe it is better to call it CompilationContext? (we compile everything like functions)

Parser parser;
Compiler *current = NULL;
Chunk *compilingChunk;

static Chunk *currentChunk()
{
    return &current->function->chunk;
}

static void errorAt(Token *token, const char *message)
{
    if (parser.panicMode)
    {
        return;
    }

    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // Nothing
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

/**
 * @brief Report error at previous token
 * @param message
 */
static void error(const char *message)
{
    errorAt(&parser.previous, message);
}

/**
 * @brief Report error at current token
 * @param message
 */
static void errorAtCurrent(const char *message)
{
    errorAt(&parser.current, message);
}

static void advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(parser.current.start);
    }
}

/**
 * @brief Read the next token, but report error if it doesn't match what's expected
 * @param type
 * @param message
 */
static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if (!check(type))
    {
        return false;
    }

    advance();
    return true;
}

static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX)
    {
        error("Loop body is too large.");
    }

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitReturn()
{
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

/**
 * @brief Add a constant to the constant table and make sure we don't have too many
 * @param value constant to add
 * @return the index of where we added the constant
 */
static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX)
    {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->count - offset - 2;
    if (jump > UINT16_MAX)
    {
        error("Too much code to jump over.");
    }
    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler *compiler, FunctionType type)
{
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;

    compiler->localCount = 0;
    compiler->scopeDepth = 0;

    compiler->function = newFunction();
    current = compiler;

    if (type != TYPE_SCRIPT)
    {
        // we call initCompiler right after we parsed function name, so it is in a previous token
        // a bit risky tactic to rely on a global state, but makes everything simpler
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }

    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->name.start = ""; // << empty name will stop user from refering to this identifier in code
    local->name.length = 0;
}

static ObjFunction *endCompiler()
{
    emitReturn();
    ObjFunction *function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
    {
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
    }
#endif
    current = current->enclosing;
    return function;
}

static void beginScope()
{
    current->scopeDepth++;
}

static void endScope()
{
    current->scopeDepth--;

    // generating OP_POP command for the locals that are no longer in scope
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth >
               current->scopeDepth)
    {
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void expression();
static void statement();
static void declaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType)
    {
    case TOKEN_BANG_EQUAL:
        emitBytes(OP_EQUAL, OP_NOT); // << we don't implement all the instructions literally
        // << here we translate != to OP_EQUAL + OP_NOT, to save on
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitBytes(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitBytes(OP_GREATER, OP_NOT);
        break;
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    default:
        return; // Unreachable.
    }
}

static void literal(bool canAssign)
{
    switch (parser.previous.type)
    {
    case TOKEN_FALSE:
        emitByte(OP_FALSE);
        break;
    case TOKEN_NIL:
        emitByte(OP_NIL);
        break;
    case TOKEN_TRUE:
        emitByte(OP_TRUE);
        break;
    default:
        return; // Unreachable.
    }
}

static uint8_t identifierConstant(Token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b)
{
    if (a->length != b->length)
    {
        return false;
    }

    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name)
{
    // Our local variables resolution is just a linear search through the locals array :(
    for (int i = compiler->localCount - 1; i >= 0; i--)
    {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name))
        {
            if (local->depth == -1)
            {
                // We don't allow this type of variable declaration:
                // {
                //   var a = "outer";
                //     {
                //       var a = a;
                //     }
                // }
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

static void addLocal(Token name)
{
    if (current->localCount == UINT8_COUNT)
    {
        error("Too many local variables in function.");
        return;
    }
    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1; // -1 means variable is not initialized yet, only declared
}

static void declareVariable()
{
    // Global variables are implicitly declared
    if (current->scopeDepth == 0) // current->scopeDepth is 0 when we're at the top level, i.e. not inside a function or block
    {
        return;
    }

    Token *name = &parser.previous;

    // Walk backwards through the locals to see if the variable is already declared
    // if we cannot find it in the current scope, let's add it!
    for (int i = current->localCount - 1; i >= 0; i--)
    {
        Local *local = &current->locals[i];
        // we are in the next scope, so this scope does not have this variable yet
        if (local->depth != -1 && local->depth < current->scopeDepth)
        {
            break;
        }
        if (identifiersEqual(name, &local->name))
        {
            error("Already variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static uint8_t parseVariable(const char *errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0)
    {
        return 0;
    }

    return identifierConstant(&parser.previous);
}

static void markInitialized()
{
    // In case this is a function name we define in global scope
    // we don't need to mark it as initialized
    if (current->localCount == 0)
    {
        return; // No locals to mark as initialized
    }

    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global)
{
    if (current->scopeDepth > 0)
    {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList()
{
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            expression();

            if (argCount == 255)
            {
                error("Can't have more than 255 arguments.");
            }

            argCount++;
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void call(bool canAssign)
{
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void and_(bool canAssign)
{
    // At the point this is called, the left-hand side expression has already been
    // compiled. That means at runtime, its value will be on top of the stack. If that
    // value is falsey, then we know the entire and must be false, so we skip the right
    // operand
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND); // 414
    patchJump(endJump);
}

// After consuming a `(`, recurse into expression until we hit `)`
static void grouping(bool canAssign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign)
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void or_(bool canAssign)
{
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void string(bool canAssign)
{
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else
    {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    }
    else
    {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;

    // Compile the operand
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType)
    {
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    default:
        return; // Unreachable.
    }
}

/**
 * @brief Table of token rules to know what prefix and infix functions to call, and what precendence they have
 */
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

/**
 * @brief Consume the next token, look up its prefix function and run it
 *        Then recurse into infix operators until we're done.
 * @param precedence
 */
static void parsePrecedence(Precedence precedence)
{
    advance();

    printf("parsePrecedence | initiating new call\n");
    printf("parsePrecedence | getting prefix rule for: `%s`\n", tt_toString(parser.previous.type));

    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    // printf("parsePrecedence | prefixRule is `%p`\n", prefixRule);

    if (prefixRule == NULL)
    {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence)
    {
        advance();
        // printf("parsePrecedence | getting infix rule for: `%.*s`\n", parser.previous.start);
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        // printf("parsePrecedence | infixRule is `%p`\n", infixRule);
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) // TODO: shouldn't it be a !canAssign check?
    {
        error("Invalid assignment target.");
    }
}

static ParseRule *getRule(TokenType type)
{
    return &rules[type];
}

static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type)
{
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    // Compile the parameter list.
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) // arguments
    {
        do
        {
            current->function->arity++;
            if (current->function->arity > 255)
            {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t paramConstant = parseVariable(
                "Expect parameter name.");
            defineVariable(paramConstant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    // The body.
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    // Create the function object.
    ObjFunction *function = endCompiler();
    emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
}

static void funDeclaration()
{
    // To make recursion work, we mark the function declaration’s variable initialized as
    // soon as we compile the name, before we compile the body. That way the name
    // can be referenced inside the body without generating an error.
    // Function name is a variable, so we can use the same logic as for variables.
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();

    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration()
{
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL))
    {
        expression();
    }
    else
    {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

// @brief Compile a for statement. only supports the for(;;) {} form
static void forStatement()
{
    // the overall logic is a bit tricky, so better to refer to the book
    // https://craftinginterpreters.com/jumping-back-and-forth.html#increment-clause
    // full loop works like:
    // 1. compute condition, jump to the body,
    // 2. do the body, jumpt to the increment,
    // 3. do the increment, jump to the condition

    // For loop allows a variable declaration in the initializer clause,
    // so let's start a new scope for it
    beginScope();

    // Initializer clause
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON))
    {
        // No initializer clause, just an empty for loop
    }
    else if (match(TOKEN_VAR))
    {
        varDeclaration();
    }
    else
    {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;

    // condition clause
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON))
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // jump out of the loop if the condition is false
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // remove computed condition from the stack
    }

    // Increment clause appears textually before the body, but executes after it.
    //
    // So we’ll jump over the increment, run the body,
    // jump back up to the increment, run it, and then go to the next iteration.
    if (!match(TOKEN_RIGHT_PAREN))
    {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;

        expression();
        emitByte(OP_POP); // remove computed increment from the stack

        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);        // jump back to the start of the loop
        loopStart = incrementStart; // update the loop start to the increment
        patchJump(bodyJump);        // patch the jump to the body of the loop
    }

    statement();

    emitLoop(loopStart);

    if (exitJump != -1)
    {
        patchJump(exitJump); // we only need this if we generated a jump instruction
        emitByte(OP_POP);    // remove computed condition from the stack
    }

    endScope(); // End the scope of the for loop
}

static void ifStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // backpatching, we need to generate jump instruction now,
    // but we don't know where it will jump yet (only after we compile the then branch with a statement command)
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // remove computed condition we put on the stack (then branch case)
    statement();
    int elseJump = emitJump(OP_JUMP); // in case we entered the then branch, we need to jump over the else branch

    patchJump(thenJump);
    emitByte(OP_POP); // remove computed condition (else branch case)

    if (match(TOKEN_ELSE))
    {
        statement();
    }
    patchJump(elseJump);
}

static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement()
{
    if (current->type == TYPE_SCRIPT)
    {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON))
    {
        emitReturn();
    }
    else
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void whileStatement()
{
    // this is a point before the while loop
    // we should jump here if we want to repeat the loop
    // WE NEED to RECOMPUTE the CONDITION EXPRESSION to see if it is still true or not!

    int loopStart = currentChunk()->count;

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE); // jump over the body of the loop

    emitByte(OP_POP); // remove computed condition from the stack in case we entered the loop body
    statement();

    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP); // remove computed condition from the stack again, in case we entered the loop
}

static void synchronize()
{
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF)
    {
        if (parser.previous.type == TOKEN_SEMICOLON)
        {
            return;
        }

        switch (parser.current.type)
        {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;

        default:
            // Do nothing, just consume the token
            ;
        }

        advance();
    }
}

static void declaration()
{
    if (match(TOKEN_FUN))
    {
        funDeclaration();
    }
    else if (match(TOKEN_VAR))
    {
        varDeclaration();
    }
    else
    {
        statement();
    }

    if (parser.panicMode)
    {
        synchronize();
    }
}

static void statement()
{
    if (match(TOKEN_PRINT))
    {
        printStatement();
    }
    else if (match(TOKEN_IF))
    {
        ifStatement();
    }
    else if (match(TOKEN_RETURN))
    {
        returnStatement();
    }
    else if (match(TOKEN_WHILE))
    {
        whileStatement();
    }
    else if (match(TOKEN_FOR))
    {
        forStatement();
    }
    else if (match(TOKEN_LEFT_BRACE))
    {
        beginScope();
        block();
        endScope();
    }
    else
    {
        expressionStatement();
    }
}

ObjFunction *compile(const char *source)
{
    initScanner(source);

    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!match(TOKEN_EOF))
    {
        declaration();
    }
    ObjFunction *function = endCompiler();
    return parser.hadError ? NULL : function;
}