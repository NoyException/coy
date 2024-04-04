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

    class Node : public std::enable_shared_from_this<Node> {
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
        bool instanceOf() const {
            return _type == std::remove_pointer_t<T>::TYPE;
        }

        template<class T>
        std::shared_ptr<T> as() {
            if (instanceOf<T>()) {
                return std::dynamic_pointer_cast<T>(shared_from_this());
            } else {
                throw std::runtime_error("Node is not of _type " + std::string(typeid(T).name()));
            }
        }

        template<class T>
        std::shared_ptr<const T> as() const {
            if (instanceOf<T>()) {
                return std::dynamic_pointer_cast<const T>(shared_from_this());
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

        explicit NodeIdentifier(std::string name);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::string getName() const { return _name; }
    };

    class NodeTyped : public Node {
    public:
        explicit NodeTyped(NodeType type);
    };

    class NodeLeftValue : public NodeTyped {
    private:
        std::shared_ptr<NodeIdentifier> _identifier;
        std::vector<std::shared_ptr<NodeTyped>> _indexes;
    public:
        static const NodeType TYPE = NodeType::LEFT_VALUE;

        explicit NodeLeftValue(const std::shared_ptr<NodeIdentifier> &identifier,
                               const std::vector<std::shared_ptr<NodeTyped>> &indexes);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] inline std::vector<std::shared_ptr<NodeTyped>> getIndexes() const { return _indexes; }
    };

    class NodeInteger : public NodeTyped {
    private:
        int _num;
    public:
        static const NodeType TYPE = NodeType::INTEGER;

        explicit NodeInteger(int num);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline int getNumber() const { return _num; }
    };

    class NodeFloat : public NodeTyped {
    private:
        float _num;
    public:
        static const NodeType TYPE = NodeType::FLOAT;

        explicit NodeFloat(float num);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline float getNumber() const { return _num; }
    };

    class NodeDataType : public NodeTyped {
    private:
        std::string _type;
    public:
        static const NodeType TYPE = NodeType::DATA_TYPE;

        explicit NodeDataType(std::string type);

        [[nodiscard]] std::string toString(int height) const override;
        
        [[nodiscard]] inline std::string getDataType() const { return _type; }
    };

    class NodeUnaryOperator : public NodeTyped {
    private:
        std::shared_ptr<NodeTyped> _node;
        std::string _op;
    public:
        static const NodeType TYPE = NodeType::UNARY_OPERATOR;

        explicit NodeUnaryOperator(std::string op, const std::shared_ptr<NodeTyped> &node);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::string getOperator() const { return _op; }

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getNode() const { return _node; }
    };

    class NodeBinaryOperator : public NodeTyped {
    private:
        std::shared_ptr<NodeTyped> _left;
        std::shared_ptr<NodeTyped> _right;
        std::string _op;
    public:
        static const NodeType TYPE = NodeType::BINARY_OPERATOR;

        explicit NodeBinaryOperator(std::string op, const std::shared_ptr<NodeTyped> &left,
                                    const std::shared_ptr<NodeTyped> &right);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::string getOperator() const { return _op; }

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getLeft() const { return _left; }

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getRight() const { return _right; }
    };

    class NodeIf : public Node {
    private:
        std::shared_ptr<NodeTyped> _condition;
        std::shared_ptr<Node> _then;
        std::shared_ptr<Node> _else;
    public:
        static const NodeType TYPE = NodeType::IF;

        explicit NodeIf(const std::shared_ptr<NodeTyped> &condition, const std::shared_ptr<Node> &then,
                        const std::shared_ptr<Node> &elseStatement = nullptr);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getCondition() const { return _condition; }

        [[nodiscard]] inline std::shared_ptr<Node> getThen() const { return _then; }

        [[nodiscard]] inline std::shared_ptr<Node> getElse() const { return _else; }
    };

    class NodeWhile : public Node {
    private:
        std::shared_ptr<NodeTyped> _condition;
        std::shared_ptr<Node> _body;
    public:
        static const NodeType TYPE = NodeType::WHILE;

        explicit NodeWhile(const std::shared_ptr<NodeTyped> &condition, const std::shared_ptr<Node> &body);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getCondition() const { return _condition; }

        [[nodiscard]] inline std::shared_ptr<Node> getBody() const { return _body; }
    };

    class NodeBreak : public Node {
    public:
        static const NodeType TYPE = NodeType::BREAK;

        explicit NodeBreak();

        [[nodiscard]] std::string toString(int height) const override;
    };

    class NodeContinue : public Node {
    public:
        static const NodeType TYPE = NodeType::CONTINUE;

        explicit NodeContinue();

        [[nodiscard]] std::string toString(int height) const override;
    };

    class NodeReturn : public Node {
    private:
        std::shared_ptr<NodeTyped> _expression;
    public:
        static const NodeType TYPE = NodeType::RETURN;

        explicit NodeReturn(const std::shared_ptr<NodeTyped> &expression);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getExpression() const { return _expression; }
    };

    class NodeDefinition : public Node {
    private:
        std::shared_ptr<NodeIdentifier> _identifier;
        std::shared_ptr<NodeTyped> _initialValue;
        std::vector<int> _dimensions;
    public:
        static const NodeType TYPE = NodeType::DEFINITION;

        explicit NodeDefinition(const std::shared_ptr<NodeIdentifier> &identifier,
                                const std::shared_ptr<NodeTyped> &initialValue,
                                const std::vector<int> &dimensions);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getInitialValue() const { return _initialValue; }

        [[nodiscard]] inline std::vector<int> getDimensions() const { return _dimensions; }
    };

    class NodeDeclaration : public Node {
    private:
        std::shared_ptr<NodeDataType> _type;
        std::vector<std::shared_ptr<NodeDefinition>> _definitions{};
    public:
        static const NodeType TYPE = NodeType::DECLARATION;

        explicit NodeDeclaration(const std::shared_ptr<NodeDataType> &type,
                                 const std::vector<std::shared_ptr<NodeDefinition>> &definitions);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeDataType> getVariableType() const { return _type; }

        [[nodiscard]] inline std::vector<std::shared_ptr<NodeDefinition>>
        getDefinitions() const { return _definitions; }
    };

    class NodeAssignment : public Node {
    private:
        std::shared_ptr<NodeLeftValue> _left;
        std::shared_ptr<NodeTyped> _expression;
    public:
        static const NodeType TYPE = NodeType::ASSIGNMENT;

        explicit NodeAssignment(const std::shared_ptr<NodeLeftValue> &left, const std::shared_ptr<NodeTyped> &expression);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] std::shared_ptr<NodeLeftValue> getLeft() const { return _left; }

        [[nodiscard]] std::shared_ptr<NodeTyped> getExpression() const { return _expression; }
    };

    class NodeBlock : public Node {
    private:
        std::vector<std::shared_ptr<Node>> _statements;
    public:
        static const NodeType TYPE = NodeType::BLOCK;

        explicit NodeBlock(const std::vector<std::shared_ptr<Node>> &statements);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::vector<std::shared_ptr<Node>> getStatements() const { return _statements; }
    };

    class NodeFunctionParameter : public Node {
    private:
        std::shared_ptr<NodeDataType> _type;
        std::shared_ptr<NodeIdentifier> _identifier;
        bool _isPointer;
        std::vector<int> _dimensions;
    public:
        static const NodeType TYPE = NodeType::FUNCTION_PARAMETER;

        explicit NodeFunctionParameter(const std::shared_ptr<NodeDataType> &type,
                                       const std::shared_ptr<NodeIdentifier> &identifier, bool isPointer,
                                       const std::vector<int> &dimensions = {});

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeDataType> getVariableType() const { return _type; }

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] inline bool isPointer() const { return _isPointer; }

        [[nodiscard]] inline std::vector<int> getDimensions() const { return _dimensions; }
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
                              const std::shared_ptr<NodeBlock> &body);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeDataType> getReturnType() const { return _returnType; }

        [[nodiscard]] inline std::vector<std::shared_ptr<NodeFunctionParameter>> getParams() const { return _params; }

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getName() const { return _name; }

        [[nodiscard]] inline std::shared_ptr<NodeBlock> getBody() const { return _body; }
    };

    class NodeFunctionCall : public NodeTyped {
    private:
        std::shared_ptr<NodeIdentifier> _identifier;
        std::vector<std::shared_ptr<NodeTyped>> _arguments;
    public:
        static const NodeType TYPE = NodeType::FUNCTION_CALL;

        explicit NodeFunctionCall(const std::shared_ptr<NodeIdentifier> &identifier,
                                  const std::vector<std::shared_ptr<NodeTyped>> &arguments);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] inline std::vector<std::shared_ptr<NodeTyped>> getArguments() const { return _arguments; }
    };

    class NodeProgram : public Node {
    private:
        std::vector<std::shared_ptr<Node>> _nodes;
    public:
        static const NodeType TYPE = NodeType::PROGRAM;

        explicit NodeProgram(const std::vector<std::shared_ptr<Node>> &nodes);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::vector<std::shared_ptr<Node>> getNodes() const { return _nodes; }
    };
} // coy

#endif //COY_NODE_H
