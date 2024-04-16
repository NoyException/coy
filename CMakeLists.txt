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
        src/frontend/Parser.h
        src/frontend/SemanticAnalyzer.cpp
        src/frontend/SemanticAnalyzer.h
        src/midend/IRGenerator.cpp
        src/midend/IRGenerator.h
        src/midend/Value.cpp
        src/midend/Value.h
        src/midend/IRInstruction.cpp
        src/midend/IRInstruction.h
        src/frontend/DataType.cpp
        src/frontend/DataType.h
        src/midend/IRStructure.cpp
        src/midend/IRStructure.h
        src/midend/IRDataType.cpp
        src/midend/IRDataType.h
        src/midend/IRPrinter.cpp
        src/midend/IRPrinter.h
        src/Compiler.cpp
        src/Compiler.h)

add_executable(test
        src/frontend/Lexer.cpp
        src/frontend/Lexer.h
        src/frontend/Token.cpp
        src/frontend/Token.h
        src/frontend/Node.cpp
        src/frontend/Node.h
        src/frontend/Parser.cpp
        src/frontend/Parser.h)
