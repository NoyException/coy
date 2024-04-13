//
// Created by noyex on 24-3-23.
//

#ifndef COY_NODE_H
#define COY_NODE_H

#include <string>
#include <stdexcept>
#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>
#include "Token.h"
#include "DataType.h"

namespace coy {

    enum class NodeType {
        NODE,
        RAW,
        IDENTIFIER,
        TYPED,
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

    bool isAssignableFrom(NodeType type, NodeType parent);

    class Node : public std::enable_shared_from_this<Node> {
    private:
        NodeType _type;
        Token _token;
    protected:
        explicit Node(NodeType type, Token token) : _type(type), _token(std::move(token)) {}

    public:
        static constexpr NodeType TYPE = NodeType::NODE;

        virtual ~Node() = default;

        [[nodiscard]] NodeType getType() const { return _type; }

        [[nodiscard]] Token getToken() const { return _token; }

        [[nodiscard]] std::string toString() const;

        [[nodiscard]] virtual std::string toString(int height) const = 0;

        template<class T>
        bool is() const {
            return isAssignableFrom(_type, std::remove_pointer_t<T>::TYPE);
        }

        template<class T>
        std::shared_ptr<T> as() {
            if (is<T>()) {
                return std::dynamic_pointer_cast<T>(shared_from_this());
            } else {
                return nullptr;
            }
        }

        template<class T>
        std::shared_ptr<const T> as() const {
            if (is<T>()) {
                return std::dynamic_pointer_cast<const T>(shared_from_this());
            } else {
                return nullptr;
            }
        }
    };

    template<typename T>
    class NodeRaw : public Node {
    private:
        T _raw;
    public:
        static constexpr NodeType TYPE = NodeType::RAW;

        explicit NodeRaw(const Token &token, T raw) : Node(NodeType::RAW, token), _raw(raw) {}

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
        std::string _uniqueName;
    public:
        static constexpr NodeType TYPE = NodeType::IDENTIFIER;

        explicit NodeIdentifier(const Token &token, std::string name);

        [[nodiscard]] std::string toString(int height) const override;
        
        void setUniqueName(const std::string &uniqueName) {
            _uniqueName = uniqueName;
        }
        
        [[nodiscard]] inline std::string getUniqueName() const { return _uniqueName; }

        [[nodiscard]] inline std::string getName() const { return _name; }
    };

    class NodeTyped : public Node {
    public:
        static constexpr NodeType TYPE = NodeType::TYPED;

        explicit NodeTyped(NodeType type, const Token &token);
    };

    class NodeLeftValue : public NodeTyped {
    private:
        std::shared_ptr<NodeIdentifier> _identifier;
        std::vector<std::shared_ptr<NodeTyped>> _indexes;
    public:
        static constexpr NodeType TYPE = NodeType::LEFT_VALUE;

        explicit NodeLeftValue(const Token &token,
                               const std::shared_ptr<NodeIdentifier> &identifier,
                               const std::vector<std::shared_ptr<NodeTyped>> &indexes);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] inline std::vector<std::shared_ptr<NodeTyped>> getIndexes() const { return _indexes; }
    };

    class NodeInteger : public NodeTyped {
    private:
        int _num;
    public:
        static constexpr NodeType TYPE = NodeType::INTEGER;

        explicit NodeInteger(const Token &token, int num);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline int getNumber() const { return _num; }
    };

    class NodeFloat : public NodeTyped {
    private:
        float _num;
    public:
        static constexpr NodeType TYPE = NodeType::FLOAT;

        explicit NodeFloat(const Token &token, float num);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline float getNumber() const { return _num; }
    };

    class NodeDataType : public NodeTyped {
    private:
        std::string _type;
    public:
        static constexpr NodeType TYPE = NodeType::DATA_TYPE;

        explicit NodeDataType(const Token &token, std::string type);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::string getTypeName() const { return _type; }

        [[nodiscard]] inline std::shared_ptr<ScalarType> getDataType() const {
            return std::make_shared<ScalarType>(_type);
        }
    };

