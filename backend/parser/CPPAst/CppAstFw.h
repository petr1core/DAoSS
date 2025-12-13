//
// Created by пк on 19.11.2025.
//

#ifndef AISDLAB_CPPASTFW_H
#define AISDLAB_CPPASTFW_H

#include <memory>
#include <vector>
#include <string>

// Forward declarations всех C++ AST классов
struct CppAstNode;
struct CppExpr;
struct CppStmt;
struct CppDecl;

struct CppIdentifier;
struct CppStringLiteral;
struct CppIntLiteral;
struct CppRealLiteral;
struct CppBoolLiteral;
struct CppNullptrLiteral;

struct CppUnaryOp;
struct CppBinaryOp;
struct CppTernaryOp;
struct CppCallExpr;
struct CppArrayAccessExpr;
struct CppMemberAccessExpr;

struct CppExprStmt;
struct CppCompoundStmt;
struct CppReturnStmt;
struct CppIfStmt;
struct CppWhileStmt;
struct CppForStmt;

struct CppVarDecl;
struct CppFunctionDecl;
struct CppClassDecl;
struct CppNamespaceDecl;

struct CppProgram;

// Умные указатели для forward declarations
using CppExprPtr = std::unique_ptr<CppExpr>;
using CppStmtPtr = std::unique_ptr<CppStmt>;
using CppDeclPtr = std::unique_ptr<CppDecl>;

using CppExprList = std::vector<CppExprPtr>;
using CppStmtList = std::vector<CppStmtPtr>;
using CppDeclList = std::vector<CppDeclPtr>;

#endif //AISDLAB_CPPASTFW_H
