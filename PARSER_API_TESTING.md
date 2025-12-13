# Тестирование API парсера

Документация по тестированию эндпоинтов парсера и работе с парсер-сервисом.

## Содержание

1. [Быстрый старт](#быстрый-старт)
2. [Архитектура](#архитектура)
3. [Эндпоинты API](#эндпоинты-api)
4. [Примеры запросов](#примеры-запросов)
5. [Тестирование через Swagger](#тестирование-через-swagger)

## Быстрый старт

Для работы системы нужно запустить **два сервера**:

1. **Парсер-сервис (C++)** — порт **8080**
   - Запуск: `./parser-server 8080` (или `parser-server.exe` на Windows)
   - URL: `http://localhost:8080`

2. **WebApi (ASP.NET Core)** — порты **5143** (HTTP) и **7143** (HTTPS)
   - Запуск: `dotnet run --project src/WebApi/DAOSS.WebApi.csproj`
   - URL: `http://localhost:5143` или `https://localhost:7143`

**Важно:** Сначала запустите парсер-сервис, затем WebApi, так как WebApi делает HTTP запросы к парсер-сервису.

### Автоматический запуск

Используйте скрипт из корня проекта для автоматического запуска всех модулей:

```powershell
# Из корня DAoSS/
# Базовый запуск (требует, чтобы всё уже было собрано)
.\start-all.ps1

# С полной подготовкой (сборка + миграции)
.\start-all.ps1 -BuildParser -BuildBackend -UpdateMigrations

# Только сборка парсера
.\start-all.ps1 -BuildParser
```

Скрипт автоматически:
1. Собирает Parser (если указан флаг `-BuildParser`)
2. Запускает Parser (порт 8080)
3. Применяет миграции (если указан флаг `-UpdateMigrations`)
4. Собирает Backend (если указан флаг `-BuildBackend`)
5. Запускает Backend (порты 5143/7143)
6. Запускает Frontend (порт 5173)

### Ручной запуск

```bash
# Терминал 1: Запуск парсер-сервиса
cd backend_and_parser/src/parser/Parser/build
./parser-server 8080  # или parser-server.exe на Windows

# Терминал 2: Запуск WebApi
cd backend_and_parser/src/WebApi
dotnet run --project DAOSS.WebApi.csproj
```

## Архитектура

```
┌─────────────┐         ┌──────────────┐         ┌─────────────┐
│   Client    │ ──────> │  WebApi      │ ──────> │   Parser    │
│  (Frontend) │         │  (Backend)   │         │   Service   │
└─────────────┘         └──────────────┘         └─────────────┘
                              │
                              │ HTTP
                              ▼
                        ┌─────────────┐
                        │   Parser    │
                        │   (C++)     │
                        │  localhost  │
                        │   :8080     │
                        └─────────────┘
```

**Поток данных:**
1. Клиент отправляет запрос на бэкенд (`/api/parser/validate` или `/api/parser/parse`)
2. Бэкенд получает запрос через `ParserController`
3. `ParserController` вызывает `IParserService`
4. `ParserService` делает HTTP запрос к парсер-сервису (C++ сервис на порту 8080)
5. Парсер-сервис обрабатывает код и возвращает результат
6. Результат возвращается клиенту

## Эндпоинты API

### POST `/api/parser/parse`

Парсит код в AST/SPR представление.

**Требования:**
- Авторизация: Да (JWT токен в заголовке `Authorization: Bearer <token>`)
- Content-Type: `application/json`

**Запрос:**
```json
{
  "code": "program Test; begin end.",
  "language": "pascal"
}
```

**Поддерживаемые языки:** `pascal`, `c`, `cpp`

**Ответ (успех):**
```json
{
  "success": true,
  "representation": { ... },
  "representationType": "SPR",
  "error": null,
  "lexerErrors": [],
  "parserErrors": []
}
```

**Ответ (ошибка парсинга):**
```json
{
  "success": false,
  "representation": null,
  "representationType": null,
  "error": "Parsing failed",
  "lexerErrors": [
    {
      "position": 10,
      "line": 1,
      "column": 10,
      "type": "LexerError",
      "message": "Unexpected character",
      "value": "x",
      "expected": null,
      "found": null
    }
  ],
  "parserErrors": []
}
```

**Коды ответов:**
- `200 OK` — успешный парсинг
- `400 Bad Request` — неверный формат запроса
- `401 Unauthorized` — отсутствует или неверный JWT токен
- `503 Service Unavailable` — парсер-сервис недоступен
- `504 Gateway Timeout` — таймаут запроса к парсер-сервису
- `500 Internal Server Error` — внутренняя ошибка сервера

### POST `/api/parser/validate`

Валидирует синтаксис кода с детальными ошибками.

**Требования:**
- Авторизация: Да (JWT токен в заголовке `Authorization: Bearer <token>`)
- Content-Type: `application/json`

**Запрос:**
```json
{
  "code": "program Test; begin end.",
  "language": "pascal"
}
```

**Ответ (валидный код):**
```json
{
  "valid": true,
  "lexerErrors": [],
  "parserErrors": []
}
```

**Ответ (невалидный код):**
```json
{
  "valid": false,
  "lexerErrors": [
    {
      "position": 10,
      "line": 1,
      "column": 10,
      "type": "LexerError",
      "message": "Unexpected character",
      "value": "x",
      "expected": null,
      "found": null
    }
  ],
  "parserErrors": [
    {
      "position": 15,
      "line": 1,
      "column": 15,
      "type": "ParserError",
      "message": "Expected ';'",
      "value": null,
      "expected": ";",
      "found": "end"
    }
  ]
}
```

**Коды ответов:**
- `200 OK` — валидация выполнена (код может быть валидным или невалидным)
- `400 Bad Request` — неверный формат запроса
- `401 Unauthorized` — отсутствует или неверный JWT токен
- `503 Service Unavailable` — парсер-сервис недоступен
- `504 Gateway Timeout` — таймаут запроса к парсер-сервису
- `500 Internal Server Error` — внутренняя ошибка сервера

### POST `/api/parser/validate/simple`

Упрощенный эндпоинт для быстрой проверки валидности (возвращает только статус и количество ошибок).

**Требования:**
- Авторизация: Да (JWT токен в заголовке `Authorization: Bearer <token>`)
- Content-Type: `application/json`

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
  "valid": true,
  "hasErrors": false,
  "lexerErrorsCount": 0,
  "parserErrorsCount": 0
}
```

**Коды ответов:**
- `200 OK` — валидация выполнена
- `400 Bad Request` — неверный формат запроса
- `401 Unauthorized` — отсутствует или неверный JWT токен
- `503 Service Unavailable` — парсер-сервис недоступен
- `504 Gateway Timeout` — таймаут запроса к парсер-сервису
- `500 Internal Server Error` — внутренняя ошибка сервера

### Структура ParserError

Все эндпоинты возвращают ошибки в едином формате:

```json
{
  "position": 10,        // Позиция в тексте (символы от начала)
  "line": 1,             // Номер строки (начиная с 1)
  "column": 10,          // Номер колонки (начиная с 1)
  "type": "LexerError",  // Тип ошибки: "LexerError" или "ParserError"
  "message": "...",      // Сообщение об ошибке
  "value": "...",        // Значение, вызвавшее ошибку (опционально)
  "expected": "...",     // Ожидаемое значение (для ParserError)
  "found": "..."         // Найденное значение (для ParserError)
}
```

> **Примечание:** Подробные примеры ответов и схемы доступны в Swagger UI.

## Примеры запросов

### Получение JWT токена

Перед тестированием парсера необходимо получить JWT токен:

```bash
# Регистрация нового пользователя
curl -X POST http://localhost:5143/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "email": "test@example.com",
    "password": "TestPassword123!",
    "name": "Test User"
  }'

# Вход и получение токена
curl -X POST http://localhost:5143/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "email": "test@example.com",
    "password": "TestPassword123!"
  }'

# Ответ содержит токен:
# {
#   "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
#   "user": { ... }
# }
```

### cURL

```bash
# Установите токен в переменную
TOKEN="your_jwt_token_here"

# Валидация кода (Pascal)
curl -X POST http://localhost:5143/api/parser/validate \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "code": "program Test; begin end.",
    "language": "pascal"
  }'

# Валидация кода (C)
curl -X POST http://localhost:5143/api/parser/validate \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "code": "int main() { return 0; }",
    "language": "c"
  }'

