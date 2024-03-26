//
// Created by noyex on 24-3-23.
//

#ifndef COY_TOKEN_H
#define COY_TOKEN_H

#include <string>
#include <stdexcept>
#include <utility>

#define TYPE_IDENTIFIER 0
#define TYPE_KEYWORD 1
#define TYPE_SEPARATOR 2
#define TYPE_OPERATOR 3
#define TYPE_DATA_TYPE 4
#define TYPE_INTEGER 101
#define TYPE_FLOAT 102
#define TYPE_STRING 103
#define TYPE_EOF (-1)
#define TYPE_UNKNOWN (-2)
#define TYPE_COMMENT (-3)

namespace coy {

    struct Token {
        int type;
        std::string value;
    };
    
} // coy

#endif //COY_TOKEN_H