    class NodeUnaryOperator : public NodeTyped {
    private:
        std::shared_ptr<NodeTyped> _operand;
        std::string _op;
    public:
        static constexpr NodeType TYPE = NodeType::UNARY_OPERATOR;

        explicit NodeUnaryOperator(const Token &token, std::string op, const std::shared_ptr<NodeTyped> &node);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::string getOperator() const { return _op; }

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getOperand() const { return _operand; }
    };

    class NodeBinaryOperator : public NodeTyped {
    private:
        std::shared_ptr<NodeTyped> _lhs;
        std::shared_ptr<NodeTyped> _rhs;
        std::string _op;
    public:
        static constexpr NodeType TYPE = NodeType::BINARY_OPERATOR;

        explicit NodeBinaryOperator(const Token &token, std::string op,
                                    const std::shared_ptr<NodeTyped> &lhs,
                                    const std::shared_ptr<NodeTyped> &rhs);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::string getOperator() const { return _op; }

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getLhs() const { return _lhs; }

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getRhs() const { return _rhs; }
    };

    class NodeIf : public Node {
    private:
        std::shared_ptr<NodeTyped> _condition;
        std::shared_ptr<Node> _then;
        std::shared_ptr<Node> _else;
    public:
        static constexpr NodeType TYPE = NodeType::IF;

        explicit NodeIf(const Token &token, const std::shared_ptr<NodeTyped> &condition,
                        const std::shared_ptr<Node> &then,
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
        static constexpr NodeType TYPE = NodeType::WHILE;

        explicit NodeWhile(const Token &token, const std::shared_ptr<NodeTyped> &condition,
                           const std::shared_ptr<Node> &body);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getCondition() const { return _condition; }

        [[nodiscard]] inline std::shared_ptr<Node> getBody() const { return _body; }
    };

    class NodeBreak : public Node {
    public:
        static constexpr NodeType TYPE = NodeType::BREAK;

        explicit NodeBreak(const Token &token);

        [[nodiscard]] std::string toString(int height) const override;
    };

    class NodeContinue : public Node {
    public:
        static constexpr NodeType TYPE = NodeType::CONTINUE;

        explicit NodeContinue(const Token &token);

        [[nodiscard]] std::string toString(int height) const override;
    };

    class NodeReturn : public Node {
    private:
        std::shared_ptr<NodeTyped> _expression;
    public:
        static constexpr NodeType TYPE = NodeType::RETURN;

        explicit NodeReturn(const Token &token, const std::shared_ptr<NodeTyped> &expression);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getExpression() const { return _expression; }
    };

    class NodeDefinition : public Node {
    private:
        std::shared_ptr<NodeIdentifier> _identifier;
        std::shared_ptr<NodeTyped> _initialValue;
        std::vector<int> _dimensions;
        std::shared_ptr<DataType> _dataType = nullptr;
    public:
        static constexpr NodeType TYPE = NodeType::DEFINITION;

        explicit NodeDefinition(const Token &token,
                                const std::shared_ptr<NodeIdentifier> &identifier,
                                const std::shared_ptr<NodeTyped> &initialValue,
                                const std::vector<int> &dimensions);

        void setBaseType(const std::string &baseType) {
            _dataType = std::make_shared<ScalarType>(baseType);
            if (!_dimensions.empty())
                _dataType = std::make_shared<ArrayType>(_dataType, _dimensions);
        }

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] inline std::shared_ptr<NodeTyped> getInitialValue() const { return _initialValue; }

        [[nodiscard]] inline std::vector<int> getDimensions() const { return _dimensions; }

        [[nodiscard]] inline std::shared_ptr<DataType> getDataType() const { return _dataType; }
    };

    class NodeDeclaration : public Node {
    private:
        std::shared_ptr<NodeDataType> _type;
        std::vector<std::shared_ptr<NodeDefinition>> _definitions{};
    public:
        static constexpr NodeType TYPE = NodeType::DECLARATION;

        explicit NodeDeclaration(const Token &token, const std::shared_ptr<NodeDataType> &type,
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
        static constexpr NodeType TYPE = NodeType::ASSIGNMENT;

        explicit NodeAssignment(const Token &token, const std::shared_ptr<NodeLeftValue> &left,
                                const std::shared_ptr<NodeTyped> &expression);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] std::shared_ptr<NodeLeftValue> getLeftValue() const { return _left; }

