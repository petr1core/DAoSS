//
// Created by пк on 21.11.2025.
// Конвертер C++ AST в JSON
//

#ifndef AISDLAB_CPPASTPARSERTOJSON_H
#define AISDLAB_CPPASTPARSERTOJSON_H

#include <iostream>
#include "../Scripts/json.hpp"
#include "../CPPAst/CppAst.h"
#include "../CPPAst/CppExpr.h"
#include "../CPPAst/CppStmt.h"
#include "../CPPAst/CppDecl.h"

using json = nlohmann::json;

class CppAstToJsonConverter {
public:
    json convertProgram(const std::unique_ptr<CppProgram> &program) {
        if (!program) {
            return json{{"error", "Null program"}};
        }

        json result;
        result["type"] = "Program";
        result["name"] = program->name;
        
        if (program->body) {
            result["body"] = convertCompoundStmt(program->body);
        } else {
            result["body"] = json::object();
        }

        return result;
    }

private:
    // Перегрузка для raw pointer
    json convertCompoundStmt(CppCompoundStmt *stmt) {
        if (!stmt) {
            return json{{"type", "Block"}, {"statements", json::array()}};
        }
        
        json result;
        result["type"] = "Block";
        
        json statements = json::array();
        for (const auto &s : stmt->statements) {
            if (s) {
                statements.push_back(convertStmt(s));
            }
        }
        result["statements"] = statements;
        
        return result;
    }

    json convertCompoundStmt(const std::unique_ptr<CppCompoundStmt> &stmt) {
        if (!stmt) {
            return json{{"type", "Block"}, {"statements", json::array()}};
        }
        
        return convertCompoundStmt(stmt.get());
    }

    // Вспомогательный метод для конвертации CppDecl (который наследуется от CppStmt)
    json convertDecl(const std::unique_ptr<CppDecl> &decl) {
        if (!decl) {
            return json{{"type", "EmptyDecl"}};
        }
        // CppDecl наследуется от CppStmt, используем raw pointer для проверки типа
        return convertStmtPtr(decl.get());
    }