# Валидация кода (C++)
curl -X POST http://localhost:5143/api/parser/validate \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "code": "#include <iostream>\nint main() { return 0; }",
    "language": "cpp"
  }'

# Парсинг кода (Pascal)
curl -X POST http://localhost:5143/api/parser/parse \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "code": "program Test; var x: integer; begin x := 5; end.",
    "language": "pascal"
  }'

# Упрощенная валидация
curl -X POST http://localhost:5143/api/parser/validate/simple \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "code": "program Test; begin end.",
    "language": "pascal"
  }'
```

### Примеры кода для тестирования

**Pascal:**
```pascal
program Test;
var
  x, y: integer;
function Add(a, b: integer): integer;
begin
  Add := a + b;
end;
begin
  x := 10;
  y := 20;
  writeln(Add(x, y));
end.
```

**C:**
```c
#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 10;
    int y = 20;
    printf("%d\n", add(x, y));
    return 0;
}
```

**C++:**
```cpp
#include <iostream>

int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 10;
    int y = 20;
    std::cout << add(x, y) << std::endl;
    return 0;
}
```

### PowerShell скрипт (TEST_REQUEST.ps1)

Скрипт `TEST_REQUEST.ps1` упрощает тестирование парсера с готовым примером Pascal кода.

**Расположение:** `backend_and_parser/TEST_REQUEST.ps1`

**Использование:**

```powershell
# Из директории backend_and_parser/
# Базовое использование (использует эндпоинт parse по умолчанию)
.\TEST_REQUEST.ps1

