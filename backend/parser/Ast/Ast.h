//
// Minimal AST core for Pascal → C++ → Flowchart pipeline
//

#ifndef AST_CORE_H
#define AST_CORE_H

#include <string>
#include "Visitor.h"
#include <vector>
#include <memory>

struct SourceRange {
    int startPos{0};
    int endPos{0};
};

struct Visitor; // forward

struct AstNode {
    int id{0};
    SourceRange range{};
    virtual ~AstNode() = default;
    virtual void accept(Visitor &v) = 0;
};

#endif // AST_CORE_H
