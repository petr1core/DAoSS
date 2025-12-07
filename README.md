# DAoSS
Проектирование и архитектура программных систем

Backend — .NET 8, 3-tier архитектура
====================================

## Структура каталогов

```
DAOSS gh/
└─ src/
   ├─ WebApi/                 // Веб-API (контроллеры, Program.cs)
   │  ├─ Domain/              // Доменная модель (сущности)
   │  │  └─ Entities/         // BaseEntity, User, Project, Diagram, Review, ...
   │  ├─ Application/         // Контракты (интерфейсы сервисов), DTO
   │  │  ├─ Interfaces/       // IAuthService, IParserService, IReviewService, ...
   │  │  ├─ DTOs/             // ParserRequest, ParserResponse, ValidationResponse, ...
   │  │  └─ Repositories/     // Интерфейсы репозиториев
   │  ├─ Infrastructure/      // Доступ к данным, EF Core, внешние интеграции
   │  │  ├─ Persistence/      // AppDbContext
   │  │  ├─ Services/         // ParserResponseMapper и др.
   │  │  └─ Migrations/       // Миграции EF Core
   │  ├─ Controllers/         // API контроллеры
   │  ├─ Services/            // Реализации сервисов (AuthService, ParserService, ...)
   │  └─ Repositories/        // Реализации репозиториев
   └─ parser/                 // Парсер-сервис (C++)
      └─ Parser/backend/parser/
         ├─ main.cpp          // HTTP сервер парсера (порт 8080), эндпоинты /api/parse, /api/validate ....
         ├─ Ast/              // AST структуры для C/C++ (Expr, Stmt, Visitor)
         ├─ Auth/             // JWT валидация (JwtValidator)
         ├─ CodeGen/          // Генераторы кода (CCodeGenerator, CppCodeGenerator, PascalCodeGenerator)
         ├─ CPPAst/           // AST структуры для C++ (CppAst, CppDecl, CppExpr, CppStmt)
         ├─ Expression/       // Выражения для Pascal (Function, Procedure, StatementExpression)
         ├─ Flowchart/        // Экспорт в JSON для блок-схем (ExporterJson, FromExpressions)
         ├─ Parser/           // Парсеры для языков (PascalParserToExpression, CParserToAST, CppParserToAST, ErrorCollector)
         ├─ Scripts/          // Вспомогательные компоненты (Lexer, Token, Stack, SearchTreeTable)
         ├─ thirdparty/       // Внешние библиотеки (httplib.h, jwt-cpp.hpp)
         ├─ json-3.12.0/      // Библиотека nlohmann/json для работы с JSON
         ├─ build/            // Сборочные файлы CMake и скомпилированные бинарники
         └─ CMakeLists.txt    // Конфигурация сборки CMake
```

**Подробнее о каждом слое:**
- `Domain/README.md` — доменные сущности
- `Application/README.md` — интерфейсы и DTOs
- `Infrastructure/README.md` — доступ к данным и внешние интеграции
- `WebApi/README.md` — контроллеры и точка входа

## Сборка и запуск

### Бэкенд (WebApi)

Из корня `DAOSS gh`:

```bash
dotnet build DAOSS.sln
dotnet run --project src/WebApi/DAOSS.WebApi.csproj
```

WebApi запускается на портах:
- HTTP: `http://localhost:5143`
- HTTPS: `https://localhost:7143`

### Парсер-сервис (C++)

Парсер-сервис должен быть запущен отдельно на порту **8080**:

```bash
cd src/parser/build
./parser-server 8080
```

**Важно:** Сначала запустите парсер-сервис, затем WebApi, так как WebApi делает HTTP запросы к парсер-сервису.

Подробнее о тестировании парсера см. [PARSER_API_TESTING.md](PARSER_API_TESTING.md)

## Архитектура слоёв

- **Domain** — не зависит от других слоёв. Содержит сущности предметной области.
- **Application** — зависит от Domain. Определяет интерфейсы сервисов и репозиториев.
- **Infrastructure** — зависит от Domain и Application. Реализует доступ к данным и бизнес-логику.
- **WebApi** — зависит от Application и Infrastructure. Предоставляет HTTP API.

## Функционал бэкенда

### Аутентификация и авторизация

- **Регистрация и вход** (`/api/auth/register`, `/api/auth/login`)
  - Регистрация пользователей с email/login и паролем
  - Аутентификация через JWT токены
  - Хеширование паролей с использованием BCrypt
  - Получение информации о текущем пользователе (`/api/auth/me`)

- **Авторизация на основе ролей**
  - Политики доступа: `ProjectRead`, `ProjectWrite`, `ProjectAdmin`
  - Роли в проектах: `owner`, `admin`, `reviewer`
  - Проверка прав доступа через `ProjectRoleHandler`

