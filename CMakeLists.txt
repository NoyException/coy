cmake_minimum_required(VERSION 3.22.1)
project(coy)

set(CMAKE_CXX_STANDARD 17)
# 将frontend和midend的头文件和源文件加入文件列表
file(GLOB FRONTEND_SRC src/frontend/*.cpp src/frontend/*.h)
file(GLOB MIDEND_SRC src/midend/*.cpp src/midend/*.h)

add_executable(coy src/main.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        src/Compiler.cpp
        src/Compiler.h
        src/backend/RISCVStructure.cpp
        src/backend/RISCVStructure.h
        src/backend/RISCVGenerator.cpp
        src/backend/RISCVGenerator.h)

add_executable(coy_lab1 src/labs/lab1.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        src/Compiler.cpp
        src/Compiler.h)

add_executable(coy_lab2 src/labs/lab2.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        src/Compiler.cpp
        src/Compiler.h)

add_executable(coy_lab3 src/labs/lab3.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        src/Compiler.cpp
        src/Compiler.h)

add_executable(coy_lab4 src/labs/lab4.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        src/Compiler.cpp
        src/Compiler.h)

#add_custom_target(test_lab4
#        COMMAND coy_lab4 input.sy output.S
#        COMMAND clang -nostdlib -nostdinc -static -target riscv64-unknown-linux-elf -march=rv64im -mabi=lp64 -fuse-ld=lld output.S sysy-runtime-lib/build/libsysy.a -o test -Lsysy_runtime_lib
#        COMMAND qemu-riscv64-static test
#        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#)
#
#add_dependencies(test_lab4 coy_lab4)