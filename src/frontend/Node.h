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
        INTEGER,
        FLOAT,
        UNARY_OPERATOR,
        BINARY_OPERATOR,
        IF,
        WHILE,
        DEFINITION,
        DECLARATION,
        ASSIGNMENT,
        BLOCK,
        FUNCTION,
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
        std::vector<std::shared_ptr<Node>> _indexes{};
    public:
        static const NodeType TYPE = NodeType::IDENTIFIER;

        explicit NodeIdentifier(std::string name) : Node(NodeType::IDENTIFIER), _name(std::move(name)) {}

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "identifier " + _name;
            if (!_indexes.empty())
                str += " with indexes";
            for (const auto &arg: _indexes) {
                str += "\n" + arg->toString(height + 1);
            }
            return str;
        }

        void addIndex(const std::shared_ptr<Node> &index) {
            _indexes.push_back(index);
        }

        [[nodiscard]] std::vector<std::shared_ptr<Node>> getIndexes() const { return _indexes; }

        [[nodiscard]] std::string getName() const { return _name; }
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

    class NodeUnaryOperator : public Node {
    private:
        std::shared_ptr<Node> _node{};
        std::string _op;
    public:
        static const NodeType TYPE = NodeType::INTEGER;

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
        static const NodeType TYPE = NodeType::INTEGER;

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
    
    class NodeDefinition : public Node {
    private:
        std::shared_ptr<Node> _identifier;
        std::shared_ptr<Node> _initialValue;
        std::vector<int> _dimensions{};
    public:
        static const NodeType TYPE = NodeType::DEFINITION;

        explicit NodeDefinition(const std::shared_ptr<Node> &identifier, const std::shared_ptr<Node> &initialValue) :
                Node(NodeType::DEFINITION), _identifier(identifier), _initialValue(initialValue) {}

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "definition";
            if(!_dimensions.empty()){
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

        [[nodiscard]] std::shared_ptr<Node> getIdentifier() const { return _identifier; }

        [[nodiscard]] std::shared_ptr<Node> getInitialValue() const { return _initialValue; }

        [[nodiscard]] std::vector<int> getDimensions() const { return _dimensions; }
    };
    
    class NodeDeclaration : public Node {
    private:
        std::string _type;
        std::vector<std::shared_ptr<Node>> _definitions{};
    public:
        static const NodeType TYPE = NodeType::DECLARATION;

        explicit NodeDeclaration(std::string type) : Node(NodeType::DECLARATION), _type(std::move(type)) {}

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "declaration " + _type;
            for (const auto &definition: _definitions) {
                str += "\n" + definition->toString(height + 1);
            }
            return str;
        }

        void addDefinition(const std::shared_ptr<Node> &definition) {
            _definitions.push_back(definition);
        }

        [[nodiscard]] std::string getVariableType() const { return _type; }

        [[nodiscard]] std::vector<std::shared_ptr<Node>> getDefinitions() const { return _definitions; }
    };

    class NodeAssignment : public Node {
    private:
        std::shared_ptr<Node> _left;
        std::shared_ptr<Node> _statement;
    public:
        static const NodeType TYPE = NodeType::ASSIGNMENT;

        explicit NodeAssignment(const std::shared_ptr<Node> &left, const std::shared_ptr<Node> &statement);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] std::shared_ptr<Node> getLeft() const { return _left; }

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

    class NodeFunction : public Node {
    private:
        std::string _name;
        std::vector<std::shared_ptr<Node>> _args{};
        std::shared_ptr<Node> _body;
    public:
        static const NodeType TYPE = NodeType::FUNCTION;

        explicit NodeFunction(std::string name, std::shared_ptr<Node> body) : Node(NodeType::FUNCTION),
                                                                              _name(std::move(name)),
                                                                              _body(std::move(body)) {}

        [[nodiscard]] std::string toString(int height) const override {
            std::string str = std::string(height * 2, ' ') + "function " + _name;
            if (!_args.empty())
                str += " with args";
            for (const auto &arg: _args) {
                str += "\n" + arg->toString(height + 1);
            }
            str += "\n" + _body->toString(height + 1);
            return str;
        }

        void addArg(const std::shared_ptr<Node> &arg) {
            _args.push_back(arg);
        }

        [[nodiscard]] std::vector<std::shared_ptr<Node>> getArgs() const { return _args; }

        [[nodiscard]] std::string getName() const { return _name; }

        [[nodiscard]] std::shared_ptr<Node> getBody() const { return _body; }
    };
} // coy

#endif //COY_NODE_H