    // Перегрузка для raw pointer 
    json convertStmtPtr(CppStmt *stmt) {
        if (!stmt) {
            return json{{"type", "EmptyStmt"}};
        }

        // Копируем логику из convertStmt, но используем raw pointer
        // Expressions
        if (auto exprStmt = dynamic_cast<CppExprStmt *>(stmt)) {
            return convertExprStmt(exprStmt);
        }
        
        // Compound
        if (auto compound = dynamic_cast<CppCompoundStmt *>(stmt)) {
            return convertCompoundStmt(compound);
        }
        
        // Control flow
        if (auto ifStmt = dynamic_cast<CppIfStmt *>(stmt)) {
            return convertIfStmt(ifStmt);
        }
        if (auto whileStmt = dynamic_cast<CppWhileStmt *>(stmt)) {
            return convertWhileStmt(whileStmt);
        }
        if (auto forStmt = dynamic_cast<CppForStmt *>(stmt)) {
            return convertForStmt(forStmt);
        }
        if (auto rangeFor = dynamic_cast<CppRangeForStmt *>(stmt)) {
            return convertRangeForStmt(rangeFor);
        }
        if (auto doWhile = dynamic_cast<CppDoWhileStmt *>(stmt)) {
            return convertDoWhileStmt(doWhile);
        }
        if (auto switchStmt = dynamic_cast<CppSwitchStmt *>(stmt)) {
            return convertSwitchStmt(switchStmt);
        }
        
        // Other statements
        if (auto returnStmt = dynamic_cast<CppReturnStmt *>(stmt)) {
            return convertReturnStmt(returnStmt);
        }
        if (auto breakStmt = dynamic_cast<CppBreakStmt *>(stmt)) {
            return json{{"type", "BreakStmt"}};
        }
        if (auto continueStmt = dynamic_cast<CppContinueStmt *>(stmt)) {
            return json{{"type", "ContinueStmt"}};
        }
        
        // Declarations
        if (auto varDecl = dynamic_cast<CppVarDecl *>(stmt)) {
            return convertVarDecl(varDecl);
        }
        if (auto assignStmt = dynamic_cast<CppAssignStmt *>(stmt)) {
            return convertAssignStmt(assignStmt);
        }
        if (auto funcDecl = dynamic_cast<CppFunctionDecl *>(stmt)) {
            return convertFunctionDecl(funcDecl);
        }
        if (auto classDecl = dynamic_cast<CppClassDecl *>(stmt)) {
            return convertClassDecl(classDecl);
        }
        if (auto namespaceDecl = dynamic_cast<CppNamespaceDecl *>(stmt)) {
            return convertNamespaceDecl(namespaceDecl);
        }
        if (auto enumDecl = dynamic_cast<CppEnumDecl *>(stmt)) {
            return convertEnumDecl(enumDecl);
        }
        if (auto typedefDecl = dynamic_cast<CppTypedefDecl *>(stmt)) {
            return convertTypedefDecl(typedefDecl);
        }
        if (auto usingDecl = dynamic_cast<CppUsingDecl *>(stmt)) {
            return convertUsingDecl(usingDecl);
        }
        if (auto usingAlias = dynamic_cast<CppUsingAliasDecl *>(stmt)) {
            return convertUsingAliasDecl(usingAlias);
        }
        if (auto templateDecl = dynamic_cast<CppTemplateDecl *>(stmt)) {
            return convertTemplateDecl(templateDecl);
        }
        if (auto methodDecl = dynamic_cast<CppMethodDecl *>(stmt)) {
            return convertMethodDecl(methodDecl);
        }
        if (auto constructorDecl = dynamic_cast<CppConstructorDecl *>(stmt)) {
            return convertConstructorDecl(constructorDecl);
        }
        if (auto operatorDecl = dynamic_cast<CppOperatorDecl *>(stmt)) {
            return convertOperatorDecl(operatorDecl);
        }
        if (auto fieldDecl = dynamic_cast<CppFieldDecl *>(stmt)) {
            return convertFieldDecl(fieldDecl);
        }
        if (auto preprocessor = dynamic_cast<CppPreprocessorDirective *>(stmt)) {
            return convertPreprocessor(preprocessor);
        }
        if (auto tryStmt = dynamic_cast<CppTryStmt *>(stmt)) {
            return convertTryStmt(tryStmt);
        }
        
        return json{{"type", "UnknownStmt"}};
    }

