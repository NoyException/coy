//
// Created by noyex on 24-3-23.
//

#include "Node.h"

namespace coy {

    std::string Node::toString() const {
        return toString(0);
    }
    
    NodeInteger::NodeInteger(int num): Node(NodeType::INTEGER), _num(num){

    }

    std::string NodeInteger::toString(int height) const {
        return std::string(height*2, ' ')+"int "+std::to_string(_num);
    }

    NodeFloat::NodeFloat(float num) : Node(NodeType::FLOAT), _num(num) {

    }

    std::string NodeFloat::toString(int height) const {
        return std::string(height*2, ' ')+"float "+std::to_string(_num);
    }

    NodeBinaryOperator::NodeBinaryOperator(Node *left, Node *right, std::string op) : Node(NodeType::BINARY_OPERATOR), _left(left), _right(right), _op(std::move(op)) {

    }

    std::string NodeBinaryOperator::toString(int height) const {
        return std::string(height*2, ' ')+"binary op "+_op+"\n"+_left->toString(height+1)+"\n"+_right->toString(height+1);
    }

    NodeIf::NodeIf(Node *condition, Node *body, Node *_else) : Node(NodeType::IF), _condition(condition), _body(body), _else(_else) {

    }

    std::string NodeIf::toString(int height) const {
        return std::string(height*2, ' ')+"if\n"+_condition->toString(height+1)+"\n"+_body->toString(height+1)+(_else ? "\n"+_else->toString(height+1) : "");
    }

} // coy