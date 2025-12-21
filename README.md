# DAOSS - Проектирование и архитектура программных систем

Система для совместной работы над проектами с поддержкой парсинга кода, создания диаграмм и системы ревью.

## Описание проекта

DAOSS (Design and Architecture of Software Systems) — это комплексная платформа для управления проектами разработки программного обеспечения, включающая:

- **Парсинг кода** — поддержка языков Pascal, C, C++ с генерацией AST/SPR
- **Визуализация** — создание и редактирование блок-схем
- **Система ревью** — код-ревью с комментариями и issues
- **Управление проектами** — роли, приглашения, версионирование

## Структура проекта

Проект организован как монорепозиторий с использованием Git подмодулей:

```
DAoSS/
├── frontend/                    # Frontend (React + TypeScript + Vite)
│   ├── src/                    # Исходный код
│   ├── package.json            # Зависимости Node.js
│   └── vite.config.ts          # Конфигурация Vite
│
├── backend_and_parser/         # Backend и Parser (подмодуль)
│   ├── src/
│   │   ├── WebApi/            # ASP.NET Core Web API
│   │   └── parser/
│   │       └── Parser/        # C++ парсер-сервис (подмодуль)
│   └── README.md              # Документация Backend
│
├── setup-submodules.ps1       # Скрипт настройки подмодулей
└── start-all.ps1              # Скрипт запуска всех модулей
```

## Модули проекта

### Frontend
- **Технологии:** React 19, TypeScript, Vite
- **Порт:** 5173 (dev), 4173 (preview)
- **Описание:** Пользовательский интерфейс для работы с проектами, диаграммами и ревью

### Backend
- **Технологии:** ASP.NET Core 8, Entity Framework Core, PostgreSQL
- **Порты:** HTTP 5143, HTTPS 7143
- **Архитектура:** 3-tier (Domain, Application, Infrastructure)
- **Описание:** RESTful API для управления проектами, пользователями, ревью и интеграции с парсером

### Parser
- **Технологии:** C++17, CMake, cpp-httplib, nlohmann/json
- **Порт:** 8080
- **Описание:** HTTP сервер для парсинга кода на языках Pascal, C, C++ с генерацией AST/SPR и блок-схем

## Требования к окружению

### Обязательные
- **Git** — для работы с репозиторием и подмодулями
- **Node.js** (v18+) и **npm** — для Frontend
- **.NET 8 SDK** — для Backend
- **PostgreSQL** — база данных для Backend

### Для сборки Parser
- **CMake** (3.15+)
- **C++ компилятор** с поддержкой C++17:
  - Windows: MSVC (Visual Studio 2019+) или MinGW
  - Linux: GCC 7+ или Clang 5+
  - macOS: Xcode Command Line Tools

### PowerShell (для скриптов)
- Windows PowerShell 5.1+ или PowerShell 7+

## Быстрый старт

### 1. Клонирование репозитория

```bash
# Клонировать с подмодулями
git clone --recurse-submodules https://github.com/petr1core/DAoSS
cd DAoSS
```

Если репозиторий уже склонирован без подмодулей:

```bash
git submodule update --init --recursive
```

### 2. Настройка подмодулей

Используйте скрипт для автоматической настройки подмодулей и переключения на нужные ветки:

```powershell
# Базовое использование
.\setup-submodules.ps1

# С автоматическим добавлением Git в PATH (если нужно)
.\setup-submodules.ps1 -AddGitToPath

# С подробным выводом
.\setup-submodules.ps1 -Verbose
```

Скрипт автоматически:
- Инициализирует все подмодули (включая вложенные)
- Переключает `backend_and_parser` на ветку `backend_and_parser`
- Переключает `Parser` на ветку `http-server_wip`

### 3. Настройка базы данных

Настройте строку подключения к PostgreSQL в `backend_and_parser/src/WebApi/appsettings.Development.json`:

```json
{
  "ConnectionStrings": {
    "DefaultConnection": "Host=localhost;Port=5432;Database=daoss;Username=postgres;Password=your_password"
  }
}
```

Или используйте переменные окружения:
- `POSTGRES_HOST`
- `POSTGRES_PORT`
- `POSTGRES_DB`
- `POSTGRES_USER`
- `POSTGRES_PASSWORD`

### 4. Установка зависимостей Frontend

```bash
npm install
```

### 5. Запуск проекта

Используйте скрипт для запуска всех модулей:

```powershell
# Базовый запуск
.\start-all.ps1

# С полной подготовкой (сборка + миграции)
.\start-all.ps1 -BuildParser -BuildBackend -UpdateMigrations

# Только сборка парсера
.\start-all.ps1 -BuildParser

# Только миграции базы данных
.\start-all.ps1 -UpdateMigrations

# С подробным выводом
.\start-all.ps1 -Verbose
```

