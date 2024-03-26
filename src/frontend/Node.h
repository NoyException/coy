//
// Created by noyex on 24-3-23.
//

#ifndef COY_NODE_H
#define COY_NODE_H

#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include "Token.h"

namespace coy {

    enum class NodeType {
        RAW,
        IDENTIFIER,
        LEFT_VALUE,
        INTEGER,
        FLOAT,
        DATA_TYPE,
        UNARY_OPERATOR,
        BINARY_OPERATOR,
        IF,
        WHILE,
        BREAK,
        CONTINUE,
        RETURN,
        DEFINITION,
        DECLARATION,
        ASSIGNMENT,
        BLOCK,
        FUNCTION_PARAMETER,
        FUNCTION,
        FUNCTION_CALL,
        PROGRAM,
    };

    class Node {
    private:
        NodeType _type;
    protected:
        explicit Node(NodeType type) : _type(type) {}

    public:
        virtual ~Node() = default;

        [[nodiscard]] NodeType getType() const { return _type; }

        [[nodiscard]] std::string toString() const;

        [[nodiscard]] virtual std::string toString(int height) const = 0;

        template<class T>
        T instanceOf() const {
            return _type == std::remove_pointer_t<T>::TYPE;
        }

        template<class T>
        T as() const {
            if (instanceOf<T>()) {
                return static_cast<T>(*this);
            } else {
                throw std::runtime_error("Node is not of _type " + std::string(typeid(T).name()));
            }
        }
    };

    template<typename T>
    class NodeRaw : public Node {
    private:
        T _raw;
    public:
        explicit NodeRaw(T raw) : Node(NodeType::RAW), _raw(raw) {}

        [[nodiscard]] std::string toString(int height) const override {
            return std::string(height * 2, ' ') + "raw";
        }

        [[nodiscard]] T get() {
            return _raw;
        }
    };

    class NodeIdentifier : public Node {
    private:
        std::string _name;
    public:
        static const NodeType TYPE = NodeType::IDENTIFIER;

        explicit NodeIdentifier(std::string name) : Node(NodeType::IDENTIFIER), _name(std::move(name)) {}

        [[nodiscard]] std::string toString(int height) const override {
            return std::string(height * 2, ' ') + "identifier " + _name;
        }

        [[nodiscard]] std::string getName() const { return _name; }
    };

    class NodeLeftValue : public Node {
    private:
        std::shared_ptr<NodeIdentifier> _identifier;
        std::vector<std::shared_ptr<Node>> _indexes{};
    public:
        static const NodeType TYPE = NodeType::LEFT_VALUE;

        explicit NodeLeftValue(const std::shared_ptr<NodeIdentifier> &identifier) : Node(NodeType::LEFT_VALUE),
                                                                                    _identifier(identifier) {}

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "left value " + _identifier->getName();
            for (const auto &index: _indexes) {
                str += "\n" + index->toString(height + 1);
            }
            return str;
        }

        [[nodiscard]] std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        void addIndex(const std::shared_ptr<Node> &index) {
            _indexes.push_back(index);
        }