# С указанием токена
.\TEST_REQUEST.ps1 -Token "your_jwt_token_here"

# С указанием эндпоинта
.\TEST_REQUEST.ps1 -Endpoint "validate"
.\TEST_REQUEST.ps1 -Endpoint "validate/simple"

# С указанием URL
.\TEST_REQUEST.ps1 -Url "https://localhost:7143/api/parser/parse"

# Комбинированное использование
.\TEST_REQUEST.ps1 -Token "your_token" -Endpoint "validate" -Url "http://localhost:5143/api/parser/validate"
```

**Параметры:**
- `-Token` (необязательный) — JWT токен для авторизации. По умолчанию используется тестовый токен (может быть устаревшим).
- `-Endpoint` (необязательный) — эндпоинт: `parse`, `validate` или `validate/simple`. По умолчанию: `parse`.
- `-Url` (необязательный) — полный URL эндпоинта. По умолчанию: `https://localhost:7143/api/parser/parse`.

**Что делает скрипт:**
1. Содержит готовый пример Pascal кода с различными конструкциями (функции, процедуры, циклы, условия)
2. Формирует JSON тело запроса с кодом и языком
3. Отправляет POST запрос с JWT авторизацией
4. Обрабатывает SSL сертификаты (отключает проверку для localhost)
5. Выводит результат в формате JSON

**Пример вывода (успех):**
```json
{
  "success": true,
  "representation": { ... },
  "representationType": "SPR",
  "error": null,
  "lexerErrors": [],
  "parserErrors": []
}
```

**Пример вывода (ошибка):**
```json
{
  "success": false,
  "representation": null,
  "representationType": null,
  "error": "Parsing failed",
  "lexerErrors": [ ... ],
  "parserErrors": [ ... ]
}
```

