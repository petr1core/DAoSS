//
// Created by пк on 19.11.2025.
//

#ifndef AISDLAB_CPPAST_H
#define AISDLAB_CPPAST_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include "CppAstFw.h"

enum class CppAccessSpecifier {
    Public,
    Private,
    Protected
};

enum class CppStorageClass {
    Auto,
    Static,
    Extern,
    Register,
    Mutable
};

enum class CppCVQualifier {
    None,
    Const,
    Volatile,
    ConstVolatile
};

struct CppSourceRange {
    int startPos{0};
    int endPos{0};
};

// Базовые предварительные объявления
struct CppVisitor;

struct CppAstNode {
    int id{0};
    CppSourceRange range{};
    virtual ~CppAstNode() = default;
    virtual void accept(CppVisitor &v) = 0;
};

struct CppExpr : CppAstNode {};
struct CppStmt : CppAstNode {};
struct CppDecl : CppStmt {};

#endif //AISDLAB_CPPAST_H