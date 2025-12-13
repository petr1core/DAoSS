//
// Created by пк on 19.11.2025.
//

#ifndef AISDLAB_ASTPARSERTOJSON_H
#define AISDLAB_ASTPARSERTOJSON_H

#include <iostream>
#include "../Scripts/json.hpp"
#include "../Ast/Expr.h"
#include "../Ast/Stmt.h"
#include "../Ast/ExprAcceptImpl.h"

using json = nlohmann::json;

class AstToJsonConverter {
public:
    json convertProgram(const std::unique_ptr<Program> &program) {
        if (!program) {
            return json{{"error", "Null program"}};
        }
        std::cout << "DEBUG: Converting program: " << program->name << std::endl;

        json result;
        result["type"] = "Program";
        result["name"] = program->name;
        auto bodyJson = convertBlock(program->body);
        std::cout << "DEBUG: Body JSON type: " << bodyJson.type_name() << std::endl;

        // Проверяем каждое поле перед добавлением
        if (!result["type"].is_string()) {
            std::cerr << "ERROR: result[type] is not string: " << result["type"].type_name() << std::endl;
        }
        if (!result["name"].is_string()) {
            std::cerr << "ERROR: result[name] is not string: " << result["name"].type_name() << std::endl;
        }
        if (!bodyJson.is_object() && !bodyJson.is_null()) {
            std::cerr << "ERROR: bodyJson is not object or null: " << bodyJson.type_name() << std::endl;
        }

        result["body"] = bodyJson;

        // Финальная проверка всего результата
        std::cout << "DEBUG: Final result type: " << result.type_name() << std::endl;
        for (auto &[key, value]: result.items()) {
            std::cout << "DEBUG: result[" << key << "] = " << value.type_name()
                      << " -> " << value << std::endl;
        }

        return result;
    }

private:

    json convertBlock(const std::unique_ptr<Block> &block) {
        if (!block) {
            std::cout << "DEBUG: Block is null" << std::endl;
            return json::object();
        }
        return convertBlock(*block);
    }

    json convertBlock(const Block &block) {
        std::cout << "DEBUG: Converting block with " << block.statements.size() << " statements" << std::endl;

        json result;
        result["type"] = "Block";

        json statements = json::array();
        for (size_t i = 0; i < block.statements.size(); ++i) {
            if (block.statements[i]) {
                std::cout << "DEBUG: Converting statement " << i << std::endl;
                try {
                    auto stmtJson = convertStmt(block.statements[i]);
                    std::cout << "DEBUG: Statement " << i << " JSON type: " << stmtJson.type_name() << std::endl;
                    statements.push_back(stmtJson);
                } catch (const std::exception &e) {
                    std::cerr << "ERROR converting statement " << i << ": " << e.what() << std::endl;
                    statements.push_back(json{{"error", e.what()}});
                }
            }
        }
        std::cout << "DEBUG: Statements array size: " << statements.size() << std::endl;
        result["statements"] = statements;

        return result;
    }

