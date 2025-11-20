//
// Created by пк on 20.11.2025.
//

#ifndef AISDLAB_CPPEXPRACCEPTIMPL_H
#define AISDLAB_CPPEXPRACCEPTIMPL_H

#include "CppVisitor.h"
#include "CppExpr.h"
#include "CppStmt.h"
#include "CppDecl.h"

// Expressions
inline void CppIdentifier::accept(CppVisitor &v) { v.visit(*this); }
inline void CppStringLiteral::accept(CppVisitor &v) { v.visit(*this); }
inline void CppIntLiteral::accept(CppVisitor &v) { v.visit(*this); }
inline void CppRealLiteral::accept(CppVisitor &v) { v.visit(*this); }
inline void CppBoolLiteral::accept(CppVisitor &v) { v.visit(*this); }
inline void CppNullptrLiteral::accept(CppVisitor &v) { v.visit(*this); }
inline void CppUnaryOp::accept(CppVisitor &v) { v.visit(*this); }
inline void CppBinaryOp::accept(CppVisitor &v) { v.visit(*this); }
inline void CppTernaryOp::accept(CppVisitor &v) { v.visit(*this); }
inline void CppCallExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppArrayAccessExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppMemberAccessExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppPointerToMemberExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppNewExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppDeleteExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppThisExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppSizeofExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppTypeidExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppTemplateIdExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppLambdaExpr::accept(CppVisitor &v) { v.visit(*this); }
inline void CppCastExpr::accept(CppVisitor &v) { v.visit(*this); }

// Statements
inline void CppExprStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppCompoundStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppReturnStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppBreakStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppContinueStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppIfStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppWhileStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppDoWhileStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppForStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppRangeForStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppSwitchStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppCaseStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppDefaultStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppTryStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppCatchStmt::accept(CppVisitor &v) { v.visit(*this); }
inline void CppVarDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppAssignStmt::accept(CppVisitor &v) { v.visit(*this); }

// Declarations
inline void CppFunctionDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppTemplateDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppClassDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppFieldDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppNamespaceDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppUsingDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppTypedefDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppUsingAliasDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppEnumDecl::accept(CppVisitor &v) { v.visit(*this); }
inline void CppPreprocessorDirective::accept(CppVisitor &v) { v.visit(*this); }
inline void CppProgram::accept(CppVisitor &v) { v.visit(*this); }

#endif //AISDLAB_CPPEXPRACCEPTIMPL_H
