//
// Created by noyex on 24-4-4.
//

#include "SemanticAnalyzer.h"

#include <algorithm>
#include <sstream>

namespace coy {

    std::vector<std::string> split(const std::string &s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    SemanticAnalyzer::SemanticAnalyzer() {
        _scopes.emplace_back();
    }

    AnalyzeResult SemanticAnalyzer::analyze(const std::shared_ptr<Node> &node) {
        switch (node->getType()) {
            case NodeType::PROGRAM: {
                auto program = node->as<NodeProgram>();
                for (const auto &n: program->getNodes()) {
                    auto result = analyze(n);
                    if (!result.isSuccess()) {
                        return result;
                    }
                }
                return AnalyzeResult::success();
            }
            case NodeType::BLOCK: {
                _scopes.emplace_front();
                auto block = node->as<NodeBlock>();
                for (const auto &statement: block->getStatements()) {
                    auto result = analyze(statement);
                    if (!result.isSuccess()) {
                        return result;
                    }
                }
                _scopes.pop_front();
                return AnalyzeResult::success();
            }
            case NodeType::FUNCTION: {
                auto function = node->as<NodeFunction>();
                auto params = function->getParams();
                std::vector<std::shared_ptr<Type>> paramTypes;
                paramTypes.reserve(params.size());
                // 分析函数参数
                for (const auto &param: params) {
                    std::shared_ptr<Type> type = std::make_shared<ScalarType>(
                            param->getVariableType()->getDataType());
                    if (!param->getDimensions().empty())
                        type = std::make_shared<ArrayType>(type, param->getDimensions());
                    if (param->isPointer())
                        type = std::make_shared<PointerType>(type);
                    paramTypes.emplace_back(type);
                }
                // 分析返回值类型
                _returnType = std::make_shared<ScalarType>(function->getReturnType()->getDataType());
                if (!_returnType->isReturnType()) {
                    return AnalyzeResult::failure("Invalid return type", node);
                }
                auto type = std::make_shared<FunctionType>(_returnType, paramTypes);
                auto result = declare(function->getName()->getName(), type).attach(node);
                if (!result.isSuccess()) {
                    return result;
                }
                // 将函数参数的声明加入到函数作用域
                _scopes.emplace_front();
                for (int i = 0; i < params.size(); ++i) {
                    result = declare(params[i]->getIdentifier()->getName(), paramTypes[i]).attach(node);
                    if (!result.isSuccess()) {
                        return result;
                    }
                }
                // 为了让函数参数的作用域在函数体内生效，不直接调用analyze(function->getBody())
                auto block = function->getBody();
                for (const auto &statement: block->getStatements()) {
                    result = analyze(statement);
                    if (!result.isSuccess()) {
                        return result;
                    }
                }
                _scopes.pop_front();
                _returnType = nullptr;
                return result;
            }
            case NodeType::DECLARATION: {
                auto declaration = node->as<NodeDeclaration>();
                auto base = declaration->getVariableType()->getDataType();
                if (base == "void")
                    return AnalyzeResult::failure("Variable cannot have void type", node);
                for (auto &definition: declaration->getDefinitions()) {
                    std::shared_ptr<Type> type = std::make_shared<ScalarType>(base);
                    if (!definition->getDimensions().empty())
                        type = std::make_shared<ArrayType>(type, definition->getDimensions());
                    auto result = declare(definition->getIdentifier()->getName(), type).attach(definition);
                    if (!result.isSuccess()) {
                        return result;
                    }
                    // 分析初始化值
                    if (definition->getInitialValue() != nullptr) {
                        result = analyze(definition->getInitialValue());
                        if (!result.isSuccess()) {
                            return result;
                        }
                        auto valueType = getType(definition->getInitialValue());
                        if (valueType == nullptr) {
                            return AnalyzeResult::failure("Unknown type of initial value", node);
                        }
                        result = assign(definition->getIdentifier()->getName(), 0,
                                        valueType).attach(definition);
                        if (!result.isSuccess()) {
                            return result;
                        }
                    }
                }
                return AnalyzeResult::success();
            }
            case NodeType::ASSIGNMENT: {
                auto assignment = node->as<NodeAssignment>();
                auto name = assignment->getLeft()->getIdentifier()->getName();
                auto indexes = assignment->getLeft()->getIndexes();
                for (const auto &index: indexes) {
                    auto indexType = getType(index);
                    if (indexType == nullptr) {
                        return AnalyzeResult::failure("Unknown type of index in left value", node);
                    }
                    if (*indexType != "int") {
                        return AnalyzeResult::failure("Index in left value must be an integer", node);
                    }
                }
                auto result = analyze(assignment->getExpression());
                if (!result.isSuccess()) {
                    return result;
                }
                auto type = getType(assignment->getExpression());
                if (type == nullptr) {
                    return AnalyzeResult::failure("Unknown type of right value in assignment", node);
                }
                return assign(name, indexes.size(), type).attach(node);
            }
            case NodeType::IF: {
                auto ifNode = node->as<NodeIf>();
                auto tmp = getType(ifNode->getCondition());
                if (tmp == nullptr) {
                    return AnalyzeResult::failure("Unknown type of condition in if statement", node);
                }
                auto conditionType = tmp->as<ScalarType>();
                if (conditionType == nullptr) {
                    return AnalyzeResult::failure("Condition in if statement must be a scalar", node);
                }
                if (!conditionType->isNumeric()) {
                    return AnalyzeResult::failure("Condition in if statement must be numeric", node);
                }

                auto result = analyze(ifNode->getThen());
                if (!result.isSuccess()) {
                    return result;
                }
                if (ifNode->getElse()) {
                    result = analyze(ifNode->getElse());
                    if (!result.isSuccess()) {
                        return result;
                    }
                }
                return AnalyzeResult::success();
            }
            case NodeType::WHILE: {
                auto whileNode = node->as<NodeWhile>();
                auto tmp = getType(whileNode->getCondition());
                if (tmp == nullptr) {
                    return AnalyzeResult::failure("Unknown type of condition in while statement", node);
                }
                auto conditionType = tmp->as<ScalarType>();
                if (conditionType == nullptr) {
                    return AnalyzeResult::failure("Condition in while statement must be a scalar", node);
                }
                return analyze(whileNode->getBody());
            }
            case NodeType::RETURN: {
                auto expr = node->as<NodeReturn>()->getExpression();
                if (!expr) {
                    if (*_returnType != "void") {
                        return AnalyzeResult::failure("Return type mismatch", node);
                    }
                    return AnalyzeResult::success();
                }
                auto returnType = getType(expr);
                if (returnType == nullptr) {
                    return AnalyzeResult::failure("Unknown type of return statement", node);
                }
                if (*returnType != *_returnType) {
                    return AnalyzeResult::failure("Return type mismatch", node);
                }
                return AnalyzeResult::success();
            }
            case NodeType::FUNCTION_CALL: {
                auto functionCall = node->as<NodeFunctionCall>();
                std::vector<std::shared_ptr<Type>> args;
                for (const auto &arg: functionCall->getArguments()) {
                    auto type = getType(arg);
                    if (type == nullptr) {
                        return AnalyzeResult::failure("Unknown type of argument in function call", node);
                    }
                    args.push_back(type);
                }
                return call(functionCall->getIdentifier()->getName(), args).attach(node);
            }
            case NodeType::LEFT_VALUE:{
                auto leftValue = node->as<NodeLeftValue>();
                auto name = leftValue->getIdentifier()->getName();
                auto indexes = leftValue->getIndexes();
                auto type = getType(name)->at((int) indexes.size());
                if (type == nullptr) {
                    return AnalyzeResult::failure("Unknown type of left value", node);
                }
                for (const auto &index: indexes) {
                    auto indexType = getType(index);
                    if (indexType == nullptr) {
                        return AnalyzeResult::failure("Unknown type of index in left value", node);
                    }
                    if (*indexType != "int") {
                        return AnalyzeResult::failure("Index in left value must be an integer", node);
                    }
                }
                return AnalyzeResult::success();
            }
            case NodeType::INTEGER:
            case NodeType::FLOAT:
            case NodeType::UNARY_OPERATOR:
            case NodeType::BINARY_OPERATOR: {
                auto type = getType(node->as<NodeTyped>());
                if (type == nullptr) {
                    return AnalyzeResult::failure("Unknown type of expression", node);
                }
                return AnalyzeResult::success();
            }
            case NodeType::BREAK:
            case NodeType::CONTINUE:
                return AnalyzeResult::success();
            default:
                return AnalyzeResult::failure("Unknown node type", node);
        }
    }

    bool SemanticAnalyzer::isDeclared(const std::string &name) {
        return std::any_of(_scopes.begin(), _scopes.end(), [&name](const auto &scope) {
            return scope.find(name) != scope.end();
        });
    }

    std::shared_ptr<Type> SemanticAnalyzer::getType(const std::string &name) {
        for (const auto &scope: _scopes) {
            auto it = scope.find(name);
            if (it != scope.end()) {
                return it->second;
            }
        }
        return nullptr;
    }

    std::shared_ptr<Type> SemanticAnalyzer::getType(const std::shared_ptr<NodeTyped> &node) {
        switch (node->getType()) {
            case NodeType::DATA_TYPE:
                return std::make_shared<ScalarType>(node->as<NodeDataType>()->getDataType());
            case NodeType::INTEGER:
                return std::make_shared<ScalarType>("int");
            case NodeType::FLOAT:
                return std::make_shared<ScalarType>("float");
            case NodeType::LEFT_VALUE: {
                auto leftValue = node->as<NodeLeftValue>();
                auto tmp = getType(leftValue->getIdentifier()->getName());
                if (tmp == nullptr) {
                    return nullptr;
                }
                return tmp->at((int) leftValue->getIndexes().size());
            }
            case NodeType::UNARY_OPERATOR:
                return getType(node->as<NodeUnaryOperator>()->getNode());
            case NodeType::BINARY_OPERATOR: {
                auto left = getType(node->as<NodeBinaryOperator>()->getLeft());
                auto right = getType(node->as<NodeBinaryOperator>()->getRight());
                if (left == nullptr || right == nullptr) {
                    return nullptr;
                }
                if (left->isAssignableFrom(right))
                    return left;
                if (right->isAssignableFrom(left))
                    return right;
                return nullptr;
            }
            case NodeType::FUNCTION_CALL: {
                auto type = getType(node->as<NodeFunctionCall>()->getIdentifier()->getName())->as<FunctionType>();
                if (type == nullptr) {
                    return nullptr;
                }
                return type->getReturnType();
            }
            default:
                return nullptr;
        }
    }

    AnalyzeResult SemanticAnalyzer::declare(const std::string &name, const std::shared_ptr<Type> &type) {
        if (_scopes.front().count(name)) {
            return AnalyzeResult::failure("Variable " + name + " is already declared in this scope");
        }
        _scopes.front().emplace(name, type);
        return AnalyzeResult::success();
    }

    AnalyzeResult SemanticAnalyzer::assign(const std::string &name, size_t indexes, const std::shared_ptr<Type> &type) {
        auto declaredType = getType(name);
        if (declaredType == nullptr) {
            return AnalyzeResult::failure("Variable " + name + " is not declared");
        }
        auto leftType = declaredType->at((int) indexes)->as<ScalarType>();
        if (leftType == nullptr) {
            return AnalyzeResult::failure("Left value must be a scalar");
        }
        if (!leftType->isAssignableFrom(type)) {
            return AnalyzeResult::failure("Type mismatch in assignment");
        }
        return AnalyzeResult::success();
    }

    AnalyzeResult SemanticAnalyzer::call(const std::string &name, const std::vector<std::shared_ptr<Type>> &args) {
        auto tmp = getType(name);
        if (tmp == nullptr) {
            return AnalyzeResult::failure("Function " + name + " is not declared");
        }
        auto declaredType = tmp->as<FunctionType>();
        if (declaredType == nullptr) {
            return AnalyzeResult::failure(name + " is not a function");
        }
        auto declaredArgs = declaredType->getArgs();
        if (args.size() != declaredArgs.size()) {
            return AnalyzeResult::failure("Argument count mismatch");
        }
        for (int i = 0; i < args.size(); ++i) {
            if (!declaredArgs[i]->isAssignableFrom(args[i])) {
                return AnalyzeResult::failure("Argument type mismatch");
            }
        }
        return AnalyzeResult::success();
    }
}