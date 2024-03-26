cmake_minimum_required(VERSION 3.22.1)
project(coy)

set(CMAKE_CXX_STANDARD 17)

add_executable(coy src/main.cpp
        src/frontend/Lexer.cpp
        src/frontend/Lexer.h
        src/frontend/Token.cpp
        src/frontend/Token.h
        src/frontend/Node.cpp
        src/frontend/Node.h
        src/frontend/Parser.cpp
        src/frontend/Parser.h)