    json convertStmt(const std::unique_ptr<CppStmt> &stmt) {
        if (!stmt) {
            return json{{"type", "EmptyStmt"}};
        }

        // Expressions
        if (auto exprStmt = dynamic_cast<CppExprStmt *>(stmt.get())) {
            return convertExprStmt(exprStmt);
        }
        
        // Compound
        if (auto compound = dynamic_cast<CppCompoundStmt *>(stmt.get())) {
            return convertCompoundStmt(compound);
        }
        
        // Control flow
        if (auto ifStmt = dynamic_cast<CppIfStmt *>(stmt.get())) {
            return convertIfStmt(ifStmt);
        }
        if (auto whileStmt = dynamic_cast<CppWhileStmt *>(stmt.get())) {
            return convertWhileStmt(whileStmt);
        }
        if (auto forStmt = dynamic_cast<CppForStmt *>(stmt.get())) {
            return convertForStmt(forStmt);
        }
        if (auto rangeFor = dynamic_cast<CppRangeForStmt *>(stmt.get())) {
            return convertRangeForStmt(rangeFor);
        }
        if (auto doWhile = dynamic_cast<CppDoWhileStmt *>(stmt.get())) {
            return convertDoWhileStmt(doWhile);
        }
        if (auto switchStmt = dynamic_cast<CppSwitchStmt *>(stmt.get())) {
            return convertSwitchStmt(switchStmt);
        }
        
        // Other statements
        if (auto returnStmt = dynamic_cast<CppReturnStmt *>(stmt.get())) {
            return convertReturnStmt(returnStmt);
        }
        if (auto breakStmt = dynamic_cast<CppBreakStmt *>(stmt.get())) {
            return json{{"type", "BreakStmt"}};
        }
        if (auto continueStmt = dynamic_cast<CppContinueStmt *>(stmt.get())) {
            return json{{"type", "ContinueStmt"}};
        }
        
        // Declarations
        if (auto varDecl = dynamic_cast<CppVarDecl *>(stmt.get())) {
            return convertVarDecl(varDecl);
        }
        if (auto assignStmt = dynamic_cast<CppAssignStmt *>(stmt.get())) {
            return convertAssignStmt(assignStmt);
        }
        if (auto funcDecl = dynamic_cast<CppFunctionDecl *>(stmt.get())) {
            return convertFunctionDecl(funcDecl);
        }
        if (auto classDecl = dynamic_cast<CppClassDecl *>(stmt.get())) {
            return convertClassDecl(classDecl);
        }
        if (auto namespaceDecl = dynamic_cast<CppNamespaceDecl *>(stmt.get())) {
            return convertNamespaceDecl(namespaceDecl);
        }
        if (auto enumDecl = dynamic_cast<CppEnumDecl *>(stmt.get())) {
            return convertEnumDecl(enumDecl);
        }
        if (auto typedefDecl = dynamic_cast<CppTypedefDecl *>(stmt.get())) {
            return convertTypedefDecl(typedefDecl);
        }
        if (auto usingDecl = dynamic_cast<CppUsingDecl *>(stmt.get())) {
            return convertUsingDecl(usingDecl);
        }
        if (auto usingAlias = dynamic_cast<CppUsingAliasDecl *>(stmt.get())) {
            return convertUsingAliasDecl(usingAlias);
        }
        if (auto templateDecl = dynamic_cast<CppTemplateDecl *>(stmt.get())) {
            return convertTemplateDecl(templateDecl);
        }
        if (auto methodDecl = dynamic_cast<CppMethodDecl *>(stmt.get())) {
            return convertMethodDecl(methodDecl);
        }
        if (auto constructorDecl = dynamic_cast<CppConstructorDecl *>(stmt.get())) {
            return convertConstructorDecl(constructorDecl);
        }
        if (auto operatorDecl = dynamic_cast<CppOperatorDecl *>(stmt.get())) {
            return convertOperatorDecl(operatorDecl);
        }
        if (auto fieldDecl = dynamic_cast<CppFieldDecl *>(stmt.get())) {
            return convertFieldDecl(fieldDecl);
        }
        if (auto preprocessor = dynamic_cast<CppPreprocessorDirective *>(stmt.get())) {
            return convertPreprocessor(preprocessor);
        }
        if (auto tryStmt = dynamic_cast<CppTryStmt *>(stmt.get())) {
            return convertTryStmt(tryStmt);
        }
        
        return json{{"type", "UnknownStmt"}};
    }

    json convertExprStmt(CppExprStmt *stmt) {
        json result;
        result["type"] = "ExprStmt";
        if (stmt->expression) {
            result["expression"] = convertExpr(stmt->expression);
        }
        return result;
    }

    json convertIfStmt(CppIfStmt *stmt) {
        json result;
        result["type"] = "IfStmt";
        result["condition"] = convertExpr(stmt->condition);
        result["thenBranch"] = convertStmt(stmt->thenBranch);
        if (stmt->elseBranch) {
            result["elseBranch"] = convertStmt(stmt->elseBranch);
        }
        return result;
    }

    json convertWhileStmt(CppWhileStmt *stmt) {
        json result;
        result["type"] = "WhileStmt";
        result["condition"] = convertExpr(stmt->condition);
        result["body"] = convertStmt(stmt->body);
        return result;
    }

    json convertDoWhileStmt(CppDoWhileStmt *stmt) {
        json result;
        result["type"] = "DoWhileStmt";
        result["body"] = convertStmt(stmt->body);
        result["condition"] = convertExpr(stmt->condition);
        return result;
    }