    json convertStmt(const std::unique_ptr<Stmt> &stmt) {
        if (!stmt) {
            return json{{"type", "EmptyStmt"}};
        }

        if (auto block = dynamic_cast<Block *>(stmt.get())) {
            std::cout << "DEBUG: Identified as Block" << std::endl;
            return convertBlock(*block);
        } else if (auto varDecl = dynamic_cast<VarDeclStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as VarDeclStmt: " << varDecl->name << std::endl;
            return convertVarDecl(*varDecl);
        } else if (auto funcDecl = dynamic_cast<FunctionDecl *>(stmt.get())) {
            std::cout << "DEBUG: Identified as FunctionDecl: " << funcDecl->name << std::endl;
            return convertFunctionDecl(*funcDecl);
        } else if (auto exprStmt = dynamic_cast<ExprStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as ExprStmt" << std::endl;
            return convertExprStmt(*exprStmt);
        } else if (auto ifStmt = dynamic_cast<IfStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as IfStmt" << std::endl;
            return convertIfStmt(*ifStmt);
        } else if (auto whileStmt = dynamic_cast<WhileStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as WhileStmt" << std::endl;
            return convertWhileStmt(*whileStmt);
        } else if (auto forStmt = dynamic_cast<ForStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as ForStmt" << std::endl;
            return convertForStmt(*forStmt);
        } else if (auto returnStmt = dynamic_cast<ReturnStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as ReturnStmt" << std::endl;
            return convertReturnStmt(*returnStmt);
        } else if (auto breakStmt = dynamic_cast<BreakStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as BreakStmt" << std::endl;
            return json{{"type", "BreakStmt"}};
        } else if (auto structDecl = dynamic_cast<StructDecl *>(stmt.get())) {
            std::cout << "DEBUG: Identified as StructDecl: " << structDecl->name << std::endl;
            return convertStructDecl(*structDecl);
        } else if (auto typedefDecl = dynamic_cast<TypedefDecl *>(stmt.get())) {
            std::cout << "DEBUG: Identified as TypedefDecl: " << typedefDecl->alias << std::endl;
            return convertTypedefDecl(*typedefDecl);
        } else if (auto assignStmt = dynamic_cast<AssignStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as AssignStmt: " << assignStmt->target << std::endl;
            return convertAssignStmt(*assignStmt);
        } else if (auto switchStmt = dynamic_cast<SwitchStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as SwitchStmt" << std::endl;
            return convertSwitchStmt(*switchStmt);
        } else if (auto caseStmt = dynamic_cast<CaseStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as CaseStmt" << std::endl;
            return convertCaseStmt(*caseStmt);
        } else if (auto defaultStmt = dynamic_cast<DefaultStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as DefaultStmt" << std::endl;
            return convertDefaultStmt(*defaultStmt);
        } else if (auto doWhileStmt = dynamic_cast<DoWhileStmt *>(stmt.get())) {
            std::cout << "DEBUG: Identified as DoWhileStmt" << std::endl;
            return convertDoWhileStmt(*doWhileStmt);
        } else if (auto preprocessor = dynamic_cast<PreprocessorDirective *>(stmt.get())) {
            std::cout << "DEBUG: Identified as PreprocessorDirective: " << preprocessor->directive << std::endl;
            return convertPreprocessor(*preprocessor);
        } else {
            std::cout << "DEBUG: Unknown statement type" << std::endl;
            return json{{"type", "UnknownStmt"}};
        }
    }


    json convertVarDecl(const VarDeclStmt &varDecl) {
        json result;
        result["type"] = "VarDeclStmt";
        result["varType"] = varDecl.typeName;
        result["name"] = varDecl.name;

        if (varDecl.initializer) {
            result["initializer"] = convertExpr(varDecl.initializer);
        }

        return result;
    }

    json convertFunctionDecl(const FunctionDecl &funcDecl) {
        json result;
        result["type"] = "FunctionDecl";
        result["returnType"] = funcDecl.returnType;
        result["name"] = funcDecl.name;

        // Parameters
        json params = json::array();
        for (const auto &param: funcDecl.params) {
            json paramJson;
            paramJson["type"] = param.typeName;
            paramJson["name"] = param.name;
            params.push_back(paramJson);
        }
        result["parameters"] = params;

        // Body
        if (funcDecl.body) {
            auto bodyResult = convertBlock(funcDecl.body);
            if (bodyResult.is_object()) {
                result["body"] = bodyResult;
            } else {
                result["body"] = json{{"error", "Block conversion failed"}};
            }
        } else {
            result["body"] = nullptr;
        }

        return result;
    }

    json convertExprStmt(const ExprStmt &exprStmt) {
        json result;
        result["type"] = "ExprStmt";
        if (exprStmt.expression) {
            auto exprResult = convertExpr(exprStmt.expression);
            if (exprResult.is_object()) {
                result["expression"] = exprResult;
            } else {
                result["expression"] = json{{"error", "Expression conversion failed"}};
            }
        } else {
            result["expression"] = nullptr;
        }
        return result;
    }

    json convertIfStmt(const IfStmt &ifStmt) {
        json result;
        result["type"] = "IfStmt";
        result["condition"] = convertExpr(ifStmt.condition);
        result["thenBranch"] = convertStmt(ifStmt.thenBranch);

        if (ifStmt.elseBranch) {
            result["elseBranch"] = convertStmt(ifStmt.elseBranch);
        }

        return result;
    }

