#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

// Since C does not have generic types, we need to define a type that can hold any value. (We already have a vector defined for chunks in chunk.h)
typedef struct
{
    int capacity;
    int count;
    Value *values;
} ValueArray;

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);

#endif // clox_value_h