    json convertForStmt(CppForStmt *stmt) {
        json result;
        result["type"] = "ForStmt";
        if (stmt->init) {
            result["init"] = convertStmt(stmt->init);
        }
        if (stmt->condition) {
            result["condition"] = convertExpr(stmt->condition);
        }
        if (stmt->increment) {
            result["increment"] = convertExpr(stmt->increment);
        }
        if (stmt->body) {
            result["body"] = convertStmt(stmt->body);
        }
        return result;
    }

    json convertRangeForStmt(CppRangeForStmt *stmt) {
        json result;
        result["type"] = "RangeForStmt";
        if (stmt->init) {
            result["init"] = convertStmt(stmt->init);
        }
        if (stmt->range) {
            result["range"] = convertExpr(stmt->range);
        }
        if (stmt->body) {
            result["body"] = convertStmt(stmt->body);
        }
        return result;
    }

    json convertSwitchStmt(CppSwitchStmt *stmt) {
        json result;
        result["type"] = "SwitchStmt";
        result["condition"] = convertExpr(stmt->condition);
        
        json cases = json::array();
        for (const auto &caseStmt : stmt->cases) {
            if (caseStmt) {
                cases.push_back(convertCaseStmt(caseStmt.get()));
            }
        }
        result["cases"] = cases;
        
        return result;
    }

    json convertCaseStmt(CppCaseStmt *stmt) {
        json result;
        result["type"] = "CaseStmt";
        if (stmt->value) {
            result["value"] = convertExpr(stmt->value);
        }
        
        json body = json::array();
        for (const auto &s : stmt->body) {
            if (s) {
                body.push_back(convertStmt(s));
            }
        }
        result["body"] = body;
        
        return result;
    }

    json convertReturnStmt(CppReturnStmt *stmt) {
        json result;
        result["type"] = "ReturnStmt";
        if (stmt->value) {
            result["value"] = convertExpr(stmt->value);
        }
        return result;
    }

    json convertVarDecl(CppVarDecl *decl) {
        json result;
        result["type"] = "VarDeclStmt";
        result["varType"] = decl->typeName;
        result["name"] = decl->name;
        if (decl->initializer) {
            result["initializer"] = convertExpr(decl->initializer);
        }
        return result;
    }

    json convertAssignStmt(CppAssignStmt *stmt) {
        json result;
        result["type"] = "AssignStmt";
        result["operator"] = stmt->op;
        result["target"] = convertExpr(stmt->target);
        result["value"] = convertExpr(stmt->value);
        return result;
    }

    json convertFunctionDecl(CppFunctionDecl *decl) {
        json result;
        result["type"] = "FunctionDecl";
        result["returnType"] = decl->returnType;
        result["name"] = decl->name;
        
        json params = json::array();
        for (const auto &param : decl->parameters) {
            json p;
            p["type"] = param.typeName;
            p["name"] = param.name;
            if (!param.defaultValue.empty()) {
                p["defaultValue"] = param.defaultValue;
            }
            params.push_back(p);
        }
        result["parameters"] = params;
        
        if (decl->body) {
            result["body"] = convertCompoundStmt(decl->body);
        }
        
        return result;
    }

    json convertClassDecl(CppClassDecl *decl) {
        json result;
        result["type"] = decl->isStruct ? "StructDecl" : (decl->isUnion ? "UnionDecl" : "ClassDecl");
        result["name"] = decl->name;
        
        if (!decl->baseClasses.empty()) {
            json bases = json::array();
            for (size_t i = 0; i < decl->baseClasses.size(); ++i) {
                json base;
                base["name"] = decl->baseClasses[i];
                if (i < decl->baseAccess.size()) {
                    base["access"] = accessSpecifierToString(decl->baseAccess[i]);
                }
                bases.push_back(base);
            }
            result["baseClasses"] = bases;
        }
        
        json members = json::array();
        for (const auto &member : decl->members) {
            if (member) {
                members.push_back(convertDecl(member));
            }
        }
        result["members"] = members;
        
        return result;
    }

