//
// Простой фреймворк для модульных тестов
//
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>

class TestFramework {
private:
    struct TestCase {
        std::string name;
        std::function<void()> testFunc;
        bool passed;
        std::string errorMessage;
    };
    
    std::vector<TestCase> tests;
    int passedCount = 0;
    int failedCount = 0;
    
public:
    void addTest(const std::string& name, std::function<void()> testFunc) {
        tests.push_back({name, testFunc, false, ""});
    }
    
    void runAll() {
        std::cout << "========================================\n";
        std::cout << "Запуск модульных тестов парсера Pascal\n";
        std::cout << "========================================\n\n";
        
        for (auto& test : tests) {
            std::cout << "[TEST] " << test.name << " ... ";
            try {
                test.testFunc();
                test.passed = true;
                passedCount++;
                std::cout << "PASSED\n";
            } catch (const std::exception& e) {
                test.passed = false;
                test.errorMessage = e.what();
                failedCount++;
                std::cout << "FAILED\n";
                std::cout << "  Ошибка: " << e.what() << "\n";
            } catch (...) {
                test.passed = false;
                test.errorMessage = "Неизвестная ошибка";
                failedCount++;
                std::cout << "FAILED\n";
                std::cout << "  Ошибка: Неизвестная ошибка\n";
            }
        }
        
        std::cout << "\n========================================\n";
        std::cout << "Результаты тестирования:\n";
        std::cout << "  Пройдено: " << passedCount << "\n";
        std::cout << "  Провалено: " << failedCount << "\n";
        std::cout << "  Всего: " << (passedCount + failedCount) << "\n";
        std::cout << "========================================\n";
        
        if (failedCount > 0) {
            std::cout << "\nДетали проваленных тестов:\n";
            for (const auto& test : tests) {
                if (!test.passed) {
                    std::cout << "  - " << test.name << ": " << test.errorMessage << "\n";
                }
            }
        }
    }
    
    int getExitCode() const {
        return failedCount > 0 ? 1 : 0;
    }
};

// Макросы для удобства
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #condition << " is false"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " << #condition << " is true"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: expected " << (expected) << ", got " << (actual); \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: expected not equal, but both are " << (expected); \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_THROWS(statement, exceptionType) \
    do { \
        bool threw = false; \
        try { \
            statement; \
        } catch (const exceptionType&) { \
            threw = true; \
        } catch (...) { \
            throw std::runtime_error("Expected " #exceptionType " but got different exception"); \
        } \
        if (!threw) { \
            throw std::runtime_error("Expected exception " #exceptionType " was not thrown"); \
        } \
    } while(0)

#endif // TEST_FRAMEWORK_H

