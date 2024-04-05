//
// Created by noyex on 24-4-4.
//

#ifndef COY_SEMANTICANALYZER_H
#define COY_SEMANTICANALYZER_H

#include <unordered_map>
#include <string>
#include <deque>
#include <optional>

#include "Node.h"

namespace coy {
    class AnalyzeResult {
    private:
        bool _success;
        std::string _message;
        std::shared_ptr<Node> _node;

        AnalyzeResult(bool success, std::string message, const std::shared_ptr<Node> &node)
                : _success(success), _message(std::move(message)), _node(node) {}

    public:
        static AnalyzeResult success() {
            return {true, "", nullptr};
        }

        static AnalyzeResult failure(const std::string &message, const std::shared_ptr<Node> &node = nullptr) {
            return {false, message, node};
        }

        [[nodiscard]] bool isSuccess() const { return _success; }

        [[nodiscard]] std::string getMessage() const { return _message; }

        [[nodiscard]] std::shared_ptr<Node> getNode() const { return _node; }

        [[nodiscard]] AnalyzeResult attach(const std::shared_ptr<Node> &node) const {
            return {isSuccess(), getMessage(), node};
        }
    };

    class Type : public std::enable_shared_from_this<Type> {
    public:
        template<typename T>
        std::shared_ptr<T> as() {
            return std::dynamic_pointer_cast<T>(shared_from_this());
        }
        
        [[nodiscard]] virtual bool isAssignableFrom(const std::shared_ptr<Type> &other) const {
            return *this == *other;
        }
        
        [[nodiscard]] virtual std::shared_ptr<Type> at(int indexes) = 0;

        [[nodiscard]] virtual std::string toString() const = 0;

        [[nodiscard]] bool operator==(const std::string &other) const {
            return toString() == other;
        }

        [[nodiscard]] bool operator!=(const std::string &other) const {
            return toString() != other;
        }

        [[nodiscard]] bool operator==(const Type &other) const {
            return toString() == other.toString();
        }

        [[nodiscard]] bool operator!=(const Type &other) const {
            return toString() != other.toString();
        }
    };

    class ScalarType : public Type {
    private:
        std::string _name;
    public:
        explicit ScalarType(std::string name) : _name(std::move(name)) {}

        [[nodiscard]] bool isNumeric() const {
            return _name == "bool" || _name == "int" || _name == "float" || _name == "double";
        }

        [[nodiscard]] bool isReturnType() const {
            return _name == "void" || isNumeric();
        }
        
        [[nodiscard]] bool isAssignableFrom(const std::shared_ptr<Type> &other) const override {
            if (auto scalar = std::dynamic_pointer_cast<ScalarType>(other)) {
                if (_name == scalar->_name)
                    return true;
                if (_name == "double" && (scalar->_name =="bool" || scalar->_name == "int" || scalar->_name == "float"))
                    return true;
                if (_name == "float" && (scalar->_name =="bool" || scalar->_name == "int"))
                    return true;
                if (_name == "int" && scalar->_name == "bool")
                    return true;
            }
            return false;
        }

        [[nodiscard]] std::shared_ptr<Type> at(int indexes) override {
            if (indexes == 0) {
                return shared_from_this();
            }
            return nullptr;
        }

        [[nodiscard]] std::string toString() const override {
            return _name;
        }
    };

    class ArrayType : public Type {
    private:
        std::shared_ptr<Type> _base;
        int _dimension;
    public:
        ArrayType(const std::shared_ptr<Type>& base, int dimension) : _base(base), _dimension(dimension) {}
        
        ArrayType(const std::shared_ptr<Type>& base, const std::vector<int>& dimensions)
        : _base(dimensions.size() == 1 ? base : std::make_shared<ArrayType>(base, std::vector<int>(dimensions.begin() + 1, dimensions.end()))),
          _dimension(dimensions[0]) {}

        [[nodiscard]] std::shared_ptr<Type> getBase() const { return _base; }

        [[nodiscard]] int getDimension() const { return _dimension; }

        [[nodiscard]] std::shared_ptr<Type> at(int indexes) override {
            if (indexes == 0) {
                return shared_from_this();
            }
            if (indexes == 1) {
                return _base;
            }
            return _base->at(indexes - 1);
        }

        [[nodiscard]] std::string toString() const override {
            return _base->toString() + "[" + std::to_string(_dimension) + "]";
        }
    };

    class FunctionType : public Type {
    private:
        std::shared_ptr<Type> _returnType;
        std::vector<std::shared_ptr<Type>> _args;
    public:
        FunctionType(std::shared_ptr<Type> returnType, std::vector<std::shared_ptr<Type>> args) : _returnType(
                std::move(returnType)), _args(std::move(args)) {}

        [[nodiscard]] std::shared_ptr<Type> getReturnType() const { return _returnType; }

        [[nodiscard]] std::vector<std::shared_ptr<Type>> getArgs() const { return _args; }
        
        [[nodiscard]] std::shared_ptr<Type> at(int indexes) override {
            if (indexes == 0) {
                return shared_from_this();
            }
            return nullptr;
        }

        [[nodiscard]] std::string toString() const override {
            std::string str = _returnType->toString() + "(";
            for (int i = 0; i < _args.size(); ++i) {
                str += _args[i]->toString();
                if (i != _args.size() - 1) {
                    str += ", ";
                }
            }
            str += ")";
            return str;
        }
    };

    class PointerType : public Type {
    private:
        std::shared_ptr<Type> _base;
    public:
        explicit PointerType(std::shared_ptr<Type> base) : _base(std::move(base)) {}

        [[nodiscard]] std::shared_ptr<Type> getBase() const { return _base; }
        
        bool isAssignableFrom(const std::shared_ptr<Type> &other) const override{
            if (auto pointer = std::dynamic_pointer_cast<ArrayType>(other)) {
                return *_base == *pointer->getBase();
            }
            return Type::isAssignableFrom(other);
        }
        
        [[nodiscard]] std::shared_ptr<Type> at(int indexes) override {
            if (indexes == 0) {
                return _base;
            }
            return _base->at(indexes - 1);
        }

        [[nodiscard]] std::string toString() const override {
            return _base->toString() + "*";
        }
    };

    class SemanticAnalyzer {
    private:
        std::deque<std::unordered_map<std::string, std::shared_ptr<Type>>> _scopes;
        std::shared_ptr<ScalarType> _returnType;
    public:
        SemanticAnalyzer();

        AnalyzeResult analyze(const std::shared_ptr<Node> &node);

        bool isDeclared(const std::string &name);

        std::shared_ptr<Type> getType(const std::string &name);

        std::shared_ptr<Type> getType(const std::shared_ptr<NodeTyped> &node);

        AnalyzeResult declare(const std::string &name, const std::shared_ptr<Type> &type);

        AnalyzeResult assign(const std::string &name, size_t indexes, const std::shared_ptr<Type> &type);

        AnalyzeResult call(const std::string &name, const std::vector<std::shared_ptr<Type>> &args);
    };
}


#endif //COY_SEMANTICANALYZER_H