    json convertWhileStmt(const WhileStmt &whileStmt) {
        json result;
        result["type"] = "WhileStmt";
        result["condition"] = convertExpr(whileStmt.condition);
        result["body"] = convertStmt(whileStmt.body);
        return result;
    }

    json convertForStmt(const ForStmt &forStmt) {
        json result;
        result["type"] = "ForStmt";

        if (forStmt.init) {
            result["init"] = convertStmt(forStmt.init);
        }
        if (forStmt.condition) {
            result["condition"] = convertExpr(forStmt.condition);
        }
        if (forStmt.increment) {
            result["increment"] = convertExpr(forStmt.increment);
        }
        if (forStmt.body) {
            result["body"] = convertStmt(forStmt.body);
        }

        return result;
    }

    json convertReturnStmt(const ReturnStmt &returnStmt) {
        json result;
        result["type"] = "ReturnStmt";

        if (returnStmt.value) {
            result["value"] = convertExpr(returnStmt.value);
        }

        return result;
    }

    json convertStructField(const std::unique_ptr<VarDeclStmt> &field) {
        if (!field) return json::object();
        return convertVarDecl(*field);
    }

    json convertStructDecl(const StructDecl &structDecl) {
        json result;
        result["type"] = "StructDecl";
        result["name"] = structDecl.name;

        json fields = json::array();
        for (const auto &field: structDecl.fields) {
            if (field) {
                fields.push_back(convertStructField(field));
            }
        }
        result["fields"] = fields;

        return result;
    }

    json convertTypedefDecl(const TypedefDecl &typedefDecl) {
        json result;
        result["type"] = "TypedefDecl";
        result["typeName"] = typedefDecl.typeName;
        result["alias"] = typedefDecl.alias;
        return result;
    }

    json convertAssignStmt(const AssignStmt &assignStmt) {
        json result;
        result["type"] = "AssignStmt";
        result["target"] = assignStmt.target;
        result["operator"] = assignStmt.op;
        result["value"] = convertExpr(assignStmt.value);
        return result;
    }

    json convertSwitchStmt(const SwitchStmt &switchStmt) {
        json result;
        result["type"] = "SwitchStmt";
        result["condition"] = convertExpr(switchStmt.condition);

        json cases = json::array();
        for (const auto &caseStmt: switchStmt.cases) {
            if (caseStmt) {
                cases.push_back(convertStmt(caseStmt));
            }
        }
        result["cases"] = cases;

        return result;
    }

    json convertCaseStmt(const CaseStmt &caseStmt) {
        json result;
        result["type"] = "CaseStmt";

        if (caseStmt.value) {
            result["value"] = convertExpr(caseStmt.value);
        }

        json body = json::array();
        for (const auto &stmt: caseStmt.body) {
            if (stmt) {
                body.push_back(convertStmt(stmt));
            }
        }
        result["body"] = body;

        return result;
    }

    json convertDefaultStmt(const DefaultStmt &defaultStmt) {
        json result;
        result["type"] = "DefaultStmt";

        json body = json::array();
        for (const auto &stmt: defaultStmt.body) {
            if (stmt) {
                body.push_back(convertStmt(stmt));
            }
        }
        result["body"] = body;

        return result;
    }

    json convertDoWhileStmt(const DoWhileStmt &doWhileStmt) {
        json result;
        result["type"] = "DoWhileStmt";
        result["body"] = convertStmt(doWhileStmt.body);
        result["condition"] = convertExpr(doWhileStmt.condition);
        return result;
    }

    json convertPreprocessor(const PreprocessorDirective &preprocessor) {
        json result;
        result["type"] = "PreprocessorDirective";
        result["directive"] = preprocessor.directive;
        result["value"] = preprocessor.value;
        return result;
    }

