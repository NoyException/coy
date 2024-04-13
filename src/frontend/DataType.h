//
// Created by noyex on 24-4-9.
//

#ifndef COY_DATATYPE_H
#define COY_DATATYPE_H

#include <memory>
#include <string>
#include <vector>

namespace coy {

    class DataType : public std::enable_shared_from_this<DataType> {
    public:
        template<typename T>
        bool is() {
            return dynamic_cast<T *>(this) != nullptr;
        }
        
        template<typename T>
        std::shared_ptr<T> as() {
            return std::dynamic_pointer_cast<T>(shared_from_this());
        }

        [[nodiscard]] virtual bool isAssignableFrom(const std::shared_ptr<DataType> &other) const {
            return *this == *other;
        }

        [[nodiscard]] virtual std::shared_ptr<DataType> at(int indexes) = 0;

        [[nodiscard]] virtual std::string toString() const = 0;

        [[nodiscard]] bool operator==(const std::string &other) const {
            return toString() == other;
        }

        [[nodiscard]] bool operator!=(const std::string &other) const {
            return toString() != other;
        }

        [[nodiscard]] bool operator==(const DataType &other) const {
            return toString() == other.toString();
        }

        [[nodiscard]] bool operator!=(const DataType &other) const {
            return toString() != other.toString();
        }
    };

    class ScalarType : public DataType {
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

        [[nodiscard]] bool isAssignableFrom(const std::shared_ptr<DataType> &other) const override {
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

        [[nodiscard]] std::shared_ptr<DataType> at(int indexes) override {
            if (indexes == 0) {
                return shared_from_this();
            }
            return nullptr;
        }

        [[nodiscard]] std::string toString() const override {
            return _name;
        }
    };

    class ArrayType : public DataType {
    private:
        std::shared_ptr<DataType> _base;
        int _dimension;
    public:
        ArrayType(const std::shared_ptr<DataType>& base, int dimension) : _base(base), _dimension(dimension) {}

        ArrayType(const std::shared_ptr<DataType>& base, const std::vector<int>& dimensions)
                : _base(dimensions.size() == 1 ? base : std::make_shared<ArrayType>(base, std::vector<int>(dimensions.begin() + 1, dimensions.end()))),
                  _dimension(dimensions[0]) {}

        [[nodiscard]] std::shared_ptr<DataType> getBase() const { return _base; }

        [[nodiscard]] int getDimension() const { return _dimension; }

        [[nodiscard]] std::shared_ptr<DataType> at(int indexes) override {
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

    class FunctionType : public DataType {
    private:
        std::shared_ptr<DataType> _returnType;
        std::vector<std::shared_ptr<DataType>> _params;
    public:
        FunctionType(std::shared_ptr<DataType> returnType, std::vector<std::shared_ptr<DataType>> params) : _returnType(
                std::move(returnType)), _params(std::move(params)) {}

        [[nodiscard]] std::shared_ptr<DataType> getReturnType() const { return _returnType; }

        [[nodiscard]] std::vector<std::shared_ptr<DataType>> getParams() const { return _params; }

        [[nodiscard]] std::shared_ptr<DataType> at(int indexes) override {
            if (indexes == 0) {
                return shared_from_this();
            }
            return nullptr;
        }

        [[nodiscard]] std::string toString() const override {
            std::string str = _returnType->toString() + "(";
            for (int i = 0; i < _params.size(); ++i) {
                str += _params[i]->toString();
                if (i != _params.size() - 1) {
                    str += ", ";
                }
            }
            str += ")";
            return str;
        }
    };

    class PointerType : public DataType {
    private:
        std::shared_ptr<DataType> _base;
    public:
        explicit PointerType(std::shared_ptr<DataType> base) : _base(std::move(base)) {}

        [[nodiscard]] std::shared_ptr<DataType> getBase() const { return _base; }

        bool isAssignableFrom(const std::shared_ptr<DataType> &other) const override{
            if (auto pointer = std::dynamic_pointer_cast<ArrayType>(other)) {
                return *_base == *pointer->getBase();
            }
            return DataType::isAssignableFrom(other);
        }

        [[nodiscard]] std::shared_ptr<DataType> at(int indexes) override {
            if (indexes == 0) {
                return _base;
            }
            return _base->at(indexes - 1);
        }

        [[nodiscard]] std::string toString() const override {
            return _base->toString() + "*";
        }
    };

} // coy

#endif //COY_DATATYPE_H
