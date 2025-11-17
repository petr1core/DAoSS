#ifndef AST_EXPR_ACCEPT_IMPL_H
#define AST_EXPR_ACCEPT_IMPL_H

#include "Visitor.h"
#include "Expr.h"
#include "Stmt.h"

inline void Identifier::accept(Visitor &v) { v.visit(*this); }
inline void StringLiteral::accept(Visitor &v) { v.visit(*this); }
inline void IntLiteral::accept(Visitor &v) { v.visit(*this); }
inline void RealLiteral::accept(Visitor &v) { v.visit(*this); }
inline void UnaryOp::accept(Visitor &v) { v.visit(*this); }
inline void BinaryOp::accept(Visitor &v) { v.visit(*this); }
inline void CallExpr::accept(Visitor &v) { v.visit(*this); }
inline void AssignStmt::accept(Visitor &v) { v.visit(*this); }
inline void ExprStmt::accept(Visitor &v) { v.visit(*this); }
inline void VarDeclStmt::accept(Visitor &v) { v.visit(*this); }
inline void IfStmt::accept(Visitor &v) { v.visit(*this); }
inline void WhileStmt::accept(Visitor &v) { v.visit(*this); }
inline void ForStmt::accept(Visitor &v) { v.visit(*this); }
inline void ReturnStmt::accept(Visitor &v) { v.visit(*this); }
inline void WriteStmt::accept(Visitor &v) { v.visit(*this); }
inline void Block::accept(Visitor &v) { v.visit(*this); }
inline void FunctionDecl::accept(Visitor &v) { v.visit(*this); }
inline void Program::accept(Visitor &v) { v.visit(*this); }

#endif // AST_EXPR_ACCEPT_IMPL_H
