#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"

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
    Table globals;
    Table strings;

    Obj *objects; // << pointer to the head of all allocated objects list
} VM;

extern VM vm; // << global variable for the VM, initially to use in object.c for garbage collection tracking

void initVM();
void freeVM();
static void runtimeError(const char *format, ...);
InterpretResult interpret(const char *source);
void push(Value value);
Value pop();
static Value peek(int distance);
static bool isFalsey(Value value);
static void concatenate();

#endif