    json convertNamespaceDecl(CppNamespaceDecl *decl) {
        json result;
        result["type"] = "NamespaceDecl";
        result["name"] = decl->name;
        if (decl->body) {
            result["body"] = convertCompoundStmt(decl->body);
        }
        return result;
    }

    json convertEnumDecl(CppEnumDecl *decl) {
        json result;
        result["type"] = "EnumDecl";
        result["name"] = decl->name;
        result["isScoped"] = decl->isScoped;
        if (!decl->underlyingType.empty()) {
            result["underlyingType"] = decl->underlyingType;
        }
        
        json enumerators = json::array();
        for (const auto &enumerator : decl->enumerators) {
            json e;
            e["name"] = enumerator.first;
            if (enumerator.second) {
                e["value"] = convertExpr(enumerator.second);
            }
            enumerators.push_back(e);
        }
        result["enumerators"] = enumerators;
        
        return result;
    }

    json convertTypedefDecl(CppTypedefDecl *decl) {
        json result;
        result["type"] = "TypedefDecl";
        result["typeName"] = decl->typeName;
        result["alias"] = decl->alias;
        return result;
    }

    json convertUsingDecl(CppUsingDecl *decl) {
        json result;
        if (decl->isNamespace) {
            result["type"] = "UsingNamespaceDecl";
            result["target"] = decl->target;
        } else {
            result["type"] = "UsingDecl";
            result["name"] = decl->name;
        }
        return result;
    }

    json convertUsingAliasDecl(CppUsingAliasDecl *decl) {
        json result;
        result["type"] = "UsingAliasDecl";
        result["typeName"] = decl->typeName;
        result["alias"] = decl->alias;
        return result;
    }

    json convertTemplateDecl(CppTemplateDecl *decl) {
        json result;
        result["type"] = "TemplateDecl";
        
        json params = json::array();
        for (const auto &param : decl->parameters) {
            json p;
            p["kind"] = param.kind;
            p["name"] = param.name;
            if (!param.defaultType.empty()) {
                p["defaultType"] = param.defaultType;
            }
            if (!param.defaultValue.empty()) {
                p["defaultValue"] = param.defaultValue;
            }
            p["isPack"] = param.isPack;
            params.push_back(p);
        }
        result["templateParameters"] = params;
        
        if (decl->declaration) {
            result["declaration"] = convertDecl(decl->declaration);
        }
        
        return result;
    }

    json convertMethodDecl(CppMethodDecl *decl) {
        json result;
        result["type"] = "MethodDecl";
        result["returnType"] = decl->returnType;
        result["name"] = decl->name;
        result["access"] = accessSpecifierToString(decl->access);
        result["isVirtual"] = decl->isVirtual;
        result["isOverride"] = decl->isOverride;
        result["isConst"] = decl->isConst;
        result["isStatic"] = decl->isStatic;
        
        json params = json::array();
        for (const auto &param : decl->parameters) {
            json p;
            p["type"] = param.typeName;
            p["name"] = param.name;
            if (!param.defaultValue.empty()) {
                p["defaultValue"] = param.defaultValue;
            }
            params.push_back(p);
        }
        result["parameters"] = params;
        
        if (decl->body) {
            result["body"] = convertCompoundStmt(decl->body);
        }
        
        return result;
    }

    json convertConstructorDecl(CppConstructorDecl *decl) {
        json result;
        result["type"] = "ConstructorDecl";
        result["name"] = decl->name;
        result["access"] = accessSpecifierToString(decl->access);
        
        json params = json::array();
        for (const auto &param : decl->parameters) {
            json p;
            p["type"] = param.typeName;
            p["name"] = param.name;
            if (!param.defaultValue.empty()) {
                p["defaultValue"] = param.defaultValue;
            }
            params.push_back(p);
        }
        result["parameters"] = params;
        
        if (!decl->initializers.empty()) {
            json inits = json::array();
            for (const auto &init : decl->initializers) {
                json i;
                i["memberName"] = init.memberName;
                i["isBaseClass"] = init.isBaseClass;
                if (init.value) {
                    i["value"] = convertExpr(init.value);
                }
                inits.push_back(i);
            }
            result["initializers"] = inits;
        }
        
        if (decl->body) {
            result["body"] = convertCompoundStmt(decl->body);
        }
        
        return result;
    }