**Порядок запуска модулей:**
1. Parser (порт 8080)
2. Backend (порты 5143/7143)
3. Frontend (порт 5173)

### Альтернативный запуск вручную

#### Запуск Parser
```bash
cd backend_and_parser/src/parser/Parser
mkdir build && cd build
cmake ..
cmake --build . --config Release
./parser-server  # или parser-server.exe на Windows
```

#### Запуск Backend
```bash
cd backend_and_parser/src/WebApi
dotnet restore
dotnet ef database update  # Применить миграции
dotnet run
```

#### Запуск Frontend
```bash
npm run dev
```

## Скрипты автоматизации

### setup-submodules.ps1

Скрипт для настройки Git подмодулей.

**Параметры:**
- `-Verbose` — подробный вывод
- `-AddGitToPath` — автоматически добавить Git в PATH

**Использование:**
```powershell
.\setup-submodules.ps1 -AddGitToPath
```

### start-all.ps1

Скрипт для последовательного запуска всех модулей проекта.

**Параметры:**
- `-Verbose` — подробный вывод
- `-AddGitToPath` — автоматически добавить Git в PATH
- `-BuildParser` — собрать Parser перед запуском
- `-BuildBackend` — собрать Backend перед запуском
- `-UpdateMigrations` — применить миграции базы данных

**Использование:**
```powershell
# Полный запуск с подготовкой
.\start-all.ps1 -BuildParser -BuildBackend -UpdateMigrations -Verbose
```

## API Эндпоинты

### Базовый URL
- **Backend API**: `http://localhost:5143/api` (HTTP) или `https://localhost:7143/api` (HTTPS)
- **Parser Service**: `http://localhost:8080/api` (внутренний, используется через Backend)

### Аутентификация

Все эндпоинты (кроме `/api/auth/login` и `/api/auth/register`) требуют JWT токен в заголовке:
```
Authorization: Bearer <token>
```

#### Аутентификация и пользователи
- `POST /api/auth/register` — регистрация нового пользователя
- `POST /api/auth/login` — вход в систему
- `GET /api/auth/me` — получение информации о текущем пользователе
- `GET /api/auth/validate` — проверка валидности токена

### Проекты

- `GET /api/projects?ownerId={guid}` — получение списка проектов пользователя
- `GET /api/projects/{id}` — получение проекта по ID
- `POST /api/projects` — создание нового проекта
- `PUT /api/projects/{id}` — обновление проекта
- `DELETE /api/projects/{id}` — удаление проекта

### Участники проекта

- `GET /api/projects/{projectId}/members` — получение списка участников проекта
- `POST /api/projects/{projectId}/members` — добавление участника в проект
- `PUT /api/projects/{projectId}/members/{memberId}` — изменение роли участника
- `DELETE /api/projects/{projectId}/members/{memberId}` — удаление участника из проекта

### Приглашения

- `POST /api/projects/{projectId}/invitations` — отправка приглашения в проект
- `GET /api/projects/{projectId}/invitations` — получение приглашений проекта
- `GET /api/invitations` — получение приглашений текущего пользователя
- `POST /api/invitations/{invitationId}/accept` — принятие приглашения
- `POST /api/invitations/{invitationId}/reject` — отклонение приглашения
- `DELETE /api/projects/{projectId}/invitations/{invitationId}` — отмена приглашения

### Исходные файлы

- `POST /api/projects/{projectId}/source-files` — загрузка нового исходного файла
- `GET /api/projects/{projectId}/source-files` — получение списка исходных файлов проекта
- `GET /api/projects/{projectId}/source-files/{fileId}` — получение информации о файле
- `GET /api/projects/{projectId}/source-files/{fileId}/versions` — получение всех версий файла
- `GET /api/projects/{projectId}/source-files/{fileId}/versions/{versionId}` — получение конкретной версии
- `POST /api/projects/{projectId}/source-files/{fileId}/versions` — создание новой версии файла
- `DELETE /api/projects/{projectId}/source-files/{fileId}` — удаление файла

### Диаграммы

- `POST /api/projects/{projectId}/diagrams/generate` — генерация диаграммы из кода
- `GET /api/projects/{projectId}/diagrams` — получение списка диаграмм проекта
- `GET /api/projects/{projectId}/diagrams/{diagramId}` — получение информации о диаграмме
- `GET /api/projects/{projectId}/diagrams/{diagramId}/versions` — получение всех версий диаграммы
- `POST /api/projects/{projectId}/diagrams/{diagramId}/versions` — создание новой версии диаграммы
- `PUT /api/projects/{projectId}/diagrams/{diagramId}` — обновление диаграммы
- `DELETE /api/projects/{projectId}/diagrams/{diagramId}` — удаление диаграммы

