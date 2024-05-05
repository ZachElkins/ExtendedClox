#include "baseobject.h"

static Value builtinObjectToString(ObjObject* object, int argCount) {
    if (argCount > 0) return NIL_VAL;
    char* name = object->obj.klass->name->chars;
    return OBJ_VAL(copyString(name, (int)strlen(name)));
}

ObjBuiltinClass* createObjectType() {
    ObjBuiltinClass* object = newBuiltinClass("Object");
    defineBuiltinMethod(object, "toString", (NativeMethodFn) builtinObjectToString);
    return object;
}