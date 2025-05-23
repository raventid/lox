#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"

#define STACK_MAX 256

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

typedef struct
{
    Chunk *chunk;
    uint8_t *ip;
    Value stack[STACK_MAX];
    Value *stackTop;
} VM;

void initVM();
void freeVM();
static void runtimeError(const char *format, ...);
InterpretResult interpret(const char *source);
void push(Value value);
Value pop();
static Value peek(int distance);

#endif
