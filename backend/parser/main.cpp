#include <string>
#include "Scripts/Lexer.h"
#include <fstream>
#include "Scripts/json.hpp"
#include "Flowchart/FromExpressions.h"
#include "Flowchart/ExporterJson.h"
#include "CodeGen/PascalCodeGenerator.h"
#include "CodeGen/CppCodeGenerator.h"
#include "Parser/PascalParserToExpression.h"
#include "Parser/CParserToAST.h"
#include "Parser/CppParserToAST.h"
#include "CodeGen//CCodeGenerator.h"
#include "Parser/AstVisualizer.h"
#include "Parser/CppAstVisualizer.h"
#include "Parser/ASTParserToJSON.h"

using namespace std;

int main() {
    std::string CPPCode =
            "#include <iostream>\n"
            "#include <vector>\n"
            "#include <string>\n"
            "#include <memory>\n"
            "#include <cmath>\n"
            "#include <cstring>\n"
            "\n"
            "#define MAX_SIZE 100\n"
            "#define SQUARE(x) ((x) * (x))\n"
            "\n"
            "const double PI = 3.1415926;\n"
            "int globalVar = 0;\n"
            "\n"
            "enum Color { RED, GREEN, BLUE };\n"
            "enum class Status { OK, ERROR, PENDING };\n"
            "\n"
            "struct Point {\n"
            "    int x, y;\n"
            "    Point(int x = 0, int y = 0) : x(x), y(y) {}\n"
            "};\n"
            "\n"
            "union Data {\n"
            "    int i;\n"
            "    float f;\n"
            "    char c;\n"
            "};\n"
            "\n"
            "class Shape {\n"
            "protected:\n"
            "    Color color;\n"
            "public:\n"
            "    Shape(Color c) : color(c) {}\n"
            "    virtual double area() const = 0; \n"
            "    virtual ~Shape() {}\n"
            "    void setColor(Color c) { color = c; }\n"
            "    Color getColor() const { return color; }\n"
            "};\n"
            "\n"
            "class Circle : public Shape {\n"
            "private:\n"
            "    double radius;\n"
            "    static int circleCount;\n"
            "public:\n"
            "    Circle(double r, Color c = RED) : Shape(c), radius(r) { circleCount++; }\n"
            "    ~Circle() override { circleCount--; }\n"
            "    \n"
            "    double area() const override {\n"
            "        return PI * radius * radius;\n"
            "    }\n"
            "    \n"
            "    Circle operator+(const Circle& other) const {\n"
            "        return Circle(radius + other.radius, color);\n"
            "    }\n"
            "    \n"
            "    bool operator==(const Circle& other) const {\n"
            "        return radius == other.radius;\n"
            "    }\n"
            "    \n"
            "    static int getCircleCount() { return circleCount; }\n"
            "    \n"
            "    friend void printCircleInfo(const Circle& c);\n"
            "};\n"
            "\n"
            "int Circle::circleCount = 0;\n"
            "\n"
            "void printCircleInfo(const Circle& c) {\n"
            "    std::cout << \"Circle radius: \" << c.radius << std::endl;\n"
            "}\n"
            "\n"
            "template<typename T>\n"
            "T max(const T& a, const T& b) {\n"
            "    return (a > b) ? a : b;\n"
            "}\n"
            "\n"
            "template<>\n"
            "const char* max(const char* const & a, const char* const & b) {\n"
            "    return (std::strlen(a) > std::strlen(b)) ? a : b;\n"
            "}\n"
            "\n"
            "template<typename T>\n"
            "class Container {\n"
            "private:\n"
            "    T* data;\n"
            "    size_t size;\n"
            "public:\n"
            "    Container(size_t s) : size(s) {\n"
            "        data = new T[size];\n"
            "    }\n"
            "    \n"
            "    ~Container() {\n"
            "        delete[] data;\n"
            "    }\n"
            "    \n"
            "    Container(const Container& other) : size(other.size) {\n"
            "        data = new T[size];\n"
            "        for (size_t i = 0; i < size; ++i) {\n"
            "            data[i] = other.data[i];\n"
            "        }\n"
            "    }\n"
            "    \n"
            "    Container& operator=(const Container& other) {\n"
            "        if (this != &other) {\n"
            "            delete[] data;\n"
            "            size = other.size;\n"
            "            data = new T[size];\n"
            "            for (size_t i = 0; i < size; ++i) {\n"
            "                data[i] = other.data[i];\n"
            "            }\n"
            "        }\n"
            "        return *this;\n"
            "    }\n"
            "    \n"
            "    T& operator[](size_t index) { return data[index]; }\n"
            "    const T& operator[](size_t index) const { return data[index]; }\n"
            "};\n"
            "\n"
            "namespace Math {\n"
            "    int add(int a, int b) {\n"
            "        return a + b;\n"
            "    }\n"
            "    \n"
            "    namespace Geometry {\n"
            "        double calculateDistance(const Point& p1, const Point& p2) {\n"
            "            return std::sqrt(SQUARE(p1.x - p2.x) + SQUARE(p1.y - p2.y));\n"
            "        }\n"
            "    }\n"
            "}\n"
            "\n"
            "using StringVector = std::vector<std::string>;\n"
            "\n"
            "std::unique_ptr<Circle> createCircle(double radius) {\n"
            "    return std::make_unique<Circle>(radius);\n"
            "}\n"
            "\n"
            "template<typename... Args>\n"
            "void printAll(Args... args) {\n"
            "    (std::cout << ... << args) << std::endl;\n"
            "}\n"
            "\n"
            "auto multiplier = [](double factor) {\n"
            "    return [factor](double value) { return value * factor; };\n"
            "};\n"
            "\n"
            "int main() {\n"
            "    int num1 = 10, num2 = 20;\n"
            "    double d = 3.14;\n"
            "    char c = 'A';\n"
            "    bool flag = true;\n"
            "    \n"
            "    int& ref = num1;\n"
            "    ref = 15;\n"
            "    \n"

            "    int* ptr = &num1;\n"
            "    *ptr = 25;\n"
            "    \n"
            "    int arr[5] = {1, 2, 3, 4, 5};\n"
            "    \n"
            "    std::vector<int> vec = {1, 2, 3, 4, 5};\n"
            "    vec.push_back(6);\n"
            "    \n"
            "    std::string str = \"Hello\";\n"
            "    str += \" World!\";\n"
            "    \n"
            "    if (num1 > 0 && num2 < 100) {\n"
            "        std::cout << \"Condition met\" << std::endl;\n"
            "    } else if (num1 == 0) {\n"
            "        std::cout << \"Zero\" << std::endl;\n"
            "    } else {\n"
            "        std::cout << \"Other\" << std::endl;\n"
            "    }\n"
            "    \n"
            "    Status s = Status::OK;\n"
            "    switch (s) {\n"
            "        case Status::OK:\n"
            "            std::cout << \"OK\" << std::endl;\n"
            "            break;\n"
            "        case Status::ERROR:\n"
            "            std::cout << \"ERROR\" << std::endl;\n"
            "            break;\n"
            "        default:\n"
            "            std::cout << \"Unknown\" << std::endl;\n"
            "    }\n"
            "    \n"
            "    for (int i = 0; i < 5; ++i) {\n"
            "        std::cout << i << \" \";\n"
            "    }\n"
            "    std::cout << std::endl;\n"
            "    \n"
            "    for (const auto& item : vec) {\n"
            "        std::cout << item << \" \";\n"
            "    }\n"
            "    std::cout << std::endl;\n"
            "    \n"
            "    int j = 0;\n"
            "    while (j < 3) {\n"
            "        std::cout << j << \" \";\n"
            "        j++;\n"
            "    }\n"
            "    std::cout << std::endl;\n"
            "    \n"
            "    Circle circle1(5.0, Color::BLUE);\n"
            "    Circle circle2(3.0);\n"
            "    \n"
            "    Circle circle3 = circle1 + circle2;\n"
            "    \n"
            "    if (circle1 == circle2) {\n"
            "        std::cout << \"Circles are equal\" << std::endl;\n"
            "    }\n"
            "    \n"
            "    Shape* shape = &circle1;\n"
            "    std::cout << \"Area: \" << shape->area() << std::endl;\n"
            "    \n"
            "    std::cout << \"Max: \" << max(10, 20) << std::endl;\n"
            "    std::cout << \"Max string: \" << max(\"hello\", \"world\") << std::endl;\n"
            "    \n"
            "    Container<int> intContainer(5);\n"
            "    intContainer[0] = 100;\n"
            "    \n"
            "    std::cout << \"Math result: \" << Math::add(5, 3) << std::endl;\n"
            "    \n"
            "    Point p1(0, 0), p2(3, 4);\n"
            "    std::cout << \"Distance: \" << Math::Geometry::calculateDistance(p1, p2) << std::endl;\n"
            "    \n"
            "    auto circlePtr = createCircle(10.0);\n"
            "    \n"
            "    auto doubleIt = multiplier(2.0);\n"
            "    std::cout << \"Doubled: \" << doubleIt(5.0) << std::endl;\n"
            "    \n"
            "    try {\n"
            "        if (num1 < 0) {\n"
            "            throw std::runtime_error(\"Negative number\");\n"
            "        }\n"
            "        \n"
            "        Circle* circlePtr = dynamic_cast<Circle*>(shape);\n"
            "        if (circlePtr) {\n"
            "            std::cout << \"Successful dynamic cast\" << std::endl;\n"
            "        }\n"
            "        \n"
            "    } catch (const std::exception& e) {\n"
            "        std::cout << \"Exception: \" << e.what() << std::endl;\n"
            "    }\n"
            "    \n"
            "    std::cout << \"Circle count: \" << Circle::getCircleCount() << std::endl;\n"
            "    \n"
            "    Data data;\n"
            "    data.i = 42;\n"
            "    std::cout << \"Union data: \" << data.i << std::endl;\n"
            "    \n"
            "    printAll(\"Hello\", \" \", \"World\", \"!\", 123);\n"
            "    \n"
            "    return 0;\n"
            "}\n";


    string pascalCode =
            "program qq;"
            "function AddNumbers ( a , b : integer ) : integer ;"
            "begin"
            "   AddNumbers := a + b;"
            "end;"
            "procedure GreetUser ( name : string ) ;"
            "begin"
            "   Writeln ( 'Hello,' , name , '!' );"
            "end;"
            "const"
            "   PI: real = 3.1415926;"
            "var"
            "   num1, num2, i: integer;"
            "   Res, d: real;"
            "   res2: string;"
            "begin"
            "   Read ( Res ) ;"
            "   Writeln ('From Read ' , Res ) ;"
            "   num1 := 12 div 2;"
            "   num1 := AddNumbers ( PI , PI);"
            "   if 5 mod 3 > 0 then begin"
            "       Writeln ('Yes,if 1 ');"
            "       Writeln ('Yes,if 2 ');"
            "   end"
            "   case num1 of"
            "   begin"
            "       1 , 2 , 3 :"
            "       begin"
            "           Writeln ( 'Switch works 1' );"
            "       end;"
            "       4 , 5 :"
            "       begin"
            "           Writeln ( 'Switch works 2' );"
            "       end;"
            "       else :"
            "       begin"
            "           Writeln ( 'Switch no works' );"
            "       end;"
            "   end;"
            "   if 5 mod 3 > 0 then begin"
            "       Writeln ('Yes,if 1 ');"
            "       Writeln ('Yes,if 2 ');"
            "   end"
            "   else begin"
            "       Writeln ('No,else 1 ');"
            "       Writeln ('No,else 2 ');"
            "   end;"
            "   if 5 mod 3 > 0 then begin"
            "       Writeln ('Yes,if 1 ');"
            "       Writeln ('Yes,if 2 ');"
            "   end"
            "   else begin"
            "       Writeln ('No,else 1 ');"
            "       Writeln ('No,else 2 ');"
            "   end;"
            "   res2 := 'Hello world' ;"
            "   num1 := 2 ;"
            "   Writeln ('From table ' , num1);"
            "   if PI <> num1 then begin"
            "       Writeln( 'Pim' );"
            "       if PI <> num1 then begin"
            "           Writeln ('Pam');"
            "       end"
            "   end"
            "   else begin"
            "       Writeln ('Pum');"
            "   end;"
            "   if PI <> num1 then begin"
            "       Writeln( 'Pim' );"
            "       if 5 mod 3 > 0 then begin"
            "           Writeln ('Yes,if 1 ');"
            "           Writeln ('Yes,if 2 ');"
            "       end"
            "       else begin"
            "           Writeln ('No,else 1 ');"
            "           Writeln ('No,else 2 ');"
            "       end;"
            "   end"
            "   else begin"
            "       Writeln ('Pum');"
            "   end;"
            "   for i := 1 to 8 do begin"
            "       Write ( '3' ) ;"
            "   end;"
            "   while num1 < 6 do begin"
            "       Write('Yes');"
            "       num1 := num1 + 1;"
            "   end;"
            "   if 5 mod 3 > 0 then begin"
            "       Writeln ('Yes,if 1 ');"
            "       Writeln ('Yes,if 2 ');"
            "   end"
            "   else begin"
            "       Writeln ('No,else 1 ');"
            "       Writeln ('No,else 2 ');"
            "   end;"
            "   repeat begin"
            "       Write('3');"
            "       num1 := num1 + 1;"
            "   end;"
            "   until num1 < 7 ;"
            "   num1 := num1 + 3;"
            "end.";
 string cCode =
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
    char *tests2[] = {
            // 1. Препроцессор и макросы (включая параметризованные)
            "#include <iostream>\n"
            "#include <vector>\n"
            "#include <string>\n"
            "#include <memory>\n"
            "#include <cmath>\n"
            "#include <cstring>\n"
            "#define MAX_SIZE 100\n"
            "#define SQUARE(x) ((x) * (x))\n"
            "#define MIN(a, b) ((a) < (b) ? (a) : (b))\n",

            // 2. Базовые типы, union и ссылки/указатели
            "const double PI = 3.1415926;\n"
            "int globalVar = 0;\n"
            "int num1 = 10, num2 = 20;\n"
            "double d = 3.14;\n"
            "char c = 'A';\n"
            "bool flag = true;\n"
            "int& ref = num1;\n"
            "int* ptr = &num1;\n"
            "union Data {\n"
            "    int i;\n"
            "    float f;\n"
            "    char c;\n"
            "};\n"
            "Data data;\n",

            // 3. Перечисления (class и обычные)
            "enum Color { RED, GREEN, BLUE };\n"
            "enum class Status { OK, ERROR, PENDING };\n"
            "enum class Byte : unsigned char { ZERO = 0, MAX = 255 };\n",

            // 4. Структуры с конструкторами
            "struct Point {\n"
            "    int x, y;\n"
            "    Point(int x = 0, int y = 0) : x(x), y(y) {}\n"
            "    Point(const Point& other) : x(other.x), y(other.y) {}\n"
            "};\n"
            "struct Rectangle {\n"
            "    Point topLeft;\n"
            "    Point bottomRight;\n"
            "    Rectangle(Point tl, Point br) : topLeft(tl), bottomRight(br) {}\n"
            "};\n",

            // 5. Абстрактный базовый класс с виртуальными функциями
            "class Shape {\n"
            "protected:\n"
            "    Color color;\n"
            "public:\n"
            "    Shape(Color c) : color(c) {}\n"
            "    virtual double area() const = 0;\n"
            "    virtual double perimeter() const = 0;\n"
            "    virtual ~Shape() {}\n"
            "    void setColor(Color c) { color = c; }\n"
            "    Color getColor() const { return color; }\n"
            "    virtual void printInfo() const { std::cout << \"Shape\"; }\n"
            "};\n",

            // 6. Производный класс с ВСЕМИ возможностями
            "class Circle : public Shape {\n"
            "private:\n"
            "    double radius;\n"
            "    static int circleCount;\n"
            "public:\n"
            "    Circle(double r, Color c = RED) : Shape(c), radius(r) { circleCount++; }\n"
            "    Circle(const Circle& other) : Shape(other.color), radius(other.radius) { circleCount++; }\n"
            "    ~Circle() override { circleCount--; }\n"
            "    \n"
            "    double area() const override { return PI * radius * radius; }\n"
            "    double perimeter() const override { return 2 * PI * radius; }\n"
            "    void printInfo() const override { std::cout << \"Circle: \" << radius; }\n"
            "    \n"
            "    Circle& operator=(const Circle& other) {\n"
            "        if (this != &other) {\n"
            "            color = other.color;\n"
            "            radius = other.radius;\n"
            "        }\n"
            "        return *this;\n"
            "    }\n"
            "    \n"
            "    Circle operator+(const Circle& other) const {\n"
            "        return Circle(radius + other.radius, color);\n"
            "    }\n"
            "    \n"
            "    bool operator==(const Circle& other) const {\n"
            "        return radius == other.radius && color == other.color;\n"
            "    }\n"
            "    \n"
            "    Circle& operator++() { radius++; return *this; }\n"
            "    Circle operator++(int) { Circle temp = *this; radius++; return temp; }\n"
            "    \n"
            "    static int getCircleCount() { return circleCount; }\n"
            "    \n"
            "    friend void printCircleInfo(const Circle& c);\n"
            "    friend std::ostream& operator<<(std::ostream& os, const Circle& c);\n"
            "};\n"
            "int Circle::circleCount = 0;\n",

            // 7. Дружественные функции и перегрузка операторов ввода/вывода
            "void printCircleInfo(const Circle& c) {\n"
            "    std::cout << \"Circle radius: \" << c.radius << \", color: \" << static_cast<int>(c.color) << std::endl;\n"
            "}\n"
            "std::ostream& operator<<(std::ostream& os, const Circle& c) {\n"
            "    os << \"Circle(radius=\" << c.radius << \", color=\" << static_cast<int>(c.color) << \")\";\n"
            "    return os;\n"
            "}\n",

            // 8. Шаблонные функции (variadic, перегрузка, специализация)
            "template<typename T>\n"
            "T max(const T& a, const T& b) {\n"
            "    return (a > b) ? a : b;\n"
            "}\n"
            "template<typename T>\n"
            "T min(const T& a, const T& b) {\n"
            "    return (a < b) ? a : b;\n"
            "}\n"
            "template<>\n"
            "const char* max(const char* const & a, const char* const & b) {\n"
            "    return (std::strlen(a) > std::strlen(b)) ? a : b;\n"
            "}\n"
            "template<>\n"
            "const char* min(const char* const & a, const char* const & b) {\n"
            "    return (std::strlen(a) < std::strlen(b)) ? a : b;\n"
            "}\n"
            "template<typename T, typename U>\n"
            "auto add(const T& a, const U& b) -> decltype(a + b) {\n"
            "    return a + b;\n"
            "}\n",

            // 9. Variadic templates и шаблонные классы
            "template<typename... Args>\n"
            "void printAll(Args... args) {\n"
            "    (std::cout << ... << args) << std::endl;\n"
            "}\n"
            "template<typename T>\n"
            "class Container {\n"
            "private:\n"
            "    T* data;\n"
            "    size_t size;\n"
            "public:\n"
            "    Container(size_t s) : size(s) { data = new T[size]; }\n"
            "    ~Container() { delete[] data; }\n"
            "    Container(const Container& other) : size(other.size) {\n"
            "        data = new T[size];\n"
            "        for (size_t i = 0; i < size; ++i) data[i] = other.data[i];\n"
            "    }\n"
            "    Container& operator=(const Container& other) {\n"
            "        if (this != &other) {\n"
            "            delete[] data;\n"
            "            size = other.size;\n"
            "            data = new T[size];\n"
            "            for (size_t i = 0; i < size; ++i) data[i] = other.data[i];\n"
            "        }\n"
            "        return *this;\n"
            "    }\n"
            "    T& operator[](size_t index) { return data[index]; }\n"
            "    const T& operator[](size_t index) const { return data[index]; }\n"
            "    size_t getSize() const { return size; }\n"
            "};\n",

            // 10. Пространства имен (вложенные) и using
            "namespace Math {\n"
            "    int add(int a, int b) { return a + b; }\n"
            "    double add(double a, double b) { return a + b; }\n"
            "    \n"
            "    namespace Geometry {\n"
            "        double calculateDistance(const Point& p1, const Point& p2) {\n"
            "            return std::sqrt(SQUARE(p1.x - p2.x) + SQUARE(p1.y - p2.y));\n"
            "        }\n"
            "        \n"
            "        namespace Advanced {\n"
            "            template<typename T>\n"
            "            T calculateArea(const T& shape) { return shape.area(); }\n"
            "        }\n"
            "    }\n"
            "}\n"
            "using StringVector = std::vector<std::string>;\n"
            "using Math::add;\n"
            "namespace Geo = Math::Geometry;\n",

            // 11. Умные указатели и фабричные функции
            "std::unique_ptr<Circle> createCircle(double radius) {\n"
            "    return std::make_unique<Circle>(radius);\n"
            "}\n"
            "std::shared_ptr<Circle> createSharedCircle(double radius) {\n"
            "    return std::make_shared<Circle>(radius);\n"
            "}\n"
            "void processCircle(const std::unique_ptr<Circle>& circle) {\n"
            "    if (circle) std::cout << \"Circle area: \" << circle->area() << std::endl;\n"
            "}\n",

            // 12. Лямбда-выражения и функциональное программирование
            "auto multiplier = [](double factor) {\n"
            "    return [factor](double value) { return value * factor; };\n"
            "};\n"
            "auto adder = [](auto x) {\n"
            "    return [x](auto y) { return x + y; };\n"
            "};\n"
            "void testLambdas() {\n"
            "    auto doubleIt = multiplier(2.0);\n"
            "    auto tripleIt = multiplier(3.0);\n"
            "    auto addFive = adder(5);\n"
            "    \n"
            "    std::cout << \"Double: \" << doubleIt(5.0) << std::endl;\n"
            "    std::cout << \"Triple: \" << tripleIt(5.0) << std::endl;\n"
            "    std::cout << \"Add five: \" << addFive(10) << std::endl;\n"
            "    \n"
            "    std::vector<int> numbers = {1, 2, 3, 4, 5};\n"
            "    std::for_each(numbers.begin(), numbers.end(), [](int n) {\n"
            "        std::cout << n * n << \" \";\n"
            "    });\n"
            "    std::cout << std::endl;\n"
            "}\n",

            // 13. STL контейнеры и алгоритмы
            "void testSTL() {\n"
            "    std::vector<int> vec = {1, 2, 3, 4, 5};\n"
            "    vec.push_back(6);\n"
            "    \n"
            "    std::string str = \"Hello\";\n"
            "    str += \" World!\";\n"
            "    \n"
            "    std::map<std::string, int> scores = {{\"Alice\", 95}, {\"Bob\", 87}};\n"
            "    scores[\"Charlie\"] = 92;\n"
            "    \n"
            "    for (const auto& [name, score] : scores) {\n"
            "        std::cout << name << \": \" << score << std::endl;\n"
            "    }\n"
            "}\n",

            // 14. Исключения и обработка ошибок
            "void testExceptions() {\n"
            "    try {\n"
            "        int value = -5;\n"
            "        if (value < 0) {\n"
            "            throw std::invalid_argument(\"Value cannot be negative\");\n"
            "        }\n"
            "        \n"
            "        std::vector<int> vec(10);\n"
            "        if (vec.size() > 100) {\n"
            "            throw std::out_of_range(\"Vector too large\");\n"
            "        }\n"
            "        \n"
            "    } catch (const std::invalid_argument& e) {\n"
            "        std::cout << \"Invalid argument: \" << e.what() << std::endl;\n"
            "    } catch (const std::out_of_range& e) {\n"
            "        std::cout << \"Out of range: \" << e.what() << std::endl;\n"
            "    } catch (const std::exception& e) {\n"
            "        std::cout << \"General exception: \" << e.what() << std::endl;\n"
            "    }\n"
            "}\n",

            // 15. RTTI и dynamic_cast
            "void testRTTI() {\n"
            "    Circle circle(5.0);\n"
            "    Shape* shape = &circle;\n"
            "    \n"
            "    Circle* circlePtr = dynamic_cast<Circle*>(shape);\n"
            "    if (circlePtr) {\n"
            "        std::cout << \"Successful dynamic cast to Circle\" << std::endl;\n"
            "    }\n"
            "    \n"
            "    if (typeid(*shape) == typeid(Circle)) {\n"
            "        std::cout << \"Shape is actually a Circle\" << std::endl;\n"
            "    }\n"
            "    \n"
            "    std::cout << \"Type name: \" << typeid(*shape).name() << std::endl;\n"
            "}\n",

            // 16. Все операторы и выражения
            "void testOperators() {\n"
            "    int a = 10, b = 3;\n"
            "    \n"
            "    // Арифметические\n"
            "    int sum = a + b;\n"
            "    int diff = a - b;\n"
            "    int product = a * b;\n"
            "    int quotient = a / b;\n"
            "    int remainder = a % b;\n"
            "    \n"
            "    // Сравнения\n"
            "    bool equal = (a == b);\n"
            "    bool notEqual = (a != b);\n"
            "    bool greater = (a > b);\n"
            "    bool less = (a < b);\n"
            "    \n"
            "    // Логические\n"
            "    bool andResult = (a > 5 && b < 10);\n"
            "    bool orResult = (a < 5 || b > 10);\n"
            "    bool notResult = !(a == b);\n"
            "    \n"
            "    // Побитовые\n"
            "    int bitAnd = a & b;\n"
            "    int bitOr = a | b;\n"
            "    int bitXor = a ^ b;\n"
            "    int bitNot = ~a;\n"
            "    int shiftLeft = a << 1;\n"
            "    int shiftRight = a >> 1;\n"
            "    \n"
            "    // Составные присваивания\n"
            "    a += b;\n"
            "    a -= b;\n"
            "    a *= b;\n"
            "    a /= b;\n"
            "    a %= b;\n"
            "    a &= b;\n"
            "    a |= b;\n"
            "    a ^= b;\n"
            "    a <<= 1;\n"
            "    a >>= 1;\n"
            "    \n"
            // ... и так далее для всех операторов
            "}\n",

            // 17. Control structures (условия, циклы, switch)
            "void testControlStructures() {\n"
            "    // If-else if-else\n"
            "    int score = 85;\n"
            "    if (score >= 90) {\n"
            "        std::cout << \"A\" << std::endl;\n"
            "    } else if (score >= 80) {\n"
            "        std::cout << \"B\" << std::endl;\n"
            "    } else if (score >= 70) {\n"
            "        std::cout << \"C\" << std::endl;\n"
            "    } else {\n"
            "        std::cout << \"F\" << std::endl;\n"
            "    }\n"
            "    \n"
            "    // Switch с enum class\n"
            "    Status status = Status::PENDING;\n"
            "    switch (status) {\n"
            "        case Status::OK:\n"
            "            std::cout << \"Everything is OK\" << std::endl;\n"
            "            break;\n"
            "        case Status::ERROR:\n"
            "            std::cout << \"An error occurred\" << std::endl;\n"
            "            break;\n"
            "        case Status::PENDING:\n"
            "            std::cout << \"Operation pending\" << std::endl;\n"
            "            break;\n"
            "    }\n"
            "    \n"
            "    // Все виды циклов\n"
            "    for (int i = 0; i < 5; ++i) {\n"
            "        std::cout << i << \" \";\n"
            "    }\n"
            "    std::cout << std::endl;\n"
            "    \n"
            "    int j = 0;\n"
            "    while (j < 3) {\n"
            "        std::cout << j << \" \";\n"
            "        j++;\n"
            "    }\n"
            "    std::cout << std::endl;\n"
            "    \n"
            "    int k = 0;\n"
            "    do {\n"
            "        std::cout << k << \" \";\n"
            "        k++;\n"
            "    } while (k < 3);\n"
            "    std::cout << std::endl;\n"
            "    \n"
            "    // Range-based for\n"
            "    std::vector<int> numbers = {1, 2, 3, 4, 5};\n"
            "    for (const auto& num : numbers) {\n"
            "        std::cout << num << \" \";\n"
            "    }\n"
            "    std::cout << std::endl;\n"
            "}\n",

            // 18. Главная функция, объединяющая всё
            "int main() {\n"
            "    testLambdas();\n"
            "    testSTL();\n"
            "    testExceptions();\n"
            "    testRTTI();\n"
            "    testOperators();\n"
            "    testControlStructures();\n"
            "    \n"
            "    // Демонстрация всех возможностей\n"
            "    auto circle = createCircle(10.0);\n"
            "    std::cout << *circle << std::endl;\n"
            "    \n"
            "    Container<int> container(5);\n"
            "    for (size_t i = 0; i < container.getSize(); ++i) {\n"
            "        container[i] = static_cast<int>(i * i);\n"
            "    }\n"
            "    \n"
            "    std::cout << \"Math result: \" << Math::add(5, 3.5) << std::endl;\n"
            "    std::cout << \"Distance: \" << Geo::calculateDistance(Point(0,0), Point(3,4)) << std::endl;\n"
            "    \n"
            "    printAll(\"Final\", \" \", \"result\", \": \", \"SUCCESS!\");\n"
            "    \n"
            "    return 0;\n"
            "}\n"
    };
    const int TESTS_COUNT = 18;


    //PASCAL TESTING

    /* cout << "=== ORIGINAL CODE ===\n" << pascalCode << "\n\n";

    // STEP 1: Parse Pascal -> Expression* (parseOnly - без выполнения)
    Lexer lexer(pascalCode, PASCAL);
    PascalParserToExpression parser(lexer, PASCAL);
    parser.parseOnly();  // Только структура, без выполнения

    const auto &exprs = parser.getExpressionsOnly();
    cout << "Parsing: created " << exprs.size() << " expressions\n\n";

    PascalToJSON exporter2;
    std::string title =  parser.getTitle();

    std::string mmd2 = exporter2.build(exprs,title);
    //std::cout<<"Output of mmd2: \n"<<mmd2<<"\n";
    // Экспорт блок-схемы по существующим Expression (Mermaid и JSON)
    std::ofstream f("flowchart.json");
    f << mmd2;
    f.close();
    cout << "Mermaid saved to flowchart.json\n\n";

    // STEP 3: Import back from JSON and generate Pascal using PascalCodeGenerator
    PascalCodeGenerator codeGenerator;
    nlohmann::json jsonData = nlohmann::json::parse(mmd2);
    std::string restoredPascal = codeGenerator.generatePascal(jsonData);

    cout << "Import: restored Pascal code directly\n\n";

    cout << "=== RESTORED CODE ===\n" << restoredPascal << "\n\n";
    // Comparison
    if (pascalCode == restoredPascal) {
        cout << "SUCCESS: Code is identical!\n";
    } else {
        cout << "NOTE: Code differs (formatting may vary)\n";
        cout << "Main structures should match.\n";
    }
*/

    //C TESTING

    /*
  char *tests[21];
  tests[0] =
          "#include <stdio.h>\n"
          "#include <stdlib.h>\n"
          "#include <string.h>\n"
          "#define PI 3.1415926\n"
          "#define MAX_SIZE 100\n";

  tests[1] =
          "int addNumbers(int a, int b);\n"
          "void greetUser(char* name);\n"
          "float calculateCircleArea(float radius);\n";

  tests[2] =
          "const float PI_CONST = 3.1415926f;\n"
          "int globalCounter = 0;\n";

  tests[3] =
          "struct Point {\n"
          "    int x;\n"
          "    int y;\n"
          "};\n"
          "\n"
          "typedef struct Point Point;\n";

  tests[4] =
          "void test() {\n"
          "    int num1, num2, i;\n"
          "    float res, d;\n"
          "    char res2[50];\n"
          "    int array[10];\n"
          "    Point p1;\n"
          "}\n";

  tests[5] =
          "void test() {\n"
          "    printf(\"Enter a number: \");\n"
          "    scanf(\"%f\", &res);\n"
          "    printf(\"From input: %.2f\\n\", res);\n"
          "}\n";

  tests[6] =
          "void test() {\n"
          "    num1 = 12 / 2;\n"
          "    num2 = 15 % 4;\n"
          "    num1 = addNumbers(5, 3);\n"
          "}\n";
  tests[7] =
          "void test() {\n"
          "    switch(num1) {\n"
          "        case 1:\n"
          "        case 2:\n"
          "        case 3:\n"
          "        case 4:\n"
          "        case 5:\n"
          "            printf(\"Switch works\\n\");\n"
          "            break;\n"
          "        default:\n"
          "            printf(\"Switch no works\\n\");\n"
          "            break;\n"
          "    }\n"
          "}\n";
  tests[8] =
          "void test() {\n"
          "    if (5 % 3 > 0) {\n"
          "        printf(\"Yes,if 1\\n\");\n"
          "        printf(\"Yes,if 2\\n\");\n"
          "    } else {\n"
          "        printf(\"No,else 1\\n\");\n"
          "        printf(\"No,else 2\\n\");\n"
          "    }\n"
          "}\n";
  tests[9] =
          "void test() {\n"
          "    strcpy(res2, \"Hello world\");\n"
          "    num1 = 2;\n"
          "}\n";
  tests[10] =
          "void test() {\n"
          "    if (PI_CONST != num1) {\n"
          "        printf(\"Pim\\n\");\n"
          "        if (PI_CONST != num1) {\n"
          "            printf(\"Pam\\n\");\n"
          "        }\n"
          "    } else {\n"
          "        printf(\"Pum\\n\");\n"
          "    }\n"
          "}\n";
  tests[11] =
          "void test() {\n"
          "    for (i = 1; i <= 8; i++) {\n"
          "        printf(\"3\");\n"
          "    }\n"
          "}\n";
  tests[12] =
          "void test() {\n"
          "    while (num1 < 6) {\n"
          "        printf(\"Yes\");\n"
          "        num1 = num1 + 1;\n"
          "    }\n"
          "}\n";
  tests[13] =
          "void test() {\n"
          "    do {\n"
          "        printf(\"3\");\n"
          "        num1 = num1 + 1;\n"
          "    } while (num1 < 7);\n"
          "}\n";
  tests[14] =
          "void test() {\n"
          "    for (i = 0; i < 10; i++) {\n"
          "        array[i] = i * 2;\n"
          "    }\n"
          "}\n";
  tests[15] =
          "void test() {\n"
          "    p1.x = 10;\n"
          "    p1.y = 20;\n"
          "    int* ptr = &num1;\n"
          "    *ptr = *ptr + 3;\n"
          "}\n";
  tests[16] =
          "void test() {\n"
          "    num1 = num1 + 3;\n"
          "    num1 += 5;\n"
          "    num1++;\n"
          "}\n";
  tests[17] =
          "void test() {\n"
          "    if (num1 > 10 && num2 < 20 || !(num1 == 15)) {\n"
          "        printf(\"Complex condition works\\n\");\n"
          "    }\n"
          "}\n";
  tests[18] =
          "void test() {\n"
          "    num1 = num1 & 0xFF;\n"
          "    num2 = num1 | 0x0F;\n"
          "}\n";
  tests[19] =
          "void test() {\n"
          "    int result = (num1 > num2) ? num1 : num2;\n"
          "}\n";
  tests[20] =
          "int addNumbers(int a, int b) {\n"
          "    return a + b;\n"
          "}\n"
          "\n"
          "void greetUser(char* name) {\n"
          "    printf(\"Hello, %s!\\n\", name);\n"
          "}\n"
          "\n"
          "float calculateCircleArea(float radius) {\n"
          "    return PI_CONST * radius * radius;\n"
          "}\n";
          */

    /* CParserToAST parserC;
    AstToJsonConverter converter;
    CCodeGenerator generator;
    try{
        auto result = parserC.parse(cCode);
        nlohmann::json mmd3 = converter.convertProgram(result);
        std::ofstream f2("flowchart2.json");
        f2 << mmd3.dump(4);
        f2.close();
        std::string restored_codeC= generator.generate(mmd3);
        std::cout<<std::endl<<restored_codeC;
    } catch (const std::exception& e) {
        std::cout << "ERROR in part " << (i + 1) << ": " << e.what() << std::endl;
    }catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }



//        for (int i = 0; i < 21; i++) {
//            if (i == 16) continue;
//            auto ast = parser.parse(tests[i]);
//            //ast->printTree();
//            if (ast) {
//                std::cout << "AST parsed successfully!\n";
//                std::cout << "AST Structure:\n";
//
//                AstVisualizer visualizer;
//                ast->accept(visualizer);
//            } else {
//                std::cout << "Failed to parse AST\n";
//            }
//        }

*/


    //CPP TESTING

/*    for (int i = 0; i < TESTS_COUNT; i++) {
        std::cout << "=== Testing part " << (i + 1) << " ===" << std::endl;
        try {
            CppParserToAST parser;
            std::cout << "TEST cppCode" << std::endl;
            std::cout << tests2[i] << std::endl;
            auto result = parser.parse(tests2[i]);
            std::cout << "SUCCESS: Part " << (i + 1) << " parsed successfully" << std::endl;
            CppAstVisualizer visualizer;
            std::cout << "AST STRUCTURE:" << std::endl;
            result->accept(visualizer);
        } catch (const std::exception& e) {
            std::cout << "ERROR in part " << (i + 1) << ": " << e.what() << std::endl;
        }catch (...) {
            std::cerr << "Unknown error occurred" << std::endl;
            return 1;
        }
        std::cout << std::endl;
    }*/

    try {
        CppParserToAST parser;
        int k = 5;
        auto ast = parser.parse(tests2[k]);
        std::cout << "TEST cppCode" << std::endl;
        std::cout << tests2[k] << std::endl;
        CppAstVisualizer visualizer;
        std::cout << "AST STRUCTURE:" << std::endl;
        ast->accept(visualizer);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }


    return 0;
}
