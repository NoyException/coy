﻿cmake_minimum_required(VERSION 3.22.1)
project(coy)

set(CMAKE_CXX_STANDARD 17)
# 将frontend和midend的头文件和源文件加入文件列表
file(GLOB FRONTEND_SRC src/frontend/*.cpp src/frontend/*.h)
file(GLOB MIDEND_SRC src/midend/*.cpp src/midend/*.h)
file(GLOB BACKEND_SRC src/backend/*.cpp src/backend/*.h)

add_executable(coy src/main.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        ${BACKEND_SRC}
        src/Compiler.cpp
        src/Compiler.h)

add_executable(coy_lab1 src/labs/lab1.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        ${BACKEND_SRC}
        src/Compiler.cpp
        src/Compiler.h)

add_executable(coy_lab2 src/labs/lab2.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        ${BACKEND_SRC}
        src/Compiler.cpp
        src/Compiler.h)

add_executable(coy_lab3 src/labs/lab3.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        ${BACKEND_SRC}
        src/Compiler.cpp
        src/Compiler.h)

add_executable(coy_lab4 src/labs/lab4.cpp
        ${FRONTEND_SRC}
        ${MIDEND_SRC}
        ${BACKEND_SRC}
        src/Compiler.cpp
        src/Compiler.h)
