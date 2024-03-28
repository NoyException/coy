﻿//
// Created by noyex on 24-3-23.
//

#include "Node.h"

#include <utility>

namespace coy {

    std::string Node::toString() const {
        return toString(0);
    }

    NodeIdentifier::NodeIdentifier(std::string name) : Node(NodeType::IDENTIFIER), _name(std::move(name)) {

    }

    std::string NodeIdentifier::toString(int height) const {
        return std::string(height * 2, ' ') + "identifier " + _name;
    }

    NodeLeftValue::NodeLeftValue(const std::shared_ptr<NodeIdentifier> &identifier,
                                 const std::vector<std::shared_ptr<Node>> &indexes)
            : Node(NodeType::LEFT_VALUE),
              _identifier(identifier), _indexes(indexes) {

    }

    std::string NodeLeftValue::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "left value " + _identifier->getName();
        for (const auto &index: _indexes) {
            str += "\n" + index->toString(height + 1);
        }
        return str;
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

    NodeDataType::NodeDataType(std::string type) : Node(NodeType::DATA_TYPE), _type(std::move(type)) {

    }

    std::string NodeDataType::toString(int height) const {
        return std::string(height * 2, ' ') + "type " + _type;
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

    NodeWhile::NodeWhile(const std::shared_ptr<Node> &condition, const std::shared_ptr<Node> &body) :
            Node(NodeType::WHILE), _condition(condition), _body(body) {

    }

    std::string NodeWhile::toString(int height) const {
        return std::string(height * 2, ' ') + "while\n" + _condition->toString(height + 1) + "\n" +
               _body->toString(height + 1);
    }

    NodeBreak::NodeBreak() : Node(NodeType::BREAK) {

    }

    std::string NodeBreak::toString(int height) const {
        return std::string(height * 2, ' ') + "break";
    }

    NodeContinue::NodeContinue() : Node(NodeType::CONTINUE) {

    }

    std::string NodeContinue::toString(int height) const {
        return std::string(height * 2, ' ') + "continue";
    }

    NodeReturn::NodeReturn(const std::shared_ptr<Node> &statement) : Node(NodeType::RETURN), _statement(statement) {

    }

    std::string NodeReturn::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "return";
        if (_statement)
            str += "\n" + _statement->toString(height + 1);
        return str;
    }

    NodeDefinition::NodeDefinition(const std::shared_ptr<NodeIdentifier> &identifier,
                                   const std::shared_ptr<Node> &initialValue,
                                   const std::vector<int> &dimensions) :
            Node(NodeType::DEFINITION), _identifier(identifier),
            _initialValue(initialValue), _dimensions(dimensions) {

    }

    std::string NodeDefinition::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "definition";
        if (!_dimensions.empty()) {
            str += " with dimensions (";
            for (int i = 0; i < _dimensions.size(); ++i) {
                str += std::to_string(_dimensions[i]);
                if (i != _dimensions.size() - 1)
                    str += ", ";
            }
            str += ")";
        }
        str += "\n" + _identifier->toString(height + 1);
        if (_initialValue)
            str += "\n" + _initialValue->toString(height + 1);
        return str;
    }

    NodeDeclaration::NodeDeclaration(const std::shared_ptr<NodeDataType> &type,
                                     const std::vector<std::shared_ptr<NodeDefinition>> &definitions)
            : Node(NodeType::DECLARATION), _type(type), _definitions(definitions) {

    }

    std::string NodeDeclaration::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "declaration";
        str += "\n" + _type->toString(height + 1);
        for (const auto &definition: _definitions) {
            str += "\n" + definition->toString(height + 1);
        }
        return str;
    }

    NodeAssignment::NodeAssignment(const std::shared_ptr<NodeLeftValue> &left, const std::shared_ptr<Node> &statement) :
            Node(NodeType::ASSIGNMENT), _left(left), _statement(statement) {

    }

    std::string NodeAssignment::toString(int height) const {
        return std::string(height * 2, ' ') + "assignment\n" + _left->toString(height + 1) + "\n" +
               _statement->toString(height + 1);
    }

    NodeBlock::NodeBlock(const std::vector<std::shared_ptr<Node>> &statements)
    : Node(NodeType::BLOCK), _statements(statements) {}

    std::string NodeBlock::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "block";
        for (const auto &statement: _statements) {
            str += "\n" + statement->toString(height + 1);
        }
        return str;
    }

    NodeFunctionParameter::NodeFunctionParameter(const std::shared_ptr<NodeDataType> &type,
                                                 const std::shared_ptr<NodeIdentifier> &identifier, bool isPointer,
                                                 const std::vector<int> &dimensions) :
            Node(NodeType::FUNCTION_PARAMETER), _type(type), _identifier(identifier), _isPointer(isPointer),
            _dimensions(dimensions) {

    }

    std::string NodeFunctionParameter::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "parameter";
        if (!_dimensions.empty()) {
            str += _type->Node::toString() + " with dimensions (";
            for (int i = 0; i < _dimensions.size(); ++i) {
                str += std::to_string(_dimensions[i]);
                if (i != _dimensions.size() - 1)
                    str += ", ";
            }
            str += ")";
        }
        str += "\n" + _type->toString(height + 1);
        str += "\n" + _identifier->toString(height + 1);
        return str;
    }

    NodeFunction::NodeFunction(const std::shared_ptr<NodeDataType> &returnType,
                               const std::shared_ptr<NodeIdentifier> &name,
                               const std::vector<std::shared_ptr<NodeFunctionParameter>> &params,
                               const std::shared_ptr<NodeBlock> &body) :
            Node(NodeType::FUNCTION), _returnType(returnType), _name(name),
            _params(params), _body(body) {

    }

    std::string NodeFunction::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "function " + _name->getName();
        str += "\n" + _returnType->toString(height + 1);
        for (const auto &param: _params) {
            str += "\n" + param->toString(height + 1);
        }
        str += "\n" + _body->toString(height + 1);
        return str;
    }

    NodeFunctionCall::NodeFunctionCall(const std::shared_ptr<NodeIdentifier> &identifier,
                                       const std::vector<std::shared_ptr<Node>> &argument) :
            Node(NodeType::FUNCTION_CALL), _identifier(identifier), _argument(argument) {

    }

    std::string NodeFunctionCall::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "function call " + _identifier->getName();
        for (const auto &arg: _argument) {
            str += "\n" + arg->toString(height + 1);
        }
        return str;
    }

    NodeProgram::NodeProgram(const std::vector<std::shared_ptr<Node>> &nodes)
            : Node(NodeType::PROGRAM), _nodes(nodes) {

    }

    std::string NodeProgram::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "program";
        for (const auto &node: _nodes) {
            str += "\n" + node->toString(height + 1);
        }
        return str;
    }
} // coy