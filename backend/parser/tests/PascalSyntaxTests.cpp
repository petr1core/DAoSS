#include <gtest/gtest.h>
#include "../Scripts/Lexer.h"
#include "../Parser/PascalParserToExpression.h"
#include "../Parser/ErrorCollector.h"
#include <string>

class PascalSyntaxTest : public ::testing::Test {
protected:
    bool isValidPascalCode(const std::string& code) {
        try {
            ErrorCollector errorCollector(code);
            Lexer lexer(code, LangType::LANG_PASCAL);
            PascalParserToExpression parser(lexer, LangType::LANG_PASCAL);
            parser.parseOnly();
            return !errorCollector.hasErrors();
        } catch (const std::exception&) {
            return false;
        }
    }

    bool isInvalidPascalCode(const std::string& code) {
        try {
            ErrorCollector errorCollector(code);
            Lexer lexer(code, LangType::LANG_PASCAL);
            PascalParserToExpression parser(lexer, LangType::LANG_PASCAL);
            parser.parseOnly();
            return errorCollector.hasErrors();
        } catch (const std::exception&) {
            return true;
        }
    }
};

TEST_F(PascalSyntaxTest, ValidMinimalProgram) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin end."));
}

TEST_F(PascalSyntaxTest, ValidProgramWithVariables) {
    EXPECT_TRUE(isValidPascalCode("program Test; var x: integer; begin end."));
}

TEST_F(PascalSyntaxTest, ValidProgramWithAssignment) {
    EXPECT_TRUE(isValidPascalCode("program Test; var x: integer; begin x := 10; end."));
}

TEST_F(PascalSyntaxTest, ValidProgramWithConstants) {
    EXPECT_TRUE(isValidPascalCode("program Test; const x = 10; begin end."));
}

TEST_F(PascalSyntaxTest, ValidProgramWithIfElse) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin if 1 = 1 then writeln('yes') else writeln('no'); end."));
}

TEST_F(PascalSyntaxTest, ValidProgramWithProcedure) {
    EXPECT_TRUE(isValidPascalCode("program Test; procedure Print; begin writeln('Hello'); end; begin end."));
}

TEST_F(PascalSyntaxTest, ValidProgramWithFunction) {
    EXPECT_TRUE(isValidPascalCode("program Test; function Add(a, b: integer): integer; begin Add := a + b; end; begin end."));
}

TEST_F(PascalSyntaxTest, ValidMultipleVariables) {
    EXPECT_TRUE(isValidPascalCode("program Test; var x, y, z: integer; begin end."));
}

TEST_F(PascalSyntaxTest, ValidDifferentVariableTypes) {
    EXPECT_TRUE(isValidPascalCode("program Test; var x: integer; y: real; z: string; begin end."));
}

TEST_F(PascalSyntaxTest, ValidMultipleConstants) {
    EXPECT_TRUE(isValidPascalCode("program Test; const x = 10; y = 20; z = 30; begin end."));
}

TEST_F(PascalSyntaxTest, ValidArithmeticOperations) {
    EXPECT_TRUE(isValidPascalCode("program Test; var x: integer; begin x := 10 + 20; end."));
}

TEST_F(PascalSyntaxTest, ValidSimpleIfWithoutElse) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin if 1 = 1 then writeln('yes'); end."));
}

TEST_F(PascalSyntaxTest, ValidNestedIf) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin if 1 = 1 then if 2 = 2 then writeln('nested'); end."));
}

TEST_F(PascalSyntaxTest, ValidProcedureWithParameters) {
    EXPECT_TRUE(isValidPascalCode("program Test; procedure Print(x: integer); begin writeln(x); end; begin end."));
}

TEST_F(PascalSyntaxTest, ValidFunctionWithoutParameters) {
    EXPECT_TRUE(isValidPascalCode("program Test; function GetValue: integer; begin GetValue := 42; end; begin end."));
}

TEST_F(PascalSyntaxTest, ValidProcedureCall) {
    EXPECT_TRUE(isValidPascalCode("program Test; procedure Print; begin writeln('test'); end; begin Print; end."));
}

TEST_F(PascalSyntaxTest, ValidFunctionCall) {
    EXPECT_TRUE(isValidPascalCode("program Test; function GetValue: integer; begin GetValue := 10; end; var x: integer; begin x := GetValue; end."));
}