    json convertOperatorDecl(CppOperatorDecl *decl) {
        json result;
        result["type"] = "OperatorDecl";
        result["returnType"] = decl->returnType;
        result["operatorSymbol"] = decl->operatorSymbol;
        result["access"] = accessSpecifierToString(decl->access);
        result["isConst"] = decl->isConst;
        
        json params = json::array();
        for (const auto &param : decl->parameters) {
            json p;
            p["type"] = param.typeName;
            p["name"] = param.name;
            params.push_back(p);
        }
        result["parameters"] = params;
        
        if (decl->body) {
            result["body"] = convertCompoundStmt(decl->body);
        }
        
        return result;
    }

    json convertFieldDecl(CppFieldDecl *decl) {
        json result;
        result["type"] = "FieldDecl";
        result["typeName"] = decl->typeName;
        result["name"] = decl->name;
        result["access"] = accessSpecifierToString(decl->access);
        result["isMutable"] = decl->isMutable;
        if (decl->initializer) {
            result["initializer"] = convertExpr(decl->initializer);
        }
        return result;
    }

    json convertPreprocessor(CppPreprocessorDirective *decl) {
        json result;
        result["type"] = "PreprocessorDirective";
        result["directive"] = decl->directive;
        result["value"] = decl->value;
        return result;
    }

    json convertTryStmt(CppTryStmt *stmt) {
        json result;
        result["type"] = "TryStmt";
        if (stmt->tryBlock) {
            result["tryBlock"] = convertCompoundStmt(stmt->tryBlock);
        }
        
        json handlers = json::array();
        for (const auto &handler : stmt->handlers) {
            if (handler) {
                handlers.push_back(convertCatchStmt(handler.get()));
            }
        }
        result["handlers"] = handlers;
        
        return result;
    }

    json convertCatchStmt(CppCatchStmt *stmt) {
        json result;
        result["type"] = "CatchStmt";
        result["exceptionType"] = stmt->exceptionType;
        result["exceptionName"] = stmt->exceptionName;
        if (stmt->body) {
            result["body"] = convertCompoundStmt(stmt->body);
        }
        return result;
    }