    json convertExpr(const std::unique_ptr<Expr> &expr) {
        if (!expr) {
            return json{{"type", "NullExpr"}};
        }
        try {
            if (auto ident = dynamic_cast<Identifier *>(expr.get())) {
                json result;
                result["type"] = "Identifier";
                result["name"] = ident->name;
                return result;
            } else if (auto intLit = dynamic_cast<IntLiteral *>(expr.get())) {
                json result;
                result["type"] = "IntLiteral";
                result["value"] = intLit->value;
                return result;
            } else if (auto realLit = dynamic_cast<RealLiteral *>(expr.get())) {
                json result;
                result["type"] = "RealLiteral";
                result["value"] = realLit->value;
                return result;
            } else if (auto strLit = dynamic_cast<StringLiteral *>(expr.get())) {
                json result;
                result["type"] = "StringLiteral";
                result["value"] = strLit->value;
                return result;
            } else if (auto binaryOp = dynamic_cast<BinaryOp *>(expr.get())) {
                json result;
                result["type"] = "BinaryOp";
                result["operator"] = binOpKindToString(binaryOp->op);
                result["left"] = convertExpr(binaryOp->left);
                result["right"] = convertExpr(binaryOp->right);
                return result;
            } else if (auto unaryOp = dynamic_cast<UnaryOp *>(expr.get())) {
                json result;
                result["type"] = "UnaryOp";
                result["operator"] = unaryOp->op;
                result["postfix"] = unaryOp->postfix;
                result["operand"] = convertExpr(unaryOp->operand);
                return result;
            } else if (auto callExpr = dynamic_cast<CallExpr *>(expr.get())) {
                json result;
                result["type"] = "CallExpr";
                result["callee"] = callExpr->callee;

                json args = json::array();
                for (const auto &arg: callExpr->arguments) {
                    args.push_back(convertExpr(arg));
                }
                result["arguments"] = args;
                return result;
            } else if (auto arrayAccess = dynamic_cast<ArrayAccessExpr *>(expr.get())) {
                json result;
                result["type"] = "ArrayAccessExpr";
                result["array"] = convertExpr(arrayAccess->array);
                result["index"] = convertExpr(arrayAccess->index);
                return result;
            } else if (auto memberAccess = dynamic_cast<MemberAccessExpr *>(expr.get())) {
                json result;
                result["type"] = "MemberAccessExpr";
                result["object"] = convertExpr(memberAccess->object);
                result["member"] = memberAccess->member;
                result["isPointerAccess"] = memberAccess->isPointerAccess;
                return result;
            } else if (auto ternaryOp = dynamic_cast<TernaryOp *>(expr.get())) {
                json result;
                result["type"] = "TernaryOp";
                result["condition"] = convertExpr(ternaryOp->condition);
                result["thenExpr"] = convertExpr(ternaryOp->thenExpr);
                result["elseExpr"] = convertExpr(ternaryOp->elseExpr);
                return result;
            } else if (auto sizeofExpr = dynamic_cast<SizeofExpr *>(expr.get())) {
                json result;
                result["type"] = "SizeofExpr";
                result["expression"] = convertExpr(sizeofExpr->expression);
                result["isType"] = sizeofExpr->isType;
                return result;
            } else {
                return json{{"type", "UnknownExpr"}};
            }
        } catch (const std::exception &e) {
            std::cerr << "ERROR in convertExpr: " << e.what() << std::endl;
            return json{{"type",  "ErrorExpr"},
                        {"error", e.what()}};
        }
    }

    std::string binOpKindToString(BinOpKind kind) {
        switch (kind) {
            case BinOpKind::Add:
                return "+";
            case BinOpKind::Sub:
                return "-";
            case BinOpKind::Mul:
                return "*";
            case BinOpKind::Div:
                return "/";
            case BinOpKind::Mod:
                return "%";
            case BinOpKind::Eq:
                return "==";
            case BinOpKind::Ne:
                return "!=";
            case BinOpKind::Lt:
                return "<";
            case BinOpKind::Le:
                return "<=";
            case BinOpKind::Gt:
                return ">";
            case BinOpKind::Ge:
                return ">=";
            case BinOpKind::And:
                return "&&";
            case BinOpKind::Or:
                return "||";
            case BinOpKind::BitAnd:
                return "&";
            case BinOpKind::BitOr:
                return "|";
            case BinOpKind::BitXor:
                return "^";
            case BinOpKind::Shl:
                return "<<";
            case BinOpKind::Shr:
                return ">>";
            default:
                return "unknown";
        }
    }
};

#endif //AISDLAB_ASTPARSERTOJSON_H
