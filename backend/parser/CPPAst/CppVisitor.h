//
// Created by пк on 19.11.2025.
//

#ifndef AISDLAB_CPPVISITOR_H
#define AISDLAB_CPPVISITOR_H

#include "CppAstFw.h"

// Предварительные объявления всех C++ узлов
struct CppIdentifier; struct CppStringLiteral; struct CppIntLiteral;
struct CppRealLiteral; struct CppBoolLiteral; struct CppNullptrLiteral;
struct CppUnaryOp; struct CppBinaryOp; struct CppTernaryOp;
struct CppCallExpr; struct CppArrayAccessExpr; struct CppMemberAccessExpr;
struct CppPointerToMemberExpr; struct CppNewExpr; struct CppDeleteExpr;
struct CppThisExpr; struct CppSizeofExpr; struct CppTypeidExpr;
struct CppTemplateIdExpr; struct CppLambdaExpr; struct CppCastExpr;

struct CppExprStmt; struct CppCompoundStmt; struct CppReturnStmt;
struct CppBreakStmt; struct CppContinueStmt; struct CppIfStmt;
struct CppWhileStmt; struct CppDoWhileStmt; struct CppForStmt;
struct CppRangeForStmt; struct CppSwitchStmt; struct CppCaseStmt;
struct CppDefaultStmt; struct CppTryStmt; struct CppCatchStmt;
struct CppVarDecl; struct CppAssignStmt; struct CppMethodDecl; struct CppOperatorDecl; struct CppConstructorDecl;

struct CppFunctionDecl; struct CppTemplateDecl; struct CppClassDecl;
struct CppFieldDecl; struct CppNamespaceDecl; struct CppUsingDecl;
struct CppTypedefDecl; struct CppUsingAliasDecl; struct CppEnumDecl;
struct CppPreprocessorDirective; struct CppProgram;

struct CppVisitor {
    virtual ~CppVisitor() = default;

    // Expressions
    virtual void visit(CppIdentifier&) = 0;
    virtual void visit(CppStringLiteral&) = 0;
    virtual void visit(CppIntLiteral&) = 0;
    virtual void visit(CppRealLiteral&) = 0;
    virtual void visit(CppBoolLiteral&) = 0;
    virtual void visit(CppNullptrLiteral&) = 0;
    virtual void visit(CppUnaryOp&) = 0;
    virtual void visit(CppBinaryOp&) = 0;
    virtual void visit(CppTernaryOp&) = 0;
    virtual void visit(CppCallExpr&) = 0;
    virtual void visit(CppArrayAccessExpr&) = 0;
    virtual void visit(CppMemberAccessExpr&) = 0;
    virtual void visit(CppPointerToMemberExpr&) = 0;
    virtual void visit(CppNewExpr&) = 0;
    virtual void visit(CppDeleteExpr&) = 0;
    virtual void visit(CppThisExpr&) = 0;
    virtual void visit(CppSizeofExpr&) = 0;
    virtual void visit(CppTypeidExpr&) = 0;
    virtual void visit(CppTemplateIdExpr&) = 0;
    virtual void visit(CppLambdaExpr&) = 0;
    virtual void visit(CppCastExpr&) = 0;

    // Statements
    virtual void visit(CppExprStmt&) = 0;
    virtual void visit(CppCompoundStmt&) = 0;
    virtual void visit(CppReturnStmt&) = 0;
    virtual void visit(CppBreakStmt&) = 0;
    virtual void visit(CppContinueStmt&) = 0;
    virtual void visit(CppIfStmt&) = 0;
    virtual void visit(CppWhileStmt&) = 0;
    virtual void visit(CppDoWhileStmt&) = 0;
    virtual void visit(CppForStmt&) = 0;
    virtual void visit(CppRangeForStmt&) = 0;
    virtual void visit(CppSwitchStmt&) = 0;
    virtual void visit(CppCaseStmt&) = 0;
    virtual void visit(CppDefaultStmt&) = 0;
    virtual void visit(CppTryStmt&) = 0;
    virtual void visit(CppCatchStmt&) = 0;
    virtual void visit(CppVarDecl&) = 0;
    virtual void visit(CppAssignStmt&) = 0;

    // Declarations
    virtual void visit(CppConstructorDecl&) = 0;
    virtual void visit(CppOperatorDecl&) = 0;
    virtual void visit(CppMethodDecl&) = 0;
    virtual void visit(CppFunctionDecl&) = 0;
    virtual void visit(CppTemplateDecl&) = 0;
    virtual void visit(CppClassDecl&) = 0;
    virtual void visit(CppFieldDecl&) = 0;
    virtual void visit(CppNamespaceDecl&) = 0;
    virtual void visit(CppUsingDecl&) = 0;
    virtual void visit(CppTypedefDecl&) = 0;
    virtual void visit(CppUsingAliasDecl&) = 0;
    virtual void visit(CppEnumDecl&) = 0;
    virtual void visit(CppPreprocessorDirective&) = 0;
    virtual void visit(CppProgram&) = 0;

};

#endif //AISDLAB_CPPVISITOR_H