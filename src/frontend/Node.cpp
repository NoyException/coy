//
// Created by noyex on 24-3-23.
//

#include "Node.h"

#include <utility>

namespace coy {

    std::string Node::toString() const {
        return toString(0);
    }

    NodeInteger::NodeInteger(int num) : Node(NodeType::INTEGER), _num(num) {

    }

    std::string NodeInteger::toString(int height) const {
        return std::string(height * 2, ' ') + "int " + std::to_string(_num);
    }

    NodeFloat::NodeFloat(float num) : Node(NodeType::FLOAT), _num(num) {

    }

    std::string NodeFloat::toString(int height) const {
        return std::string(height * 2, ' ') + "float " + std::to_string(_num);
    }

    NodeUnaryOperator::NodeUnaryOperator(std::string op, const std::shared_ptr<Node> &node)
            : Node(NodeType::UNARY_OPERATOR),
              _op(std::move(op)), _node(node) {}

    std::string NodeUnaryOperator::toString(int height) const {
        return std::string(height * 2, ' ') + "unary op " + _op + "\n" + _node->toString(height + 1);
    }

    NodeBinaryOperator::NodeBinaryOperator(std::string op, const std::shared_ptr<Node> &left,
                                           const std::shared_ptr<Node> &right)
            : Node(NodeType::BINARY_OPERATOR),
              _op(std::move(op)), _left(left),
              _right(right) {

    }

    std::string NodeBinaryOperator::toString(int height) const {
        return std::string(height * 2, ' ') + "binary op " + _op + "\n" + _left->toString(height + 1) + "\n" +
               _right->toString(height + 1);
    }

    NodeIf::NodeIf(const std::shared_ptr<Node> &condition, const std::shared_ptr<Node> &body,
                   const std::shared_ptr<Node> &elseStatement) : Node(NodeType::IF), _condition(condition), _body(body),
                                                                 _else(elseStatement) {

    }

    std::string NodeIf::toString(int height) const {
        return std::string(height * 2, ' ') + "if\n" + _condition->toString(height + 1) + "\n" +
               _body->toString(height + 1) + (_else ? "\n" + _else->toString(height + 1) : "");
    }

    NodeAssignment::NodeAssignment(const std::shared_ptr<NodeLeftValue> &left, const std::shared_ptr<Node> &statement) :
            Node(NodeType::ASSIGNMENT), _left(left), _statement(statement) {

    }

    std::string NodeAssignment::toString(int height) const {
        return std::string(height * 2, ' ') + "assignment\n" + _left->toString(height + 1) + "\n" +
               _statement->toString(height + 1);
    }
} // coy