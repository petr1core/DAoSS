//
// Created by пк on 19.11.2025.
//

#ifndef AISDLAB_CPPDECL_H
#define AISDLAB_CPPDECL_H

#include "CppStmt.h"
#include <vector>
#include <string>

// Параметры функций/шаблонов
struct CppParameter {
    std::string typeName;
    std::string name;
    std::string defaultValue;
};

struct CppTemplateParameter {
    std::string kind; // type, non-type, template
    std::string name;
    std::string defaultType; // для type параметров
    std::string defaultValue; // для non-type параметров
    bool isPack{false};
};
struct CppInitializer {
    std::string memberName;    // Имя члена (или базового класса)
    std::unique_ptr<CppExpr> value; // Выражение инициализации
    bool isBaseClass;          // true для базовых классов, false для полей
};

// Функции
struct CppFunctionDecl : CppDecl {
    std::string returnType;
    std::string name;
    std::vector<CppParameter> parameters;
    std::vector<CppInitializer> initializers;
    std::unique_ptr<CppCompoundStmt> body;
    bool isVirtual{false};
    bool isOverride{false};
    bool isConst{false};

    void accept(CppVisitor &v) override;
};

// Шаблонные функции/классы
struct CppTemplateDecl : CppDecl {
    std::vector<CppTemplateParameter> parameters;
    std::unique_ptr<CppDecl> declaration;

    void accept(CppVisitor &v) override;
};

// Классы и структуры
struct CppClassDecl : CppDecl {
    std::string name;
    std::vector<std::string> baseClasses;
    std::vector<CppAccessSpecifier> baseAccess;
    CppDeclList members;
    bool isStruct{false}; // struct vs class
    bool isUnion {false};
    void accept(CppVisitor &v) override;
};
// Методы классов
struct CppMethodDecl : CppDecl {
    std::string returnType;
    std::string name;
    std::vector<CppParameter> parameters;
    std::unique_ptr<CppCompoundStmt> body;
    bool isVirtual = false;
    bool isOverride = false;
    bool isConst = false;
    bool isStatic = false;
    bool isFinal = false;

    CppAccessSpecifier access;
    void accept(CppVisitor &v) override;

};

struct CppConstructorDecl :  CppDecl {
    std::string name;
    std::vector<CppParameter> parameters;
    std::vector<CppInitializer> initializers;
    std::unique_ptr<CppCompoundStmt> body;
    CppAccessSpecifier access;
    void accept(CppVisitor &v) override;

};
// Поля классов
struct CppFieldDecl : CppDecl {
    std::string typeName;
    std::string name;
    std::unique_ptr<CppExpr> initializer;
    CppAccessSpecifier access{CppAccessSpecifier::Private};
    CppCVQualifier cvQualifier{CppCVQualifier::None};
    bool isMutable{false};
    bool isStatic{false};


    void accept(CppVisitor &v) override;
};

// Пространства имен
struct CppNamespaceDecl : CppDecl {
    std::string name;
    std::unique_ptr<CppCompoundStmt> body;

    void accept(CppVisitor &v) override;
};

struct CppUsingDecl : CppDecl {
    std::string name;
    std::string target; // для using namespace
    bool isNamespace{false};

    void accept(CppVisitor &v) override;
};

// Псевдонимы типов
struct CppTypedefDecl : CppDecl {
    std::string typeName;
    std::string alias;

    void accept(CppVisitor &v) override;
};

struct CppUsingAliasDecl : CppDecl {
    std::string typeName;
    std::string alias;

    void accept(CppVisitor &v) override;
};

// Перечисления
struct CppEnumDecl : CppDecl {
    std::string name;
    std::string underlyingType;
    std::vector<std::pair<std::string, std::unique_ptr<CppExpr>>> enumerators;
    bool isScoped{false}; // enum class
    void accept(CppVisitor &v) override;
};
struct CppOperatorDecl : CppDecl {
    std::string returnType;
    std::string operatorSymbol;
    std::vector<CppParameter> parameters;
    std::unique_ptr<CppCompoundStmt> body;
    bool isConst = false;
    CppAccessSpecifier access;
    void accept(CppVisitor &v) override;

};

// Препроцессор
struct CppPreprocessorDirective : CppDecl {
    std::string directive;
    std::string value;

    void accept(CppVisitor &v) override;
};

// Программа
struct CppProgram : CppDecl {
    std::string name;
    std::unique_ptr<CppCompoundStmt> body;

    void accept(CppVisitor &v) override;
};

#endif //AISDLAB_CPPDECL_H