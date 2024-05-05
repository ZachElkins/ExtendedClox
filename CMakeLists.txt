cmake_minimum_required(VERSION 3.25)
project(clox C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_OSX_ARCHITECTURES "x86_64")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

add_executable(clox src/main.c src/common.h src/chunk.h src/chunk.c src/memory.h src/memory.c src/debug.h src/debug.c src/value.h src/value.c src/vm.c src/vm.h src/compiler.c src/compiler.h src/scanner.h src/scanner.c src/object.h src/object.c src/table.h src/table.c src/libc/baseobject.h src/libc/baseobject.c)
