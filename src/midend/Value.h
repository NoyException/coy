//
// Created by noyex on 24-4-7.
//

#ifndef COY_VALUE_H
#define COY_VALUE_H

#include <memory>
#include <stdexcept>
#include <vector>

#include "IRDataType.h"

namespace coy {

    enum class ValueType {
        VALUE,
        NONE,
        INTEGER,
        FLOAT,
        LABEL,
        PARAMETER,
    };

    class Value : public std::enable_shared_from_this<Value> {
    private:
        ValueType _type;
    public:
        static constexpr ValueType TYPE = ValueType::VALUE;

        explicit Value(ValueType type) : _type(type) {}

        [[nodiscard]] ValueType getType() const {
            return _type;
        }

        template<class T>
        bool is() const {
            return isAssignableFrom(_type, std::remove_pointer_t<T>::TYPE);
        }

        template<class T>
        std::shared_ptr<T> as() {
            if (is<T>()) {
                return std::dynamic_pointer_cast<T>(shared_from_this());
            } else {
                throw std::runtime_error("Value is not of _type " + std::string(typeid(T).name()));
            }
        }

        virtual std::string toString() const = 0;
    };

    class None : public Value {
    public:
        static constexpr ValueType TYPE = ValueType::NONE;

        explicit None() : Value(TYPE) {}

        static const std::shared_ptr<None> INSTANCE;

        [[nodiscard]] std::string toString() const override {
            return "()";
        }
    };

    class Integer : public Value {
    private:
        int _value;
    public:
        static constexpr ValueType TYPE = ValueType::INTEGER;

        explicit Integer(int value) : Value(TYPE), _value(value) {}

        [[maybe_unused]] static const std::shared_ptr<Integer> ZERO;

        [[maybe_unused]] static const std::shared_ptr<Integer> ONE;

        [[maybe_unused]] static const std::shared_ptr<Integer> MINUS_ONE;

        [[nodiscard]] int getValue() const {
            return _value;
        }

        [[nodiscard]] std::string toString() const override {
            return std::to_string(_value);
        }
    };

    class Float : public Value {
    private:
        float _value;
    public:
        static constexpr ValueType TYPE = ValueType::FLOAT;

        explicit Float(float value) : Value(TYPE), _value(value) {}

        [[nodiscard]] float getValue() const {
            return _value;
        }

        [[nodiscard]] std::string toString() const override {
            return std::to_string(_value);
        }
    };

    class Label : public Value {
    private:
        std::string _name;
    public:
        static constexpr ValueType TYPE = ValueType::LABEL;

        explicit Label(std::string name) : Value(TYPE), _name(std::move(name)) {}

        [[nodiscard]] std::string getName() const {
            return _name;
        }

        [[nodiscard]] std::string toString() const override {
            return "%L" + _name;
        }
    };

    class Parameter : public Value {
    private:
        std::string _uniqueName;
        std::shared_ptr<IRDataType> _dataType;
    public:
        static constexpr ValueType TYPE = ValueType::PARAMETER;

        Parameter(std::string uniqueName, std::shared_ptr<IRDataType> dataType)
                : Value(TYPE), _uniqueName(std::move(uniqueName)), _dataType(std::move(dataType)) {}

        [[nodiscard]] std::string getUniqueName() const {
            return _uniqueName;
        }

        [[nodiscard]] const std::shared_ptr<IRDataType> &getDataType() const {
            return _dataType;
        }

        [[nodiscard]] std::string toString() const override {
            return "%" + _uniqueName;
        }
    };

} // coy

#endif //COY_VALUE_H