### Управление проектами

- **CRUD операции** (`/api/projects`)
  - Создание, чтение, обновление и удаление проектов
  - Поддержка видимости проектов (private/public)
  - Настройка правил ревью через `RequiredReviewersRules` (JSON)
  - Автоматическое добавление владельца в участники при создании

### Управление участниками проекта

- **Управление участниками** (`/api/projects/{projectId}/members`)
  - Добавление участников с назначением роли (owner/admin/reviewer)
  - Получение списка всех участников проекта
  - Изменение роли участника
  - Передача владения проектом (owner может передать права другому участнику)
  - Исключение участников из проекта
  - Защита от удаления владельца проекта

- **Система ролей**
  - **Owner** — полный контроль над проектом, может передавать владение
  - **Admin** — управление участниками и настройками проекта
  - **Reviewer** — просмотр и создание ревью

### Система приглашений

- **Приглашения в проекты** (`/api/projects/{projectId}/invitations`)
  - Отправка приглашений пользователям (только admin/owner)
  - Просмотр приглашений проекта и пользователя
  - Принятие/отклонение приглашений
  - Автоматическое добавление в участники при принятии
  - Отмена приглашений
  - Срок действия приглашений (7 дней)

### Система ревью

- **Управление ревью** (`/api/projects/{projectId}/reviews`)
  - Создание ревью для версий диаграмм или исходных файлов
  - Просмотр списка ревью проекта
  - Изменение статуса ревью (open, approved, changes_requested)
  - Удаление ревью
  - Поддержка типов целей: `diagram_version`, `source_file_version`

- **Элементы ревью** (`/api/projects/{projectId}/reviews/{reviewId}/items`)
  - Создание элементов ревью (comment/issue)
  - Привязка к коду или диаграмме через anchor
  - Управление статусом элементов (open/resolved)
  - Обновление и удаление элементов

- **Комментарии** (`/api/projects/{projectId}/reviews/{reviewId}/items/{itemId}/comments`)
  - Добавление комментариев к элементам ревью
  - Просмотр всех комментариев элемента
  - Редактирование и удаление комментариев
  - Отслеживание автора комментария

### Парсинг кода

- **Парсинг кода** (`/api/parser/parse`)
  - Парсинг исходного кода в AST/SPR представление
  - Поддержка языков: Pascal (SPR), C (AST), C++ (AST)
  - Возвращает структурированное представление программы
  - Обработка ошибок парсинга (lexer и parser ошибки)

- **Валидация кода** (`/api/parser/validate`)
  - Проверка синтаксиса кода с детальными ошибками
  - Возвращает список ошибок лексера и парсера
  - Поддержка всех языков: pascal, c, cpp

- **Упрощенная валидация** (`/api/parser/validate/simple`)
  - Быстрая проверка валидности кода
  - Возвращает только статус и количество ошибок

**Архитектура парсера:**
- Парсер-сервис (C++) работает как отдельный HTTP сервер на порту 8080
- WebApi выступает прокси между клиентом и парсер-сервисом
- Единая система авторизации через JWT токены
- Обработка таймаутов и недоступности сервиса

### Технические детали

- **База данных**: PostgreSQL через Entity Framework Core
- **Миграции**: Поддержка версионирования схемы БД
- **Unit of Work**: Атомарное сохранение изменений
- **Swagger**: Автоматическая документация API (в режиме Development)
- **Валидация**: Проверка входных данных через Data Annotations
- **Обработка ошибок**: Стандартизированные ответы с сообщениями об ошибках
- **Парсер-сервис**: C++ HTTP сервер с JWT авторизацией, поддержка CORS
- **Интеграция парсера**: HTTP клиент с настраиваемым таймаутом, маппинг ответов

## Примечания

- Основные сервисы и репозитории реализованы и зарегистрированы в DI контейнере
- `IParserService` реализован и интегрирован с C++ парсер-сервисом
- Интерфейсы `IDiagramService`, `ISyncService` определены, но реализации ожидаются в будущем
- Все эндпоинты защищены авторизацией и проверкой прав доступа
- Реализации репозиториев и сервисов физически находятся в WebApi, но логически относятся к Infrastructure слою

## Документация

Каждый слой проекта содержит README с подробным описанием:
- `src/WebApi/Domain/README.md` — доменные сущности
- `src/WebApi/Application/README.md` — интерфейсы и DTOs
- `src/WebApi/Infrastructure/README.md` — доступ к данным и интеграции
- `src/WebApi/README.md` — контроллеры и точка входа
- `PARSER_API_TESTING.md` — документация по тестированию парсера