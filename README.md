# Parser Service - C++ HTTP сервер для парсинга кода

HTTP сервер на C++ для парсинга исходного кода на языках Pascal, C и C++ с генерацией AST/SPR представлений и блок-схем.

## Описание

Parser Service — это отдельный HTTP сервер, который предоставляет API для:
- Парсинга кода в структурированное представление (AST для C/C++, SPR для Pascal)
- Валидации синтаксиса кода
- Генерации блок-схем в формате JSON
- Генерации кода из AST/SPR

## Архитектура

### Поддерживаемые языки

- **Pascal** — парсинг в SPR (Structured Program Representation)
- **C** — парсинг в AST (Abstract Syntax Tree)
- **C++** — парсинг в AST (Abstract Syntax Tree)

### Компоненты парсера

```
Parser/
├── Ast/              # AST структуры для C/C++ (Expr, Stmt, Visitor)
├── CPPAst/           # AST структуры для C++ (CppAst, CppDecl, CppExpr, CppStmt)
├── Expression/       # Выражения для Pascal (Function, Procedure, StatementExpression)
├── Parser/           # Парсеры языков
│   ├── PascalParserToExpression.h
│   ├── CParserToAST.h
│   ├── CppParserToAST.h
│   └── ErrorCollector.h
├── CodeGen/          # Генераторы кода
│   ├── CCodeGenerator.h
│   ├── CppCodeGenerator.h
│   └── PascalCodeGenerator.h
├── Flowchart/        # Экспорт в JSON для блок-схем
│   ├── ExporterJson.h
│   └── FromExpressions.h
├── Scripts/          # Вспомогательные компоненты
│   ├── Lexer.h
│   ├── Token.h
│   ├── Stack.h
│   └── SearchTreeTable.h
├── thirdparty/       # Внешние библиотеки
│   └── httplib.h     # cpp-httplib для HTTP сервера
├── json-3.12.0/      # Библиотека nlohmann/json
├── main.cpp          # Точка входа, HTTP сервер
└── CMakeLists.txt    # Конфигурация сборки
```

### Структура данных

**AST (Abstract Syntax Tree)** — для C/C++:
- Иерархическая структура узлов (Expr, Stmt)
- Поддержка всех конструкций языка
- Возможность обхода через Visitor паттерн

**SPR (Structured Program Representation)** — для Pascal:
- Представление через Expression деревья
- Функции, процедуры, выражения
- Поддержка всех конструкций Pascal

## Требования

### Для сборки
- **CMake** 3.15 или выше
- **C++ компилятор** с поддержкой C++17:
  - Windows: MSVC (Visual Studio 2019+) или MinGW-w64
  - Linux: GCC 7+ или Clang 5+
  - macOS: Xcode Command Line Tools (Clang)

### Зависимости
Все зависимости включены в репозиторий:
- `cpp-httplib` (header-only) — в `thirdparty/`
- `nlohmann/json` — в `json-3.12.0/`
- `jwt-cpp` — для JWT авторизации (если используется)

## Сборка

### Автоматическая сборка

Используйте скрипт `start-all.ps1` с флагом `-BuildParser`:

```powershell
.\start-all.ps1 -BuildParser
```

### Ручная сборка

```bash
# Перейти в директорию парсера
cd backend_and_parser/src/parser/Parser

# Создать директорию для сборки
mkdir build
cd build

# Генерировать файлы сборки
cmake ..

# Собрать проект
cmake --build . --config Release

# На Windows (MSVC)
cmake --build . --config Release

# На Linux/macOS
cmake --build . --config Release
# или просто
make
```

### Результат сборки

После успешной сборки исполняемый файл будет находиться в:
- Windows: `build/Release/parser-server.exe` или `build/Debug/parser-server.exe`
- Linux/macOS: `build/parser-server`

## Запуск

### Автоматический запуск

Используйте скрипт `start-all.ps1`:

```powershell
.\start-all.ps1
```

### Ручной запуск

```bash
# Из директории build
cd build
./parser-server 8080

# Или указать порт явно
./parser-server 8080
```

**Важно:** Парсер должен быть запущен **перед** запуском Backend, так как Backend делает HTTP запросы к парсеру.

## API Эндпоинты

