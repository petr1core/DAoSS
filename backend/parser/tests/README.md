# Модульные тесты для парсера Pascal

Этот каталог содержит модульные тесты для проверки синтаксиса Pascal кода.

## Структура

- `TestFramework.h` - простой фреймворк для модульных тестов
- `PascalSyntaxTests.cpp` - основные тесты для проверки синтаксиса Pascal

## Описание тестов

### Тесты для валидного кода (Positive Tests)

Проверяют, что парсер корректно обрабатывает валидный Pascal код:

- Минимальная программа (`program Test; begin end.`)
- Программа с переменными
- Программа с присваиванием
- Программа с константами
- Программа с условными операторами (if-else)
- Программа с циклами (for, while)
- Программа с процедурами и функциями

### Тесты для синтаксических ошибок (Negative Tests)

Проверяют, что парсер корректно обнаруживает синтаксические ошибки:

- Отсутствие точки в конце программы
- Отсутствие точки с запятой после `program`
- Отсутствие `begin` или `end`
- Неправильные ключевые слова
- Отсутствие точек с запятой в объявлениях
- Незакрытые блоки
- Отсутствие `then` в `if`
- Отсутствие `do` в цикле `for`
- Неправильный оператор присваивания (`=` вместо `:=`)
- Отсутствие двоеточия в объявлении переменной
- Пустая программа
- Незакрытые скобки
- Незакрытые строковые литералы
- И другие синтаксические ошибки

## Сборка тестов

Тесты автоматически собираются вместе с основным проектом при использовании CMake.

### Windows (Visual Studio)

```powershell
cd backend_and_parser\src\parser\Parser\backend\parser\build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux/macOS

```bash
cd backend_and_parser/src/parser/Parser/backend/parser/build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Запуск тестов

После сборки тесты можно запустить:

### Windows

```powershell
# Из директории build
.\Release\pascal-syntax-tests.exe

# Или из Debug конфигурации
.\Debug\pascal-syntax-tests.exe
```

### Linux/macOS

```bash
# Из директории build
./pascal-syntax-tests
```

## Пример вывода

```
========================================
Запуск модульных тестов парсера Pascal
========================================

[TEST] Валидный код: минимальная программа ... PASSED
[TEST] Валидный код: программа с переменными ... PASSED
[TEST] Ошибка: отсутствует точка в конце ... PASSED
[TEST] Ошибка: отсутствует begin ... PASSED
...

========================================
Результаты тестирования:
  Пройдено: 25
  Провалено: 0
  Всего: 25
========================================
```

## Добавление новых тестов

Чтобы добавить новый тест, откройте файл `PascalSyntaxTests.cpp` и добавьте новый тест:

```cpp
framework.addTest("Описание теста", []() {
    std::string code = "program Test; begin end.";
    ASSERT_TRUE(isValidPascalCode(code)); // или другая проверка
});
```

### Доступные макросы для проверок

- `ASSERT_TRUE(condition)` - проверяет, что условие истинно
- `ASSERT_FALSE(condition)` - проверяет, что условие ложно
- `ASSERT_EQ(expected, actual)` - проверяет равенство
- `ASSERT_NE(expected, actual)` - проверяет неравенство
- `ASSERT_THROWS(statement, exceptionType)` - проверяет, что выбрасывается исключение

### Вспомогательные функции

- `isValidPascalCode(code)` - возвращает `true`, если код валиден
- `hasParserErrors(code)` - возвращает `true`, если есть ошибки парсера
- `throwsException(code)` - возвращает `true`, если парсер выбрасывает исключение

## Интеграция с CI/CD

Тесты можно интегрировать в систему непрерывной интеграции. Пример для GitHub Actions:

```yaml
- name: Build tests
  run: |
    cd backend_and_parser/src/parser/Parser/backend/parser/build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release

- name: Run tests
  run: |
    cd backend_and_parser/src/parser/Parser/backend/parser/build
    ./pascal-syntax-tests
```

## Отладка тестов

Если тест провалился, проверьте:

1. Что код действительно содержит синтаксическую ошибку (для negative tests)
2. Что парсер правильно обрабатывает этот случай
3. Что исключения правильно перехватываются

Для отладки можно добавить вывод в тест:

```cpp
framework.addTest("Тест с отладкой", []() {
    std::string code = "program Test; begin end.";
    std::cout << "Тестируемый код: " << code << std::endl;
    bool result = isValidPascalCode(code);
    std::cout << "Результат: " << (result ? "валиден" : "невалиден") << std::endl;
    ASSERT_TRUE(result);
});
```