    json convertExpr(const std::unique_ptr<CppExpr> &expr) {
        if (!expr) {
            return json{{"type", "NullExpr"}};
        }

        try {
            if (auto ident = dynamic_cast<CppIdentifier *>(expr.get())) {
                json result;
                result["type"] = "Identifier";
                result["name"] = ident->name;
                return result;
            }
            
            if (auto strLit = dynamic_cast<CppStringLiteral *>(expr.get())) {
                json result;
                result["type"] = "StringLiteral";
                result["value"] = strLit->value;
                return result;
            }
            
            if (auto intLit = dynamic_cast<CppIntLiteral *>(expr.get())) {
                json result;
                result["type"] = "IntLiteral";
                result["value"] = intLit->value;
                return result;
            }
            
            if (auto realLit = dynamic_cast<CppRealLiteral *>(expr.get())) {
                json result;
                result["type"] = "RealLiteral";
                result["value"] = realLit->value;
                return result;
            }
            
            if (auto boolLit = dynamic_cast<CppBoolLiteral *>(expr.get())) {
                json result;
                result["type"] = "BoolLiteral";
                result["value"] = boolLit->value;
                return result;
            }
            
            if (auto nullptrLit = dynamic_cast<CppNullptrLiteral *>(expr.get())) {
                return json{{"type", "NullptrLiteral"}};
            }
            
            if (auto unaryOp = dynamic_cast<CppUnaryOp *>(expr.get())) {
                json result;
                result["type"] = "UnaryOp";
                result["operator"] = unaryOp->op;
                result["postfix"] = unaryOp->postfix;
                result["operand"] = convertExpr(unaryOp->operand);
                return result;
            }
            
            if (auto binaryOp = dynamic_cast<CppBinaryOp *>(expr.get())) {
                json result;
                result["type"] = "BinaryOp";
                result["operator"] = binOpKindToString(binaryOp->op);
                result["left"] = convertExpr(binaryOp->left);
                result["right"] = convertExpr(binaryOp->right);
                return result;
            }
            
            if (auto ternaryOp = dynamic_cast<CppTernaryOp *>(expr.get())) {
                json result;
                result["type"] = "TernaryOp";
                result["condition"] = convertExpr(ternaryOp->condition);
                result["thenExpr"] = convertExpr(ternaryOp->thenExpr);
                result["elseExpr"] = convertExpr(ternaryOp->elseExpr);
                return result;
            }
            
            if (auto callExpr = dynamic_cast<CppCallExpr *>(expr.get())) {
                json result;
                result["type"] = "CallExpr";
                result["callee"] = convertExpr(callExpr->callee);
                
                json args = json::array();
                for (const auto &arg : callExpr->arguments) {
                    args.push_back(convertExpr(arg));
                }
                result["arguments"] = args;
                return result;
            }
            
            if (auto arrayAccess = dynamic_cast<CppArrayAccessExpr *>(expr.get())) {
                json result;
                result["type"] = "ArrayAccessExpr";
                result["array"] = convertExpr(arrayAccess->array);
                result["index"] = convertExpr(arrayAccess->index);
                return result;
            }
            
            if (auto memberAccess = dynamic_cast<CppMemberAccessExpr *>(expr.get())) {
                json result;
                result["type"] = "MemberAccessExpr";
                result["object"] = convertExpr(memberAccess->object);
                result["member"] = memberAccess->member;
                result["isPointerAccess"] = memberAccess->isPointerAccess;
                return result;
            }
            
            if (auto ptrToMember = dynamic_cast<CppPointerToMemberExpr *>(expr.get())) {
                json result;
                result["type"] = "PointerToMemberExpr";
                result["object"] = convertExpr(ptrToMember->object);
                result["member"] = ptrToMember->member;
                result["isArrow"] = ptrToMember->isArrow;
                return result;
            }
            
            if (auto newExpr = dynamic_cast<CppNewExpr *>(expr.get())) {
                json result;
                result["type"] = "NewExpr";
                result["typeName"] = newExpr->typeName;
                result["isArray"] = newExpr->isArray;
                
                json args = json::array();
                for (const auto &arg : newExpr->arguments) {
                    args.push_back(convertExpr(arg));
                }
                result["arguments"] = args;
                
                if (newExpr->arraySize) {
                    result["arraySize"] = convertExpr(newExpr->arraySize);
                }
                return result;
            }
            
            if (auto deleteExpr = dynamic_cast<CppDeleteExpr *>(expr.get())) {
                json result;
                result["type"] = "DeleteExpr";
                result["isArray"] = deleteExpr->isArray;
                result["operand"] = convertExpr(deleteExpr->operand);
                return result;
            }
            
            if (auto thisExpr = dynamic_cast<CppThisExpr *>(expr.get())) {
                return json{{"type", "ThisExpr"}};
            }
            
            if (auto sizeofExpr = dynamic_cast<CppSizeofExpr *>(expr.get())) {
                json result;
                result["type"] = "SizeofExpr";
                result["isType"] = sizeofExpr->isType;
                result["expression"] = convertExpr(sizeofExpr->expression);
                return result;
            }
            
            if (auto typeidExpr = dynamic_cast<CppTypeidExpr *>(expr.get())) {
                json result;
                result["type"] = "TypeidExpr";
                result["isType"] = typeidExpr->isType;
                result["expression"] = convertExpr(typeidExpr->expression);
                return result;
            }
            
            if (auto templateId = dynamic_cast<CppTemplateIdExpr *>(expr.get())) {
                json result;
                result["type"] = "TemplateIdExpr";
                result["templateName"] = templateId->templateName;
                
                json args = json::array();
                for (const auto &arg : templateId->templateArgs) {
                    args.push_back(convertExpr(arg));
                }
                result["templateArguments"] = args;
                return result;
            }
            
            if (auto lambda = dynamic_cast<CppLambdaExpr *>(expr.get())) {
                json result;
                result["type"] = "LambdaExpr";
                
                json captures = json::array();
                for (const auto &capture : lambda->captures) {
                    json c;
                    c["name"] = capture.name;
                    c["byReference"] = capture.byReference;
                    c["isThis"] = capture.isThis;
                    if (capture.init) {
                        c["init"] = convertExpr(capture.init);
                    }
                    captures.push_back(c);
                }
                result["captures"] = captures;
                
                json params = json::array();
                for (const auto &param : lambda->parameters) {
                    if (param) {
                        json p;
                        p["typeName"] = param->typeName;
                        p["name"] = param->name;
                        params.push_back(p);
                    }
                }
                result["parameters"] = params;
                
                if (lambda->returnType) {
                    result["returnType"] = convertExpr(lambda->returnType);
                }
                
                if (lambda->body) {
                    result["body"] = convertCompoundStmt(lambda->body);
                }
                
                return result;
            }
            
            if (auto cast = dynamic_cast<CppCastExpr *>(expr.get())) {
                json result;
                result["type"] = "CastExpr";
                result["castType"] = cast->castType;
                result["targetType"] = cast->targetType;
                result["expression"] = convertExpr(cast->expression);
                return result;
            }
            
            return json{{"type", "UnknownExpr"}};
        } catch (const std::exception &e) {
            std::cerr << "ERROR in convertExpr: " << e.what() << std::endl;
            return json{{"type", "ErrorExpr"}, {"error", e.what()}};
        }
    }