**Примечание:** Для получения актуального JWT токена используйте `/api/auth/login` через Swagger или curl.

## Тестирование через Swagger

1. Запустите парсер-сервис и бэкенд:
```bash
# Терминал 1: Парсер
cd backend_and_parser/src/parser/Parser/build
./parser-server 8080

# Терминал 2: Backend
cd backend_and_parser/src/WebApi
dotnet run --project DAOSS.WebApi.csproj
```

Или используйте скрипт автоматизации:
```powershell
.\start-all.ps1
```

2. Откройте Swagger UI: 
   - HTTP: `http://localhost:5143/swagger`
   - HTTPS: `https://localhost:7143/swagger`

3. Авторизуйтесь:
   - Используйте `/api/auth/login` для получения токена
   - Нажмите кнопку "Authorize" в правом верхнем углу Swagger UI
   - Введите: `Bearer your_token_here` (без кавычек)
   - Нажмите "Authorize" и "Close"

4. Протестируйте эндпоинты:
   - `/api/parser/validate` - валидация кода с детальными ошибками
   - `/api/parser/parse` - парсинг кода в AST/SPR
   - `/api/parser/validate/simple` - упрощенная валидация

5. В Swagger UI:
   - Выберите нужный эндпоинт
   - Нажмите "Try it out"
   - Заполните тело запроса (JSON)
   - Нажмите "Execute"
   - Просмотрите ответ и коды статуса

## Troubleshooting

### Парсер-сервис недоступен (503 Service Unavailable)

**Причина:** Парсер-сервис не запущен или недоступен на порту 8080.

**Решение:**
1. Убедитесь, что парсер запущен:
   ```bash
   cd backend_and_parser/src/parser/Parser/build
   ./parser-server 8080
   ```
2. Проверьте, что порт 8080 свободен:
   ```bash
   # Windows
   netstat -ano | findstr :8080
   
   # Linux/macOS
   lsof -i :8080
   ```
3. Проверьте логи парсера на наличие ошибок

### Таймаут запроса (504 Gateway Timeout)

**Причина:** Парсер-сервис не отвечает в течение установленного таймаута.

**Решение:**
1. Проверьте, что парсер запущен и отвечает
2. Увеличьте таймаут в настройках `ParserService` (если нужно)
3. Проверьте сложность кода — очень большие файлы могут требовать больше времени

### Неверный формат запроса (400 Bad Request)

**Причина:** Неверный формат JSON или отсутствуют обязательные поля.

**Решение:**
1. Проверьте формат JSON
2. Убедитесь, что присутствуют поля `code` и `language`
3. Проверьте, что `language` один из: `pascal`, `c`, `cpp`

### Неавторизованный доступ (401 Unauthorized)

**Причина:** Отсутствует или неверный JWT токен.

**Решение:**
1. Получите токен через `/api/auth/login`:
   ```bash
   curl -X POST http://localhost:5143/api/auth/login \
     -H "Content-Type: application/json" \
     -d '{"email":"user@example.com","password":"password"}'
   ```
2. Используйте токен в заголовке `Authorization: Bearer <token>`
3. Убедитесь, что токен не истёк

### Ошибки парсинга

Если код содержит ошибки, они будут возвращены в ответе:
- `lexerErrors` — ошибки лексера (неожиданные символы, неверные токены)
- `parserErrors` — ошибки парсера (неверный синтаксис, отсутствующие элементы)

Проверьте поля `message`, `expected`, `found` в объектах ошибок для понимания проблемы.

## Полезные ссылки

- [Swagger UI](http://localhost:5143/swagger) - интерактивная документация API (HTTP)
- [README.md](README.md) - общая документация проекта
- [Parser README.md](src/parser/Parser/README.md) - документация парсер-сервиса
- [Some explanations.md](Some%20explanations.md) - нюансы работы с бэкендом