Парсер предоставляет следующие HTTP эндпоинты:

### POST `/api/parse`

Парсит код в AST/SPR представление.

**Запрос:**
```json
{
  "code": "program Test; begin writeln('Hello'); end.",
  "language": "pascal"
}
```

**Ответ:**
```json
{
  "ast": { ... },
  "errors": []
}
```

**Поддерживаемые языки:** `pascal`, `c`, `cpp`

### POST `/api/validate`

Валидирует синтаксис кода с детальными ошибками.

**Запрос:**
```json
{
  "code": "program Test; begin end.",
  "language": "pascal"
}
```

**Ответ:**
```json
{
  "isValid": true,
  "errors": []
}
```

### POST `/api/validate/simple`

Упрощенная валидация — возвращает только статус.

**Запрос:**
```json
{
  "code": "program Test; begin end.",
  "language": "pascal"
}
```

**Ответ:**
```json
{
  "isValid": true,
  "errorCount": 0
}
```

### Авторизация

Все эндпоинты требуют JWT токен в заголовке:
```
Authorization: Bearer <token>
```

## Интеграция с Backend

### Как Backend обращается к парсеру

1. Клиент отправляет запрос на Backend (`/api/parser/parse` или `/api/parser/validate`)
2. `ParserController` в Backend получает запрос
3. `ParserService` делает HTTP запрос к парсер-сервису на `http://localhost:8080`
4. Парсер обрабатывает код и возвращает результат
5. Backend маппит ответ и возвращает клиенту

### Формат запросов/ответов

**Запрос от Backend к Parser:**
```json
POST http://localhost:8080/api/parse
Content-Type: application/json
Authorization: Bearer <jwt_token>

{
  "code": "...",
  "language": "pascal"
}
```

**Ответ от Parser:**
```json
{
  "ast": { ... },
  "errors": []
}
```

### Настройка в Backend

Backend настраивает подключение к парсеру через:
- URL: `http://localhost:8080` (по умолчанию)
- Таймаут: настраивается в `ParserService`
- JWT токен: передается из запроса клиента

## Ветка

Парсер использует ветку **`http-server_wip`** (не `main`).

При настройке подмодулей скрипт `setup-submodules.ps1` автоматически переключает парсер на эту ветку.

## Отладка

### Проверка работоспособности

```bash
# Проверить, что сервер запущен
curl http://localhost:8080/api/validate/simple \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"code":"program Test; begin end.","language":"pascal"}'
```

### Логи

Парсер выводит логи в консоль:
- Запросы и ответы
- Ошибки парсинга
- Ошибки HTTP сервера

### Проблемы с портом

Если порт 8080 занят:
```bash
# Windows
netstat -ano | findstr :8080

# Linux/macOS
lsof -i :8080
```

Измените порт при запуске:
```bash
./parser-server 8081
```

И обновите настройки в Backend.

## Разработка

### Добавление поддержки нового языка

1. Создать новый парсер в `Parser/`
2. Добавить AST структуры (если нужны)
3. Добавить обработку в `main.cpp`
4. Обновить документацию

### Расширение функционала

- Генерация кода: использовать классы из `CodeGen/`
- Экспорт блок-схем: использовать `Flowchart/ExporterJson.h`
- Обработка ошибок: использовать `ErrorCollector`

## Дополнительная документация

- [PARSER_API_TESTING.md](../../PARSER_API_TESTING.md) — подробное описание тестирования API
- [Backend README](../../README.md) — документация Backend и интеграции

## Troubleshooting

### Ошибка сборки: CMake не найден
```bash
# Установить CMake
# Windows: скачать с cmake.org или через chocolatey
choco install cmake

# Linux
sudo apt-get install cmake

# macOS
brew install cmake
```

### Ошибка сборки: компилятор не найден
```bash
# Windows: установить Visual Studio Build Tools или MinGW
# Linux
sudo apt-get install build-essential

# macOS
xcode-select --install
```

### Парсер не запускается
- Проверьте, что порт 8080 свободен
- Проверьте права доступа к исполняемому файлу
- Проверьте логи в консоли

### Backend не может подключиться к парсеру
- Убедитесь, что парсер запущен
- Проверьте URL в настройках Backend
- Проверьте firewall/брандмауэр