    std::string binOpKindToString(CppBinOpKind kind) {
        switch (kind) {
            case CppBinOpKind::Add: return "+";
            case CppBinOpKind::Sub: return "-";
            case CppBinOpKind::Mul: return "*";
            case CppBinOpKind::Div: return "/";
            case CppBinOpKind::Mod: return "%";
            case CppBinOpKind::And: return "&&";
            case CppBinOpKind::Or: return "||";
            case CppBinOpKind::BitAnd: return "&";
            case CppBinOpKind::BitOr: return "|";
            case CppBinOpKind::BitXor: return "^";
            case CppBinOpKind::Shl: return "<<";
            case CppBinOpKind::Shr: return ">>";
            case CppBinOpKind::Lt: return "<";
            case CppBinOpKind::Gt: return ">";
            case CppBinOpKind::Le: return "<=";
            case CppBinOpKind::Ge: return ">=";
            case CppBinOpKind::Eq: return "==";
            case CppBinOpKind::Ne: return "!=";
            case CppBinOpKind::Assign: return "=";
            case CppBinOpKind::AddAssign: return "+=";
            case CppBinOpKind::SubAssign: return "-=";
            case CppBinOpKind::MulAssign: return "*=";
            case CppBinOpKind::DivAssign: return "/=";
            case CppBinOpKind::ModAssign: return "%=";
            case CppBinOpKind::AndAssign: return "&&=";
            case CppBinOpKind::OrAssign: return "||=";
            case CppBinOpKind::XorAssign: return "^=";
            case CppBinOpKind::ShlAssign: return "<<=";
            case CppBinOpKind::ShrAssign: return ">>=";
            default: return "unknown";
        }
    }

    std::string accessSpecifierToString(CppAccessSpecifier access) {
        switch (access) {
            case CppAccessSpecifier::Public: return "public";
            case CppAccessSpecifier::Private: return "private";
            case CppAccessSpecifier::Protected: return "protected";
            default: return "unknown";
        }
    }
};

#endif //AISDLAB_CPPASTPARSERTOJSON_H
