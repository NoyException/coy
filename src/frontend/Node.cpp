//
// Created by noyex on 24-3-23.
//

#include "Node.h"

#include <utility>

namespace coy {

    std::string Node::toString() const {
        return toString(0);
    }

    NodeIdentifier::NodeIdentifier(const Token &token, std::string name)
            : Node(NodeType::IDENTIFIER, token), _name(std::move(name)) {

    }

    std::string NodeIdentifier::toString(int height) const {
        return std::string(height * 2, ' ') + "identifier " + _name;
    }

    NodeTyped::NodeTyped(NodeType type, const Token &token) : Node(type, token) {

    }

    NodeLeftValue::NodeLeftValue(const Token &token,
                                 const std::shared_ptr<NodeIdentifier> &identifier,
                                 const std::vector<std::shared_ptr<NodeTyped>> &indexes)
            : NodeTyped(NodeType::LEFT_VALUE, token),
              _identifier(identifier), _indexes(indexes) {

    }

    std::string NodeLeftValue::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "left value " + _identifier->getName();
        for (const auto &index: _indexes) {
            str += "\n" + index->toString(height + 1);
        }
        return str;
    }

    NodeInteger::NodeInteger(const Token &token, int num)
            : NodeTyped(NodeType::INTEGER, token), _num(num) {

    }

    std::string NodeInteger::toString(int height) const {
        return std::string(height * 2, ' ') + "int " + std::to_string(_num);
    }

    NodeFloat::NodeFloat(const Token &token, float num)
            : NodeTyped(NodeType::FLOAT, token), _num(num) {

    }

    std::string NodeFloat::toString(int height) const {
        return std::string(height * 2, ' ') + "float " + std::to_string(_num);
    }

    NodeDataType::NodeDataType(const Token &token, std::string type)
            : NodeTyped(NodeType::DATA_TYPE, token), _type(std::move(type)) {

    }

    std::string NodeDataType::toString(int height) const {
        return std::string(height * 2, ' ') + "type " + _type;
    }

    NodeUnaryOperator::NodeUnaryOperator(const Token &token, std::string op, const std::shared_ptr<NodeTyped> &node)
            : NodeTyped(NodeType::UNARY_OPERATOR, token),
              _op(std::move(op)), _node(node) {}

    std::string NodeUnaryOperator::toString(int height) const {
        return std::string(height * 2, ' ') + "unary op " + _op + "\n" + _node->toString(height + 1);
    }

    NodeBinaryOperator::NodeBinaryOperator(const Token &token, std::string op,
                                           const std::shared_ptr<NodeTyped> &left,
                                           const std::shared_ptr<NodeTyped> &right)
            : NodeTyped(NodeType::BINARY_OPERATOR, token),
              _op(std::move(op)), _left(left),
              _right(right) {

    }

    std::string NodeBinaryOperator::toString(int height) const {
        return std::string(height * 2, ' ') + "binary op " + _op + "\n" + _left->toString(height + 1) + "\n" +
               _right->toString(height + 1);
    }

    NodeIf::NodeIf(const Token &token, const std::shared_ptr<NodeTyped> &condition,
                   const std::shared_ptr<Node> &then,
                   const std::shared_ptr<Node> &elseStatement)
            : Node(NodeType::IF, token), _condition(condition), _then(then),
              _else(elseStatement) {

    }

    std::string NodeIf::toString(int height) const {
        return std::string(height * 2, ' ') + "if\n" + _condition->toString(height + 1) + "\n" +
               _then->toString(height + 1) + (_else ? "\n" + _else->toString(height + 1) : "");
    }

    NodeWhile::NodeWhile(const Token &token, const std::shared_ptr<NodeTyped> &condition,
                         const std::shared_ptr<Node> &body)
            : Node(NodeType::WHILE, token), _condition(condition), _body(body) {

    }

    std::string NodeWhile::toString(int height) const {
        return std::string(height * 2, ' ') + "while\n" + _condition->toString(height + 1) + "\n" +
               _body->toString(height + 1);
    }

    NodeBreak::NodeBreak(const Token &token) : Node(NodeType::BREAK, token) {

    }

    std::string NodeBreak::toString(int height) const {
        return std::string(height * 2, ' ') + "break";
    }

    NodeContinue::NodeContinue(const Token &token) : Node(NodeType::CONTINUE, token) {

    }

    std::string NodeContinue::toString(int height) const {
        return std::string(height * 2, ' ') + "continue";
    }

    NodeReturn::NodeReturn(const Token &token, const std::shared_ptr<NodeTyped> &expression)
            : Node(NodeType::RETURN, token), _expression(expression) {

    }

    std::string NodeReturn::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "return";
        if (_expression)
            str += "\n" + _expression->toString(height + 1);
        return str;
    }

    NodeDefinition::NodeDefinition(const Token &token,
                                   const std::shared_ptr<NodeIdentifier> &identifier,
                                   const std::shared_ptr<NodeTyped> &initialValue,
                                   const std::vector<int> &dimensions) :
            Node(NodeType::DEFINITION, token), _identifier(identifier),
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

    NodeDeclaration::NodeDeclaration(const Token &token, const std::shared_ptr<NodeDataType> &type,
                                     const std::vector<std::shared_ptr<NodeDefinition>> &definitions)
            : Node(NodeType::DECLARATION, token), _type(type), _definitions(definitions) {

    }

    std::string NodeDeclaration::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "declaration";
        str += "\n" + _type->toString(height + 1);
        for (const auto &definition: _definitions) {
            str += "\n" + definition->toString(height + 1);
        }
        return str;
    }

    NodeAssignment::NodeAssignment(const Token &token,
                                   const std::shared_ptr<NodeLeftValue> &left,
                                   const std::shared_ptr<NodeTyped> &expression) :
            Node(NodeType::ASSIGNMENT, token), _left(left), _expression(expression) {

    }

    std::string NodeAssignment::toString(int height) const {
        return std::string(height * 2, ' ') + "assignment\n" + _left->toString(height + 1) + "\n" +
               _expression->toString(height + 1);
    }

    NodeBlock::NodeBlock(const Token &token, const std::vector<std::shared_ptr<Node>> &statements)
            : Node(NodeType::BLOCK, token), _statements(statements) {}

    std::string NodeBlock::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "block";
        for (const auto &statement: _statements) {
            str += "\n" + statement->toString(height + 1);
        }
        return str;
    }

    NodeFunctionParameter::NodeFunctionParameter(const Token &token,
                                                 const std::shared_ptr<NodeDataType> &type,
                                                 const std::shared_ptr<NodeIdentifier> &identifier, bool isPointer,
                                                 const std::vector<int> &dimensions) :
            Node(NodeType::FUNCTION_PARAMETER, token), _type(type), _identifier(identifier), _isPointer(isPointer),
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

    NodeFunction::NodeFunction(const Token &token,
                               const std::shared_ptr<NodeDataType> &returnType,
                               const std::shared_ptr<NodeIdentifier> &name,
                               const std::vector<std::shared_ptr<NodeFunctionParameter>> &params,
                               const std::shared_ptr<NodeBlock> &body) :
            Node(NodeType::FUNCTION, token), _returnType(returnType), _name(name),
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

    NodeFunctionCall::NodeFunctionCall(const Token &token,
                                       const std::shared_ptr<NodeIdentifier> &identifier,
                                       const std::vector<std::shared_ptr<NodeTyped>> &arguments) :
            NodeTyped(NodeType::FUNCTION_CALL, token), _identifier(identifier), _arguments(arguments) {

    }

    std::string NodeFunctionCall::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "function call " + _identifier->getName();
        for (const auto &arg: _arguments) {
            str += "\n" + arg->toString(height + 1);
        }
        return str;
    }

    NodeProgram::NodeProgram(const Token &token, const std::vector<std::shared_ptr<Node>> &nodes)
            : Node(NodeType::PROGRAM, token), _nodes(nodes) {

    }

    std::string NodeProgram::toString(int height) const {
        std::string str = std::string(height * 2, ' ') + "program";
        for (const auto &node: _nodes) {
            str += "\n" + node->toString(height + 1);
        }
        return str;
    }
} // coy