TEST_F(PascalSyntaxTest, ValidComparisons) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin if 1 < 2 then writeln('less'); end."));
}

TEST_F(PascalSyntaxTest, ValidLogicalAND) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin if (1 = 1) and (2 = 2) then writeln('and'); end."));
}

TEST_F(PascalSyntaxTest, ValidLogicalOR) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin if (1 = 1) or (2 = 3) then writeln('or'); end."));
}

TEST_F(PascalSyntaxTest, ValidLogicalNOT) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin if not (1 = 2) then writeln('not'); end."));
}

TEST_F(PascalSyntaxTest, ValidStringLiterals) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin writeln('Hello World'); end."));
}

TEST_F(PascalSyntaxTest, ValidRealNumbers) {
    EXPECT_TRUE(isValidPascalCode("program Test; var x: real; begin x := 3.14; end."));
}

TEST_F(PascalSyntaxTest, ValidConstAndVarTogether) {
    EXPECT_TRUE(isValidPascalCode("program Test; const x = 10; var y: integer; begin end."));
}

TEST_F(PascalSyntaxTest, ValidComplexExpression) {
    EXPECT_TRUE(isValidPascalCode("program Test; var x: integer; begin x := 10 + 20 * 2; end."));
}

TEST_F(PascalSyntaxTest, ValidNestedParentheses) {
    EXPECT_TRUE(isValidPascalCode("program Test; begin writeln((1 + 2) * 3); end."));
}

TEST_F(PascalSyntaxTest, ErrorMissingDotAtEnd) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; begin end"));
}

TEST_F(PascalSyntaxTest, ErrorMissingSemicolonAfterVariable) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; var x: integer begin end."));
}

TEST_F(PascalSyntaxTest, ErrorMissingDoInForLoop) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; var i: integer; begin for i := 1 to 10 writeln(i); end."));
}

TEST_F(PascalSyntaxTest, ErrorEmptyProgram) {
    EXPECT_TRUE(isInvalidPascalCode(""));
}

TEST_F(PascalSyntaxTest, ErrorOnlyProgramKeyword) {
    EXPECT_TRUE(isInvalidPascalCode("program"));
}

TEST_F(PascalSyntaxTest, ErrorMissingEndOnlyBegin) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; begin"));
}

TEST_F(PascalSyntaxTest, ErrorEmptyProgramOnlyWhitespace) {
    EXPECT_TRUE(isInvalidPascalCode("   \n  \t  \n  "));
}

TEST_F(PascalSyntaxTest, ErrorMissingSemicolonAfterProcedure) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; procedure TestProc begin end begin end."));
}

TEST_F(PascalSyntaxTest, ErrorMissingEndInProcedure) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; procedure TestProc; begin writeln('test'); begin end."));
}

TEST_F(PascalSyntaxTest, ErrorMissingDotAtEndWithProcedure) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; procedure P; begin end; begin end"));
}

TEST_F(PascalSyntaxTest, ErrorMissingEndWithVar) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; var x: integer; begin x := 10"));
}

TEST_F(PascalSyntaxTest, ErrorMissingDotAtEndWithMultipleStatements) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; var x, y: integer; begin x := 10; y := 20; end"));
}

TEST_F(PascalSyntaxTest, ErrorMissingSemicolonAfterVarMultipleVariables) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; var x: integer; y: real; z: string begin end."));
}

TEST_F(PascalSyntaxTest, ErrorMissingEndWithVarAndStatements) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; var x, y: integer; begin x := 10; y := 20"));
}

TEST_F(PascalSyntaxTest, ErrorMissingDotAtEndWithVarConstAndStatements) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; const x = 10; var y: integer; begin y := x + 5; end"));
}

TEST_F(PascalSyntaxTest, ErrorMissingEndWithProcedure) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; procedure P; begin writeln('test'); begin"));
}

TEST_F(PascalSyntaxTest, ErrorMissingDotAtEndWithFunctionsAndProcedures) {
    EXPECT_TRUE(isInvalidPascalCode("program Test; function F: integer; begin F := 1; end; procedure P; begin end; begin P; end"));
}

