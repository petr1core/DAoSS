#include <string>
#include "Lexer.h"
#include "Parser.h"
#include <fstream>
#include "json.hpp"
#include "Flowchart/FromExpressions.h"
#include "Flowchart/ExporterJson.h"

using namespace std;
using json = nlohmann::json;

int main() {
    json programJson;
    string test_textPascal =
            "program qq;"
            "function AddNumbers ( a , b : integer ) : integer ;"
            "begin"
                "AddNumbers := a + b;"
            "end;"
            "procedure GreetUser ( name : string ) ;"
            "begin"
                "Writeln ( 'Hello,' , name , '!' );"
            "end;"
            "const"
                "PI: real = 3.1415926;"
            "var"
                "num1, num2, i: integer;"
                "Res, d: real;"
                "res2: string;"
            "begin"
                "Read ( Res ) ;"
                "Writeln ('From Read ' , Res ) ;"
                "num1 := 12 div 2;"
                "num1 := AddNumbers ( PI , PI);"
                "case num1 of"
                    "begin"
                        "1 , 2 , 3 , 4 , 5 :"
                    "begin"
                        "Writeln ( 'Switch works' );"
                    "end;"
                    "else :"
                    "begin"
                        "Writeln ( 'Switch no works' );"
                    "end;"
                "end;"
                "if 5 mod 3 > 0 then begin"
                    "Writeln ('Yes,if 1 ');"
                    "Writeln ('Yes,if 2 ');"
                "end"
                "else begin"
                    "Writeln ('No,else 1 ');"
                    "Writeln ('No,else 2 ');"
                "end;"
                "if 5 mod 3 > 0 then begin"
                    "Writeln ('Yes,if 1 ');"
                    "Writeln ('Yes,if 2 ');"
                "end"
                "else begin"
                    "Writeln ('No,else 1 ');"
                    "Writeln ('No,else 2 ');"
                "end;"
                "res2 := 'Hello world' ;"
                "num1 := 2 ;"
                "if 5 mod 3 = 0 then begin"
                    "Writeln ('Yes,if 2 ');"
                "end"
                "else begin"
                    "Writeln ('No,else 2 ');"
                "end;"
                "Writeln ('From table ' , num1);"
                "if PI <> num1 then begin"
                    "Writeln( 'Pim' );"
                    "if PI <> num1 then begin"
                        "Writeln ('Pam');"
                    "end"
                "end"
                "else begin"
                    "Writeln ('Pum');"
                "end;"
                "for i := 1 to 8 do begin"
                    "Write ( '3' ) ;"
                "end;"
                "while num1 < 6 do begin"
                    "Write('Yes');"
                    "num1 := num1 + 1;"
                "end;"
                "repeat begin"
                    "Write('3');"
                    "num1 := num1 + 1;"
                "end;"
                "until num1 < 7 ;"
                "num1 := num1 + 3;"
            "end.";
    string test_textC =
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n"
            "#include <string.h>\n"
            "#define PI 3.1415926\n"
            "#define MAX_SIZE 100\n"
            "int addNumbers(int a, int b);\n"
            "void greetUser(char* name);\n"
            "float calculateCircleArea(float radius);\n"
            "\n"
            "const float PI_CONST = 3.1415926f;\n"
            "int globalCounter = 0;\n"
            "\n"
            "struct Point {\n"
            "    int x;\n"
            "    int y;\n"
            "};\n"
            "\n"
            "typedef struct Point Point;\n"
            "\n"
            "int main() {\n"
            "    int num1, num2, i;\n"
            "    float res, d;\n"
            "    char res2[50];\n"
            "    int array[10];\n"
            "    Point p1;\n"
            "    printf(\"Enter a number: \");\n"
            "    scanf(\"%f\", &res);\n"
            "    printf(\"From input: %.2f\\n\", res);\n"
            "    num1 = 12 / 2;\n"
            "    num2 = 15 % 4;\n"
            "    num1 = addNumbers(5, 3);\n"
            "    switch(num1) {\n"
            "        case 1:\n"
            "        case 2:\n"
            "        case 3:\n"
            "        case 4:\n"
            "        case 5:\n"
            "            printf(\"Switch works\\n\");\n"
            "            break;"
            "        default:\n"
            "            printf(\"Switch no works\\n\");\n"
            "            break;\n"
            "    }\n"
            "    if (5 % 3 > 0) {\n"
            "        printf(\"Yes,if 1\\n\");\n"
            "        printf(\"Yes,if 2\\n\");\n"
            "    } else {\n"
            "        printf(\"No,else 1\\n\");\n"
            "        printf(\"No,else 2\\n\");\n"
            "    }\n"
            "    if (5 % 3 > 0) {\n"
            "        printf(\"Yes,if 1\\n\");\n"
            "        printf(\"Yes,if 2\\n\");\n"
            "    } else {\n"
            "        printf(\"No,else 1\\n\");\n"
            "        printf(\"No,else 2\\n\");\n"
            "    }\n"
            "    strcpy(res2, \"Hello world\");\n"
            "    num1 = 2;\n"
            "    if (5 % 3 == 0) {\n"
            "        printf(\"Yes,if 2\\n\");\n"
            "    } else {\n"
            "        printf(\"No,else 2\\n\");\n"
            "    }\n"
            "    printf(\"From table: %d\\n\", num1);\n"
            "    if (PI_CONST != num1) {\n"
            "        printf(\"Pim\\n\");\n"
            "        if (PI_CONST != num1) {\n"
            "            printf(\"Pam\\n\");\n"
            "        }\n"
            "    } else {\n"
            "        printf(\"Pum\\n\");\n"
            "    }\n"
            "    for (i = 1; i <= 8; i++) {\n"
            "        printf(\"3\");\n"
            "    }\n"
            "    printf(\"\\n\");\n"
            "    while (num1 < 6) {\n"
            "        printf(\"Yes\");\n"
            "        num1 = num1 + 1;\n"
            "    }\n"
            "    printf(\"\\n\");\n"
            "    do {\n"
            "        printf(\"3\");\n"
            "        num1 = num1 + 1;\n"
            "    } while (num1 < 7);\n"
            "    printf(\"\\n\");\n"
            "    for (i = 0; i < 10; i++) {\n"
            "        array[i] = i * 2;\n"
            "    }\n"
            "    p1.x = 10;\n"
            "    p1.y = 20;\n"
            "    int* ptr = &num1;\n"
            "    *ptr = *ptr + 3;\n"
            "    num1 = num1 + 3;\n"
            "    num1 += 5;\n"
            "    num1++;\n"
            "    if (num1 > 10 && num2 < 20 || !(num1 == 15)) {\n"
            "        printf(\"Complex condition works\\n\");\n"
            "    }\n"
            "    num1 = num1 & 0xFF;\n"
            "    num2 = num1 | 0x0F;\n"
            "    int result = (num1 > num2) ? num1 : num2;\n"
            "    greetUser(\"John\");\n"
            "    float area = calculateCircleArea(5.0f);\n"
            "    printf(\"Circle area: %.2f\\n\", area);\n"
            "    return 0;\n"
            "}\n"
            "int addNumbers(int a, int b) {\n"
            "    return a + b;\n"
            "}\n"
            "void greetUser(char* name) {\n"
            "    printf(\"Hello, %s!\\n\", name);\n"
            "}\n"
            "float calculateCircleArea(float radius) {\n"
            "    return PI_CONST * radius * radius;\n"
            "}";
    string test_textCPlusPlus = "return 0;";

    //Lexer lexer(test_textPascal, PASCAL);
    Lexer lexer(test_textC, C);
    lexer.printTokenList();
    Parser parser(lexer, PASCAL);
//    parser.initDeclaration();
//    parser.print();
//    // Экспорт блок-схемы по существующим Expression (Mermaid и JSON)
//    const auto &exprs = parser.getExpressionsOnly();
//    auto* ffe=new FlowchartFromExpressions();
//    std::string mmd = ffe->build(exprs);
//    std::ofstream f("flowchart.mmd");
//    f << "graph TD\n";
//    f << "N1" << "(["<<parser.getTitle()<<"])\n";
//    f << "N1 --> N2\n";
//    f << mmd;
//    f.close();
//    auto* ffj=new FlowchartExporterJson();
//    std::string json = ffj->toJson(exprs);
//    std::ofstream f2("flowchart.json");
//    f2 << json;
//    f2.close();

    /*TPostfixCalc c;
    string s1 ="b : integer ";
    string s2="a : integer ";
    string s3="a := 5 ";
    string s4="b := 2 ";
    string s5="a := a mod b";
    Lexer lexer1 (s1);
    Lexer lexer2(s2);
    Lexer lexer3(s3);
    Lexer lexer4(s4);
    Lexer lexer5(s5);
    StatementExpression sx1(lexer1.getTokenList());
    StatementExpression sx2(lexer2.getTokenList());
    StatementExpression sx3(lexer3.getTokenList());
    StatementExpression sx4(lexer4.getTokenList());
    StatementExpression sx5(lexer5.getTokenList());
    string s6="if ( 2 <> 8 ) xor ( 4 < 2 ) then begin"
                "Write('Yes');"
              "end";
    Lexer lexer6(s6);
    ConditionExpression cx(0,lexer6.getTokenList());
    //string s="(8 + 3) * (9 + 4)";
    //Lexer lexer(s);
    //vector<Token>list=lexer.getTokenList();
    c.ChangeEquation(sx1);
    c.ChangeEquation(sx2);
    c.ChangeEquation(sx3);
    c.ChangeEquation(sx4);
    c.ChangeEquation(sx5);
    c.ChangeEquation(cx);
    c.getTable().root->print();
    cout<<c.getTable().findNode("a",c.getTable().root)->data.value;
    cout<<c;*/
    return 0;
}