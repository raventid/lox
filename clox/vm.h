#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "object.h"
#include "table.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

// Represents single ongoing funtion call
typedef struct
{
    ObjFunction *function; // Pointer to the function we are in
    uint8_t *ip;           // Pointer to the current instruction
    Value *slots;
} CallFrame;

typedef struct
{
    CallFrame frames[FRAMES_MAX];
    int frameCount;

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
// static bool call(ObjFunction *function, int argCount);
static bool callValue(Value callee, int argCount);
static bool isFalsey(Value value);
static void concatenate();

#endif
