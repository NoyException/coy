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

    void SemanticAnalyzer::addReserved(const std::string &name) {
        _reserved.insert(name);
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
                std::vector<std::shared_ptr<DataType>> paramTypes;
                paramTypes.reserve(params.size());
                // 分析函数参数
                for (const auto &param: params) {
                    std::shared_ptr<DataType> type = std::make_shared<ScalarType>(
                            param->getVariableType()->getTypeName());
                    if (!param->getDimensions().empty())
                        type = std::make_shared<ArrayType>(type, param->getDimensions());
                    if (param->isPointer())
                        type = std::make_shared<PointerType>(type);
                    paramTypes.emplace_back(type);
                }
                // 分析返回值类型
                _returnType = function->getReturnType()->getDataType();
                if (!_returnType->isReturnType()) {
                    return AnalyzeResult::failure("Invalid return type", node);
                }
                auto type = std::make_shared<FunctionType>(_returnType, paramTypes);
                auto result = declare(function->getIdentifier(), type).attach(node);
                if (!result.isSuccess()) {
                    return result;
                }
                // 将函数参数的声明加入到函数作用域
                _scopes.emplace_front();
                for (int i = 0; i < params.size(); ++i) {
                    result = declare(params[i]->getIdentifier(), paramTypes[i]).attach(node);
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
                auto base = declaration->getVariableType()->getTypeName();
                if (base == "void")
                    return AnalyzeResult::failure("Variable cannot have void type", node);
                for (auto &definition: declaration->getDefinitions()) {
                    std::shared_ptr<DataType> type = definition->getDataType();
                    auto result = declare(definition->getIdentifier(), type).attach(definition);
                    if (!result.isSuccess()) {
                        return result;
                    }
                    // 分析初始化值
                    if (definition->getInitialValue() != nullptr) {
                        result = analyze(definition->getInitialValue());
                        if (!result.isSuccess()) {
                            return result;
                        }
                        auto valueType = result.getType();
                        if (valueType == nullptr) {
                            return AnalyzeResult::failure("Unknown type of initial value", node);
                        }
                        result = assign(definition->getIdentifier(), 0,
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
                auto result = analyze(assignment->getLeftValue());
                if (!result.isSuccess()) {
                    return result;
                }
                result = analyze(assignment->getExpression());
                if (!result.isSuccess()) {
                    return result;
                }
                auto type = result.getType();
                if (type == nullptr) {
                    return AnalyzeResult::failure("Unknown type of right value in assignment", node);
                }
                return assign(assignment->getLeftValue()->getIdentifier(),
                              assignment->getLeftValue()->getIndexes().size(), type).attach(node);
            }
            case NodeType::IF: {
                auto ifNode = node->as<NodeIf>();
                auto result = analyze(ifNode->getCondition());
                if (!result.isSuccess()) {
                    return result;
                }
                auto tmp = result.getType();
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

                result = analyze(ifNode->getThen());
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
                auto result = analyze(whileNode->getCondition());
                if (!result.isSuccess()) {
                    return result;
                }
                auto tmp = result.getType();
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
                if (expr == nullptr) {
                    if (*_returnType != "void") {
                        return AnalyzeResult::failure("Return type mismatch", node);
                    }
                    return AnalyzeResult::success();
                }
                auto result = analyze(expr);
                if (!result.isSuccess()) {
                    return result;
                }
                auto returnType = result.getType();
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
                std::vector<std::shared_ptr<DataType>> args;
                for (const auto &arg: functionCall->getArguments()) {
                    auto result = analyze(arg);
                    if (!result.isSuccess()) {
                        return result;
                    }
                    auto type = result.getType();
                    if (type == nullptr) {
                        return AnalyzeResult::failure("Unknown type of argument in function call", node);
                    }
                    args.push_back(type);
                }
                return call(functionCall->getIdentifier(), args).attach(node);
            }
            case NodeType::LEFT_VALUE: {
                auto leftValue = node->as<NodeLeftValue>();
                auto name = leftValue->getIdentifier()->getName();
                auto indexes = leftValue->getIndexes();
                auto searchResult = searchScope(name);
                auto type = searchResult.second->at((int) indexes.size());
                if (type == nullptr) {
                    return AnalyzeResult::failure("Unknown type of left value", node);
                }
                leftValue->getIdentifier()->setUniqueName(searchResult.first);
                for (const auto &index: indexes) {
                    auto result = analyze(index);
                    if (!result.isSuccess()) {
                        return result;
                    }
                    auto indexType = result.getType();
                    if (indexType == nullptr) {
                        return AnalyzeResult::failure("Unknown type of index in left value", node);
                    }
                    if (*indexType != "int") {
                        return AnalyzeResult::failure("Index in left value must be an integer", node);
                    }
                }
                return AnalyzeResult::success(type);
            }
            case NodeType::INTEGER: {
                return AnalyzeResult::success(std::make_shared<ScalarType>("int"));
            }
            case NodeType::FLOAT: {
                return AnalyzeResult::success(std::make_shared<ScalarType>("float"));
            }
            case NodeType::UNARY_OPERATOR: {
                auto unaryOperator = node->as<NodeUnaryOperator>();
                auto result = analyze(unaryOperator->getOperand());
                if (!result.isSuccess()) {
                    return result;
                }
                auto type = result.getType();
                if (type == nullptr) {
                    return AnalyzeResult::failure("Unknown type of expression", node);
                }
                return AnalyzeResult::success();
            }
            case NodeType::BINARY_OPERATOR: {
                auto binaryOperator = node->as<NodeBinaryOperator>();
                auto result = analyze(binaryOperator->getLhs());
                if (!result.isSuccess()) {
                    return result;
                }
                auto leftType = result.getType();
                if (leftType == nullptr) {
                    return AnalyzeResult::failure("Unknown type of left operand", node);
                }
                result = analyze(binaryOperator->getRhs());
                if (!result.isSuccess()) {
                    return result;
                }
                auto rightType = result.getType();
                if (rightType == nullptr) {
                    return AnalyzeResult::failure("Unknown type of right operand", node);
                }
                if (leftType->isAssignableFrom(rightType))
                    return AnalyzeResult::success(leftType);
                if (rightType->isAssignableFrom(leftType))
                    return AnalyzeResult::success(rightType);
                return AnalyzeResult::failure("DataType mismatch in binary operator", node);
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

    std::pair<std::string, std::shared_ptr<DataType>> SemanticAnalyzer::searchScope(const std::string &name) {
        for (const auto &scope: _scopes) {
            auto it = scope.find(name);
            if (it != scope.end()) {
                return it->second;
            }
        }
        return {"", nullptr};
    }

    AnalyzeResult SemanticAnalyzer::declare(const std::shared_ptr<NodeIdentifier> &identifier,
                                            const std::shared_ptr<DataType> &type) {
        auto name = identifier->getName();
        if (_scopes.front().count(name)) {
            return AnalyzeResult::failure("Variable " + name + " is already declared in this scope");
        }
        std::string uniqueName = type->is<FunctionType>()
                                 ? (_reserved.count(name)
                                    ? name
                                    : "func_" + std::to_string(_functionId++)
                                 )
                                 : "var_" + std::to_string(_variableId++);
        identifier->setUniqueName(uniqueName);
        _scopes.front().emplace(name, std::make_pair(uniqueName, type));
        return AnalyzeResult::success();
    }

    AnalyzeResult SemanticAnalyzer::assign(const std::shared_ptr<NodeIdentifier> &identifier, size_t indexes,
                                           const std::shared_ptr<DataType> &type) {
        auto name = identifier->getName();
        auto result = searchScope(name);
        auto declaredType = result.second;
        if (declaredType == nullptr) {
            return AnalyzeResult::failure("Variable " + name + " is not declared");
        }
        identifier->setUniqueName(result.first);
        auto leftType = declaredType->at((int) indexes);
        if (leftType->as<ScalarType>() == nullptr && leftType->as<PointerType>() == nullptr) {
            return AnalyzeResult::failure("Left value must be a scalar or a pointer");
        }
        if (!leftType->isAssignableFrom(type)) {
            return AnalyzeResult::failure("DataType mismatch in assignment");
        }
        return AnalyzeResult::success();
    }

    AnalyzeResult SemanticAnalyzer::call(const std::shared_ptr<NodeIdentifier> &identifier,
                                         const std::vector<std::shared_ptr<DataType>> &args) {
        auto name = identifier->getName();
        auto searchResult = searchScope(name);
        auto tmp = searchResult.second;
        if (tmp == nullptr) {
            return AnalyzeResult::failure("IRFunction " + name + " is not declared");
        }
        identifier->setUniqueName(searchResult.first);
        auto declaredType = tmp->as<FunctionType>();
        if (declaredType == nullptr) {
            return AnalyzeResult::failure(name + " is not a function");
        }
        auto declaredArgs = declaredType->getParams();
        if (args.size() != declaredArgs.size()) {
            return AnalyzeResult::failure("Argument count mismatch");
        }
        for (int i = 0; i < args.size(); ++i) {
            if (!declaredArgs[i]->isAssignableFrom(args[i])) {
                return AnalyzeResult::failure("Argument type mismatch");
            }
        }
        return AnalyzeResult::success(declaredType->getReturnType());
    }
}