### Ревью

- `POST /api/projects/{projectId}/reviews` — создание ревью
- `GET /api/projects/{projectId}/reviews` — получение списка ревью проекта
- `GET /api/projects/{projectId}/reviews/{reviewId}` — получение ревью по ID
- `PUT /api/projects/{projectId}/reviews/{reviewId}` — обновление статуса ревью
- `DELETE /api/projects/{projectId}/reviews/{reviewId}` — удаление ревью

#### Элементы ревью

- `POST /api/projects/{projectId}/reviews/{reviewId}/items` — создание элемента ревью
- `GET /api/projects/{projectId}/reviews/{reviewId}/items` — получение элементов ревью
- `GET /api/projects/{projectId}/reviews/{reviewId}/items/{itemId}` — получение элемента ревью
- `PUT /api/projects/{projectId}/reviews/{reviewId}/items/{itemId}` — обновление элемента ревью
- `DELETE /api/projects/{projectId}/reviews/{reviewId}/items/{itemId}` — удаление элемента ревью

#### Комментарии

- `POST /api/projects/{projectId}/reviews/{reviewId}/items/{itemId}/comments` — добавление комментария
- `GET /api/projects/{projectId}/reviews/{reviewId}/items/{itemId}/comments` — получение комментариев элемента

### Парсинг кода

- `POST /api/parser/parse` — парсинг кода в AST/SPR представление
- `POST /api/parser/validate` — валидация синтаксиса кода с детальными ошибками
- `POST /api/parser/validate/simple` — упрощенная валидация кода

**Поддерживаемые языки:** `pascal`, `c`, `cpp`

### Swagger документация

В режиме Development доступна автоматическая документация API:
- **Swagger UI**: `http://localhost:5143/swagger` (HTTP) или `https://localhost:7143/swagger` (HTTPS)

## Адреса страниц Frontend

### Базовый URL
- **Development**: `http://localhost:5173`
- **Preview**: `http://localhost:4173`

### Публичные страницы

- `/` — главная страница (редирект на `/projects` для авторизованных)
- `/login` — страница входа в систему
- `/register` — страница регистрации (редирект на `/login?mode=register`)

### Защищенные страницы

Требуют авторизации (JWT токен):

- `/projects` — список проектов пользователя
- `/projects/new` — создание нового проекта
- `/projects/:id` — детальная информация о проекте
  - Вкладки: Обзор, Участники, Приглашения, Ревью
- `/invitations` — список приглашений текущего пользователя
- `/editor` — редактор блок-схем

### Навигация

После авторизации доступны следующие разделы:
- **Проекты** — управление проектами
- **Приглашения** — просмотр и управление приглашениями

## Документация

### Основная документация
- [Backend README](backend_and_parser/README.md) — подробная документация Backend
- [Parser README](backend_and_parser/src/parser/Parser/README.md) — документация парсер-сервиса
- [Some explanations](backend_and_parser/Some%20explanations.md) — нюансы работы с бэкендом

### Дополнительные материалы
- [PARSER_API_TESTING.md](backend_and_parser/PARSER_API_TESTING.md) — тестирование API парсера
- [Architecture.md](../Architecture.md) — архитектура проекта (если есть)

## Разработка

### Структура Frontend

```
src/
├── components/          # React компоненты
│   ├── FlowchartEditor.tsx
│   ├── LoginPage.tsx
│   └── UserPanel.tsx
├── services/           # API клиенты
│   └── api.ts
├── utils/              # Утилиты
│   └── auth.ts
└── main.tsx           # Точка входа
```

### Структура Backend

См. [Backend README](backend_and_parser/README.md) для подробной информации о структуре.

### Структура Parser

См. [Parser README](backend_and_parser/src/parser/Parser/README.md) для подробной информации.

## Troubleshooting

### Проблемы с подмодулями

Если подмодули не инициализированы:
```bash
git submodule update --init --recursive
cd backend_and_parser
git submodule update --init --recursive
```

### Проблемы с портами

Убедитесь, что порты 5173, 5143, 7143, 8080 свободны:
- Windows: `netstat -ano | findstr :8080`
- Linux/macOS: `lsof -i :8080`

### Проблемы с базой данных

Проверьте строку подключения и убедитесь, что PostgreSQL запущен:
```bash
# Windows
pg_ctl status

# Linux
sudo systemctl status postgresql

# macOS
brew services list | grep postgresql
```

### Проблемы со сборкой Parser

Убедитесь, что установлены CMake и C++ компилятор:
```bash
cmake --version
g++ --version  # или cl.exe на Windows
```
