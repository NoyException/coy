//
// Created by noyex on 24-4-10.
//

#ifndef COY_IRDATATYPE_H
#define COY_IRDATATYPE_H

#include <memory>
#include <vector>
#include <string>

namespace coy {

    class IRDataType {
    public:
        virtual ~IRDataType() = default;

        [[nodiscard]] virtual int maxDimension() const {
            return 0;
        }

        [[nodiscard]] virtual std::string toString() const = 0;
    };

    class IREmptyType : public IRDataType {
    public:
        [[nodiscard]] std::string toString() const override {
            return "()";
        }
    };

    class IRInteger32Type : public IRDataType {
    public:
        [[nodiscard]] std::string toString() const override {
            return "i32";
        }
    };

    class IRArrayType : public IRDataType {
    private:
        std::shared_ptr<IRDataType> _elementType;
        std::vector<int> _dimensions;
    public:
        IRArrayType(std::shared_ptr<IRDataType> elementType, std::vector<int> dimensions)
                : _elementType(std::move(elementType)), _dimensions(std::move(dimensions)) {}

        [[nodiscard]] const std::shared_ptr<IRDataType> &getElementType() const {
            return _elementType;
        }

        [[nodiscard]] const std::vector<int> &getDimensions() const {
            return _dimensions;
        }

        [[nodiscard]] int maxDimension() const override {
            return _dimensions.size();
        }

        [[nodiscard]] std::string toString() const override {
            return _elementType->toString() + "*";
        }
    };

    class IRPointerType : public IRDataType {
    private:
        std::shared_ptr<IRDataType> _pointedType;
    public:
        explicit IRPointerType(std::shared_ptr<IRDataType> pointedType) : _pointedType(std::move(pointedType)) {}

        [[nodiscard]] const std::shared_ptr<IRDataType> &getPointedType() const {
            return _pointedType;
        }

        [[nodiscard]] int maxDimension() const override {
            return 1+(_pointedType->maxDimension());
        }

        [[nodiscard]] std::string toString() const override {
            if (auto array = std::dynamic_pointer_cast<IRArrayType>(_pointedType)) {
                return array->toString();
            }
            return _pointedType->toString() + "*";
        }
    };

    class IRFunctionType : public IRDataType {
    private:
        std::shared_ptr<IRDataType> _returnType;
        std::vector<std::shared_ptr<IRDataType>> _paramTypes;
    public:
        IRFunctionType(std::shared_ptr<IRDataType> returnType, std::vector<std::shared_ptr<IRDataType>> paramTypes)
                : _returnType(std::move(returnType)), _paramTypes(std::move(paramTypes)) {}

        [[nodiscard]] const std::shared_ptr<IRDataType> &getReturnType() const {
            return _returnType;
        }

        [[nodiscard]] const std::vector<std::shared_ptr<IRDataType>> &getParamTypes() const {
            return _paramTypes;
        }

        [[nodiscard]] std::string toString() const override {
            auto str = "fn " + _returnType->toString() + "(";
            for (auto &param: _paramTypes) {
                str += param->toString() + ", ";
            }
            if (!_paramTypes.empty()) {
                str.pop_back();
                str.pop_back();
            }
            str += ")";
            return str;
        }
    };

} // coy

#endif //COY_IRDATATYPE_H