        [[nodiscard]] std::shared_ptr<NodeTyped> getExpression() const { return _expression; }
    };

    class NodeBlock : public Node {
    private:
        std::vector<std::shared_ptr<Node>> _statements;
    public:
        static constexpr NodeType TYPE = NodeType::BLOCK;

        explicit NodeBlock(const Token &token, const std::vector<std::shared_ptr<Node>> &statements);

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
        static constexpr NodeType TYPE = NodeType::FUNCTION_PARAMETER;

        explicit NodeFunctionParameter(const Token &token, const std::shared_ptr<NodeDataType> &type,
                                       const std::shared_ptr<NodeIdentifier> &identifier, bool isPointer,
                                       const std::vector<int> &dimensions = {});

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeDataType> getVariableType() const { return _type; }

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] inline bool isPointer() const { return _isPointer; }

        [[nodiscard]] inline std::vector<int> getDimensions() const { return _dimensions; }

        [[nodiscard]] inline std::shared_ptr<DataType> getDataType() const {
            std::shared_ptr<DataType> dataType = _type->getDataType();
            if (!_dimensions.empty())
                dataType = std::make_shared<ArrayType>(dataType, _dimensions);
            if (_isPointer)
                dataType = std::make_shared<PointerType>(dataType);
            return dataType;
        }
    };

    class NodeFunction : public Node {
    private:
        std::shared_ptr<NodeDataType> _returnType;
        std::shared_ptr<NodeIdentifier> _name;
        std::vector<std::shared_ptr<NodeFunctionParameter>> _params;
        std::shared_ptr<NodeBlock> _body;
    public:
        static constexpr NodeType TYPE = NodeType::FUNCTION;

        explicit NodeFunction(const Token &token,
                              const std::shared_ptr<NodeDataType> &returnType,
                              const std::shared_ptr<NodeIdentifier> &name,
                              const std::vector<std::shared_ptr<NodeFunctionParameter>> &params,
                              const std::shared_ptr<NodeBlock> &body);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeDataType> getReturnType() const { return _returnType; }

        [[nodiscard]] inline std::vector<std::shared_ptr<NodeFunctionParameter>> getParams() const { return _params; }

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getIdentifier() const { return _name; }

        [[nodiscard]] inline std::shared_ptr<NodeBlock> getBody() const { return _body; }

        [[nodiscard]] inline std::shared_ptr<DataType> getFunctionType() const {
            std::vector<std::shared_ptr<DataType>> params;
            params.reserve(_params.size());
            for (const auto &param: _params) {
                params.push_back(param->getDataType());
            }
            return std::make_shared<FunctionType>(_returnType->getDataType(), params);
        }
    };

    class NodeFunctionCall : public NodeTyped {
    private:
        std::shared_ptr<NodeIdentifier> _identifier;
        std::vector<std::shared_ptr<NodeTyped>> _arguments;
    public:
        static constexpr NodeType TYPE = NodeType::FUNCTION_CALL;

        explicit NodeFunctionCall(const Token &token,
                                  const std::shared_ptr<NodeIdentifier> &identifier,
                                  const std::vector<std::shared_ptr<NodeTyped>> &arguments);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::shared_ptr<NodeIdentifier> getIdentifier() const { return _identifier; }

        [[nodiscard]] inline std::vector<std::shared_ptr<NodeTyped>> getArguments() const { return _arguments; }
    };

    class NodeProgram : public Node {
    private:
        std::vector<std::shared_ptr<Node>> _nodes;
    public:
        static constexpr NodeType TYPE = NodeType::PROGRAM;

        explicit NodeProgram(const Token &token, const std::vector<std::shared_ptr<Node>> &nodes);

        [[nodiscard]] std::string toString(int height) const override;

        [[nodiscard]] inline std::vector<std::shared_ptr<Node>> getNodes() const { return _nodes; }
    };
} // coy

#endif //COY_NODE_H
