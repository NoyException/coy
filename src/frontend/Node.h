//
// Created by noyex on 24-3-23.
//

#ifndef COY_NODE_H
#define COY_NODE_H

#include <string>
#include <stdexcept>

namespace coy {
    
    enum class NodeType {
        INTEGER,
        FLOAT,
        BINARY_OPERATOR,
        IF,
        WHILE,
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

        template<class T> T instanceOf() const {
            return _type == std::remove_pointer_t<T>::TYPE;
        }
        template<class T> T as() const {
            if (instanceOf<T>()){
                return static_cast<T>(*this);
            } else {
                throw std::runtime_error("Node is not of _type " + std::string(typeid(T).name()));
            }
        }
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
    
    class NodeBinaryOperator : public Node {
    private:
        Node* _left;
        Node* _right;
        std::string _op;
    public:
        static const NodeType TYPE = NodeType::INTEGER;
        explicit NodeBinaryOperator(Node* left, Node* right, std::string op);
        [[nodiscard]] std::string toString(int height) const override;
        [[nodiscard]] Node* getLeft() const { return _left; }
        [[nodiscard]] Node* getRight() const { return _right; }
        [[nodiscard]] std::string getOperator() const { return _op; }
    };
    
    class NodeIf : public Node {
    private:
        Node* _condition;
        Node* _body;
        Node* _else;
    public:
        static const NodeType TYPE = NodeType::IF;
        explicit NodeIf(Node* condition, Node* body, Node* _else = nullptr);
        [[nodiscard]] std::string toString(int height) const override;
        [[nodiscard]] Node* getCondition() const { return _condition; }
        [[nodiscard]] Node* getBody() const { return _body; }
        [[nodiscard]] Node* getElse() const { return _else; }
    };

} // coy

#endif //COY_NODE_H
