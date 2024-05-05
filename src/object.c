#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass* newClass(ObjString* name) {
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    initTable(&klass->methods);
    return klass;
}

static ObjNativeMethod* newNativeMethod(NativeMethodFn function) {
    ObjNativeMethod* nativeMethod = ALLOCATE_OBJ(ObjNativeMethod, OBJ_NATIVE_METHOD);
    nativeMethod->function = function;
    return nativeMethod;
}

void defineBuiltinMethod(ObjBuiltinClass* klass, const char* name, NativeMethodFn function) {
    push(OBJ_VAL(copyString(name, strlen(name))));
    push(OBJ_VAL(newNativeMethod(function)));
    tableSet(&klass->obj.methods, AS_STRING(peek(1)), peek(0));
    pop();
    pop();
}

ObjBuiltinClass* newBuiltinClass(const char* name) {
    push(OBJ_VAL(copyString(name, strlen(name))));
    ObjBuiltinClass* klass = ALLOCATE_OBJ(ObjBuiltinClass, OBJ_BUILTIN_CLASS);
    klass->obj.name = AS_STRING(peek(0));
    initTable(&klass->obj.methods);
    push(OBJ_VAL(klass));
    pop();
    pop();
    return klass;
}

ObjClosure* newClosure(ObjFunction* function) {
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue, function->upvalueCount);

    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjInstance* newInstance(ObjClass* klass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

ObjNative* newNative(NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();
    return string;
}

static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* takeString(char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);

    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);

    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

static char* functionToString(ObjFunction* function) {
    if (function->name == NULL) {
        return "<script>"; // Should not hit this cae
    }

    char* name = function->name->chars;
    size_t size = strlen(name) + 6;
    char* chars = ALLOCATE(char, size);
    snprintf(chars, size, "<fn %s>", name);
    return chars;
}

char *objectToString(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD:
            return functionToString(AS_BOUND_METHOD(value)->method->function);
        case OBJ_CLASS:
            return AS_CLASS(value)->name->chars;
        case OBJ_CLOSURE:
            return functionToString(AS_CLOSURE(value)->function);
        case OBJ_FUNCTION:
            return functionToString(AS_FUNCTION(value));
        case OBJ_INSTANCE: {
            char *name = AS_INSTANCE(value)->klass->name->chars;
            size_t size = strlen(name) + 12;
            char *chars = ALLOCATE(char, size);
            snprintf(chars, size, "<%s instance>", name);
            return chars;
        }
        case OBJ_NATIVE:
            return "<native fn>";
        case OBJ_STRING:
            return AS_CSTRING(value);
        case OBJ_UPVALUE:
            return "upvalue";
    }
}

static void printFunction(ObjFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD:
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
        case OBJ_CLASS:
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_INSTANCE:
            printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
    }
}