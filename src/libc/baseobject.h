#ifndef clox_baseobject_h
#define clox_baseobject_h

#include "../object.h"

typedef struct {
    ObjInstance obj;
} ObjObject;

ObjBuiltinClass* createObjectType();

#endif //clox_baseobject_h