        [[nodiscard]] std::vector<std::shared_ptr<Node>> getIndexes() const { return _indexes; }
    };

    class NodeInteger : public Node {
    private:
        int _num;
    public:
        static const NodeType TYPE = NodeType::INTEGER;

        explicit NodeInteger(int num);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] int getNumber() const { return _num; }
    };

    class NodeFloat : public Node {
    private:
        float _num;
    public:
        static const NodeType TYPE = NodeType::FLOAT;

        explicit NodeFloat(float num);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] float getNumber() const { return _num; }
    };

    class NodeDataType : public Node {
    private:
        std::string _type;
    public:
        static const NodeType TYPE = NodeType::DATA_TYPE;

        explicit NodeDataType(std::string type) : Node(NodeType::DATA_TYPE), _type(std::move(type)) {}

        [[nodiscard]] std::string toString(int height) const override {
            return std::string(height * 2, ' ') + "type " + _type;
        }

        [[nodiscard]] std::string getVariableType() const { return _type; }
    };

    class NodeUnaryOperator : public Node {
    private:
        std::shared_ptr<Node> _node{};
        std::string _op;
    public:
        static const NodeType TYPE = NodeType::UNARY_OPERATOR;

        explicit NodeUnaryOperator(std::string op, const std::shared_ptr<Node> &node);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] std::string getOperator() const { return _op; }

        [[nodiscard]] std::shared_ptr<Node> getNode() const { return _node; }
    };

    class NodeBinaryOperator : public Node {
    private:
        std::shared_ptr<Node> _left{};
        std::shared_ptr<Node> _right{};
        std::string _op;
    public:
        static const NodeType TYPE = NodeType::BINARY_OPERATOR;

        explicit NodeBinaryOperator(std::string op, const std::shared_ptr<Node> &left,
                                    const std::shared_ptr<Node> &right);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] std::string getOperator() const { return _op; }

        [[nodiscard]] std::shared_ptr<Node> getLeft() const { return _left; }

        [[nodiscard]] std::shared_ptr<Node> getRight() const { return _right; }
    };

    class NodeIf : public Node {
    private:
        std::shared_ptr<Node> _condition;
        std::shared_ptr<Node> _body;
        std::shared_ptr<Node> _else;
    public:
        static const NodeType TYPE = NodeType::IF;

        explicit NodeIf(const std::shared_ptr<Node> &condition, const std::shared_ptr<Node> &body,
                        const std::shared_ptr<Node> &elseStatement = nullptr);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] std::shared_ptr<Node> getCondition() const { return _condition; }

        [[nodiscard]] std::shared_ptr<Node> getBody() const { return _body; }

        [[nodiscard]] std::shared_ptr<Node> getElse() const { return _else; }
    };
    
    class NodeWhile : public Node {
    private:
        std::shared_ptr<Node> _condition;
        std::shared_ptr<Node> _body;
    public:
        static const NodeType TYPE = NodeType::WHILE;

        explicit NodeWhile(const std::shared_ptr<Node> &condition, const std::shared_ptr<Node> &body) :
                Node(NodeType::WHILE), _condition(condition), _body(body) {}

        [[nodiscard]] std::string toString(int height) const override {
            return std::string(height * 2, ' ') + "while\n" + _condition->toString(height + 1) + "\n" +
                   _body->toString(height + 1);
        }

        [[nodiscard]] std::shared_ptr<Node> getCondition() const { return _condition; }

        [[nodiscard]] std::shared_ptr<Node> getBody() const { return _body; }
    };
    
    class NodeBreak : public Node {
    public:
        static const NodeType TYPE = NodeType::BREAK;

        explicit NodeBreak() : Node(NodeType::BREAK) {}

        [[nodiscard]] std::string toString(int height) const override {
            return std::string(height * 2, ' ') + "break";
        }
    };
    
    class NodeContinue : public Node {
    public:
        static const NodeType TYPE = NodeType::CONTINUE;

        explicit NodeContinue() : Node(NodeType::CONTINUE) {}

        [[nodiscard]] std::string toString(int height) const override {
            return std::string(height * 2, ' ') + "continue";
        }
    };
    
    class NodeReturn : public Node {
    private:
        std::shared_ptr<Node> _statement;
    public:
        static const NodeType TYPE = NodeType::RETURN;

        explicit NodeReturn(const std::shared_ptr<Node> &statement) : Node(NodeType::RETURN), _statement(statement) {}

        [[nodiscard]] std::string toString(int height) const override {
            return std::string(height * 2, ' ') + "return\n" + _statement->toString(height + 1);
        }

        [[nodiscard]] std::shared_ptr<Node> getStatement() const { return _statement; }
    };

    class NodeDefinition : public Node {
    private:
        std::shared_ptr<NodeIdentifier> _identifier;
        std::shared_ptr<Node> _initialValue;
        std::vector<int> _dimensions{};
    public:
        static const NodeType TYPE = NodeType::DEFINITION;

        explicit NodeDefinition(const std::shared_ptr<NodeIdentifier> &identifier,
                                const std::shared_ptr<Node> &initialValue) :
                Node(NodeType::DEFINITION), _identifier(identifier), _initialValue(initialValue) {}

        [[nodiscard]] std::string toString(int height) const override {
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

        void addDimension(int dimension) {
            _dimensions.push_back(dimension);
        }

        [[nodiscard]] std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] std::shared_ptr<Node> getInitialValue() const { return _initialValue; }

        [[nodiscard]] std::vector<int> getDimensions() const { return _dimensions; }
    };

    class NodeDeclaration : public Node {
    private:
        std::shared_ptr<NodeDataType> _type;
        std::vector<std::shared_ptr<NodeDefinition>> _definitions{};
    public:
        static const NodeType TYPE = NodeType::DECLARATION;

        explicit NodeDeclaration(const std::shared_ptr<NodeDataType> &type) : Node(NodeType::DECLARATION),
                                                                              _type(type) {}

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "declaration";
            str += "\n" + _type->toString(height + 1);
            for (const auto &definition: _definitions) {
                str += "\n" + definition->toString(height + 1);
            }
            return str;
        }

        void addDefinition(const std::shared_ptr<NodeDefinition> &definition) {
            _definitions.push_back(definition);
        }

        [[nodiscard]] std::shared_ptr<NodeDataType> getVariableType() const { return _type; }

        [[nodiscard]] std::vector<std::shared_ptr<NodeDefinition>> getDefinitions() const { return _definitions; }
    };

    class NodeAssignment : public Node {
    private:
        std::shared_ptr<NodeLeftValue> _left;
        std::shared_ptr<Node> _statement;
    public:
        static const NodeType TYPE = NodeType::ASSIGNMENT;

        explicit NodeAssignment(const std::shared_ptr<NodeLeftValue> &left, const std::shared_ptr<Node> &statement);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] std::shared_ptr<NodeLeftValue> getLeft() const { return _left; }

        [[nodiscard]] std::shared_ptr<Node> getStatement() const { return _statement; }
    };

    class NodeBlock : public Node {
    private:
        std::vector<std::shared_ptr<Node>> _statements{};
    public:
        static const NodeType TYPE = NodeType::BLOCK;

        explicit NodeBlock() : Node(NodeType::BLOCK) {}

        void addStatement(const std::shared_ptr<Node> &statement) {
            _statements.push_back(statement);
        }

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "block";
            for (const auto &statement: _statements) {
                str += "\n" + statement->toString(height + 1);
            }
            return str;
        }

        [[nodiscard]] std::vector<std::shared_ptr<Node>> getStatements() const { return _statements; }
    };

    class NodeFunctionParameter : public Node {
    private:
        std::shared_ptr<NodeDataType> _type;
        std::shared_ptr<NodeIdentifier> _identifier;
        bool _isPointer;
        std::vector<int> _dimensions{};
    public:
        static const NodeType TYPE = NodeType::FUNCTION_PARAMETER;

        explicit NodeFunctionParameter(const std::shared_ptr<NodeDataType> &type,
                                       const std::shared_ptr<NodeIdentifier> &identifier, bool isPointer) :
                Node(NodeType::FUNCTION_PARAMETER), _type(type), _identifier(identifier), _isPointer(isPointer) {}

        [[nodiscard]] std::string toString(int height) const override {
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

        void addDimension(int dimension) {
            _dimensions.push_back(dimension);
        }

        [[nodiscard]] std::shared_ptr<NodeDataType> getVariableType() const { return _type; }

        [[nodiscard]] std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] bool isPointer() const { return _isPointer; }

        [[nodiscard]] std::vector<int> getDimensions() const { return _dimensions; }
    };

    class NodeFunction : public Node {
    private:
        std::shared_ptr<NodeDataType> _returnType;
        std::shared_ptr<NodeIdentifier> _name;
        std::vector<std::shared_ptr<NodeFunctionParameter>> _params;
        std::shared_ptr<NodeBlock> _body;
    public:
        static const NodeType TYPE = NodeType::FUNCTION;

        explicit NodeFunction(const std::shared_ptr<NodeDataType> &returnType,
                              const std::shared_ptr<NodeIdentifier> &name,
                              const std::vector<std::shared_ptr<NodeFunctionParameter>> &params,
                              const std::shared_ptr<NodeBlock> &body) :
                Node(NodeType::FUNCTION), _returnType(returnType), _name(name),
                _params(params), _body(body) {}

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "function " + _name->getName();
            str += "\n" + _returnType->toString(height + 1);
            for (const auto &param: _params) {
                str += "\n" + param->toString(height + 1);
            }
            str += "\n" + _body->toString(height + 1);
            return str;
        }

        [[nodiscard]] std::shared_ptr<NodeDataType> getReturnType() const { return _returnType; }

        [[nodiscard]] std::vector<std::shared_ptr<NodeFunctionParameter>> getParams() const { return _params; }

        [[nodiscard]] std::shared_ptr<NodeIdentifier> getName() const { return _name; }

        [[nodiscard]] std::shared_ptr<NodeBlock> getBody() const { return _body; }
    };

    class NodeFunctionCall : public Node {
    private:
        std::shared_ptr<NodeIdentifier> _identifier;
        std::vector<std::shared_ptr<Node>> _argument;
    public:
        static const NodeType TYPE = NodeType::FUNCTION_CALL;

        explicit NodeFunctionCall(const std::shared_ptr<NodeIdentifier> &identifier,
                                  const std::vector<std::shared_ptr<Node>> &argument) :
                Node(NodeType::FUNCTION_CALL), _identifier(identifier), _argument(argument) {}

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "function call " + _identifier->getName();
            for (const auto &arg: _argument) {
                str += "\n" + arg->toString(height + 1);
            }
            return str;
        }

        [[nodiscard]] std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] std::vector<std::shared_ptr<Node>> getArgument() const { return _argument; }
    };

    class NodeProgram : public Node {
    private:
        std::vector<std::shared_ptr<Node>> _nodes;
    public:
        static const NodeType TYPE = NodeType::PROGRAM;

        explicit NodeProgram(const std::vector<std::shared_ptr<Node>> &nodes)
                : Node(NodeType::PROGRAM), _nodes(nodes) {}

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "program";
            for (const auto &node: _nodes) {
                str += "\n" + node->toString(height + 1);
            }
            return str;
        }

        [[nodiscard]] std::vector<std::shared_ptr<Node>> getNodes() const { return _nodes; }
    };
} // coy

#endif //COY_NODE_H
