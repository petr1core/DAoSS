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
   - Запуск: `./parser-server 8080`
   - URL: `http://localhost:8080`

2. **WebApi (ASP.NET Core)** — порты **5143** (HTTP) и **7143** (HTTPS)
   - Запуск: `dotnet run --project src/WebApi/DAOSS.WebApi.csproj`
   - URL: `http://localhost:5143` или `https://localhost:7143`

**Важно:** Сначала запустите парсер-сервис, затем WebApi, так как WebApi делает HTTP запросы к парсер-сервису.

### Порядок запуска

```bash
# Терминал 1: Запуск парсер-сервиса
cd "DAOSS gh/src/parser/build"
./parser-server 8080

# Терминал 2: Запуск WebApi
cd "DAOSS gh"
dotnet run --project src/WebApi/DAOSS.WebApi.csproj
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
- Авторизация: Да (JWT токен)
- Content-Type: `application/json`

**Запрос:**
```json
{
  "code": "program Test; begin end.",
  "language": "pascal"
}
```

**Поддерживаемые языки:** `pascal`, `c`, `cpp`

### POST `/api/parser/validate`

Валидирует синтаксис кода с детальными ошибками.

**Требования:**
- Авторизация: Да (JWT токен)
- Content-Type: `application/json`

**Запрос:**
```json
{
  "code": "program Test; begin end.",
  "language": "pascal"
}
```

### POST `/api/parser/validate/simple`

Упрощенный эндпоинт для быстрой проверки валидности (возвращает только статус и количество ошибок).

**Требования:**
- Авторизация: Да (JWT токен)
- Content-Type: `application/json`

**Запрос:**
```json
{
  "code": "program Test; begin end.",
  "language": "pascal"
}
```

> **Примечание:** Подробные примеры ответов доступны в Swagger UI.

## Примеры запросов

### cURL

```bash
# Сначала получите JWT токен через /api/auth/login
TOKEN="your_jwt_token_here"

# Валидация кода
curl -X POST http://localhost:5143/api/parser/validate \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "code": "program Test; begin end.",
    "language": "pascal"
  }'

# Парсинг кода
curl -X POST http://localhost:5143/api/parser/parse \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "code": "program Test; var x: integer; begin x := 5; end.",
    "language": "pascal"
  }'
```

### PowerShell скрипт (TEST_REQUEST.ps1)

Скрипт `TEST_REQUEST.ps1` упрощает тестирование парсера с готовым примером Pascal кода.

**Использование:**

```powershell
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
- `-Token` (необязательный) — JWT токен для авторизации. По умолчанию используется тестовый токен.
- `-Endpoint` (необязательный) — эндпоинт: `parse`, `validate` или `validate/simple`. По умолчанию: `parse`.
- `-Url` (необязательный) — полный URL эндпоинта. По умолчанию: `https://localhost:7143/api/parser/parse`.

**Что делает скрипт:**
1. Содержит готовый пример Pascal кода с различными конструкциями (функции, процедуры, циклы, условия)
2. Формирует JSON тело запроса с кодом и языком
3. Отправляет POST запрос с JWT авторизацией
4. Выводит результат в формате JSON

**Пример вывода:**
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

## Тестирование через Swagger

1. Запустите бэкенд:
```bash
dotnet run --project src/WebApi/DAOSS.WebApi.csproj
```

2. Откройте Swagger UI: 
   - HTTP: `http://localhost:5143/swagger`
   - HTTPS: `https://localhost:7143/swagger`

3. Авторизуйтесь:
   - Используйте `/api/auth/login` для получения токена
   - Нажмите "Authorize" в Swagger UI
   - Введите: `Bearer your_token_here`

4. Протестируйте эндпоинты:
   - `/api/parser/validate` - валидация кода
   - `/api/parser/parse` - парсинг кода
   - `/api/parser/validate/simple` - упрощенная валидация

## Полезные ссылки

- [Swagger UI](http://localhost:5143/swagger) - интерактивная документация API (HTTP)
- [README.md](README.md) - общая документация проекта
