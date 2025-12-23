# Инструкция по развертыванию проекта DAoSS

Это руководство поможет вам настроить и запустить проект на вашем компьютере.

## 📋 Содержание

1. [Требования](#требования)
2. [Настройка базы данных PostgreSQL](#настройка-базы-данных-postgresql)
3. [Настройка Backend (.NET)](#настройка-backend-net)
4. [Настройка Android Client](#настройка-android-client)
5. [Запуск проекта](#запуск-проекта)
6. [Проверка работы](#проверка-работы)
7. [Устранение проблем](#устранение-проблем)

---

## 🔧 Требования

### Для Backend:
- **.NET 8 SDK** или выше
- **PostgreSQL 12+** (рекомендуется 14+)
- **Entity Framework Core Tools** (для миграций)
- **Git** для клонирования репозитория

### Для Android Client:
- **Android Studio** (Hedgehog | 2023.1.1 или новее)
- **JDK 11** или выше
- **Android SDK** (API Level 24+)
- **Эмулятор Android** или физическое устройство

---

## 🗄️ Настройка базы данных PostgreSQL

### 1. Установка PostgreSQL

**Windows:**
```powershell
# Скачайте установщик с https://www.postgresql.org/download/windows/
# Или используйте Chocolatey:
choco install postgresql
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install postgresql postgresql-contrib
```

**macOS:**
```bash
brew install postgresql
brew services start postgresql
```

### 2. Создание базы данных

```bash
# Войдите в PostgreSQL
psql -U postgres

# Создайте базу данных
CREATE DATABASE daoss_db;

# Создайте пользователя (опционально)
CREATE USER daoss_user WITH PASSWORD 'your_password';
GRANT ALL PRIVILEGES ON DATABASE daoss_db TO daoss_user;

# Выйдите из psql
\q
```

### 3. Проверка подключения

```bash
psql -U postgres -d daoss_db -h localhost
```

---

## ⚙️ Настройка Backend (.NET)

### 1. Клонирование репозитория

```bash
git clone <repository-url>
cd DAoSS
```

### 2. Установка .NET SDK

Проверьте версию:
```bash
dotnet --version
# Должно быть 8.0 или выше
```

Если не установлен, скачайте с https://dotnet.microsoft.com/download

### 3. Установка Entity Framework Core Tools

```bash
dotnet tool install --global dotnet-ef
```

Если возникла ошибка с NuGet, очистите кэш:
```bash
dotnet nuget locals all --clear
dotnet tool install --global dotnet-ef
```

Проверьте установку:
```bash
dotnet ef --version
```

### 4. Настройка конфигурации

1. Перейдите в папку backend:
```bash
cd DAoSS-backend_wip/src/WebApi
```

2. Создайте файл `appsettings.Development.json` на основе примера:
```bash
# Windows PowerShell
Copy-Item appsettings.Development.example.json appsettings.Development.json

# Linux/macOS
cp appsettings.Development.example.json appsettings.Development.json
```

3. Откройте `appsettings.Development.json` и настройте:

```json
{
  "Logging": {
    "LogLevel": {
      "Default": "Debug",
      "Microsoft.AspNetCore": "Information"
    }
  },
  "ConnectionStrings": {
    "Default": "Host=localhost;Port=5432;Database=daoss_db;Username=postgres;Password=your_password",
    "DefaultPassword": "your_postgres_password"
  },
  "Jwt": {
    "Key": "YOUR_SECRET_JWT_KEY_HERE_MINIMUM_32_CHARACTERS_LONG",
    "Issuer": "daoss-dev",
    "Audience": "daoss-client",
    "ExpiryInMinutes": 60
  }
}
```

**Важно:**
- Замените `your_password` на ваш пароль PostgreSQL
- Замените `YOUR_SECRET_JWT_KEY_HERE...` на случайную строку минимум 32 символа
- Не коммитьте этот файл в Git (он уже в .gitignore)

### 5. Восстановление зависимостей

```bash
cd DAoSS-backend_wip
dotnet restore
```

### 6. Применение миграций базы данных

```bash
# Из корня проекта DAoSS-backend_wip
dotnet ef database update --project src/WebApi/DAOSS.WebApi.csproj
```

Если возникла ошибка, убедитесь что:
- PostgreSQL запущен
- База данных создана
- Пароль в `appsettings.Development.json` правильный

### 7. Запуск Backend

```bash
cd src/WebApi
dotnet run
```

Или из корня проекта:
```bash
dotnet run --project src/WebApi/DAOSS.WebApi.csproj
```

Сервер должен запуститься на:
- **HTTP**: `http://localhost:5143`
- **HTTPS**: `https://localhost:7143`

Проверьте в браузере: `http://localhost:5143` - должно появиться сообщение "Бэкенд сервиса запущен"

---

## 📱 Настройка Android Client

### 1. Установка Android Studio

1. Скачайте Android Studio с https://developer.android.com/studio
2. Установите с настройками по умолчанию
3. При первом запуске установите Android SDK (API Level 24+)

### 2. Открытие проекта

1. Запустите Android Studio
2. Выберите **File → Open**
3. Выберите папку `FlowchartEditorClient`
4. Дождитесь синхронизации Gradle (может занять несколько минут)

### 3. Настройка API URL

Откройте файл `app/build.gradle` и найдите строку:

```gradle
buildConfigField "String", "BASE_URL", "\"http://10.0.2.2:5143/\""
```

**Для Android Emulator** (рекомендуется для разработки):
- Оставьте как есть: `http://10.0.2.2:5143/`
- `10.0.2.2` - это специальный IP для обращения к localhost компьютера

**Для реального Android устройства:**
1. Узнайте IP адрес вашего компьютера:
   - **Windows**: `ipconfig` (ищите IPv4 адрес)
   - **Linux/Mac**: `ifconfig` или `ip addr`
2. Измените на: `http://192.168.x.x:5143/` (замените на ваш IP)
3. Убедитесь, что телефон и компьютер в одной Wi-Fi сети

### 4. Сборка проекта

1. В Android Studio выберите **Build → Make Project**
2. Дождитесь завершения сборки

---

## 🚀 Запуск проекта

### Порядок запуска:

1. **Запустите PostgreSQL** (если не запущен автоматически)
   ```bash
   # Windows
   net start postgresql-x64-14  # или другая версия
   
   # Linux
   sudo systemctl start postgresql
   
   # macOS
   brew services start postgresql
   ```

2. **Запустите Backend**
   ```bash
   cd DAoSS-backend_wip/src/WebApi
   dotnet run
   ```
   
   Дождитесь сообщения: `Now listening on: http://localhost:5143`

3. **Запустите Android приложение**
   - В Android Studio нажмите **Run** (зеленая кнопка ▶️)
   - Выберите эмулятор или подключенное устройство
   - Приложение установится и запустится

---

## ✅ Проверка работы

### 1. Проверка Backend

Откройте в браузере:
- `http://localhost:5143` - должно показать "Бэкенд сервиса запущен"
- `http://localhost:5143/swagger` - Swagger UI (в режиме Development)

### 2. Проверка Android приложения

1. **Регистрация:**
   - Откройте приложение
   - Нажмите "Создать аккаунт"
   - Заполните форму и зарегистрируйтесь

2. **Вход:**
   - Введите логин и пароль
   - Нажмите "Войти"

3. **Создание проекта:**
   - Нажмите кнопку "Создать проект"
   - Заполните название и описание
   - Проект должен появиться в списке

4. **Добавление участников:**
   - Откройте проект
   - Нажмите "Добавить участника"
   - Введите User ID и выберите роль

---

## 🔍 Устранение проблем

### Backend не запускается

**Ошибка: "Connection refused" или проблемы с БД:**
```bash
# Проверьте, запущен ли PostgreSQL
# Windows
sc query postgresql-x64-14

# Linux
sudo systemctl status postgresql

# Проверьте подключение
psql -U postgres -d daoss_db -h localhost
```

**Ошибка миграций:**
```bash
# Удалите и пересоздайте базу данных
psql -U postgres
DROP DATABASE daoss_db;
CREATE DATABASE daoss_db;
\q

# Примените миграции заново
dotnet ef database update --project src/WebApi/DAOSS.WebApi.csproj
```

**Порт 5143 занят:**
- Измените порт в `appsettings.json` или `launchSettings.json`
- Или остановите процесс, использующий порт

### Android приложение не подключается к серверу

**"Network error" или "Failed to connect":**
1. Проверьте, что Backend запущен (`http://localhost:5143`)
2. Для эмулятора используйте `10.0.2.2:5143`
3. Для реального устройства:
   - Проверьте IP адрес компьютера
   - Убедитесь, что устройство в той же Wi-Fi сети
   - Проверьте файрвол Windows/антивирус

**"401 Unauthorized":**
- Токен истек или неверный
- Выйдите и войдите заново

**Приложение не собирается:**
```bash
# Очистите кэш Gradle
cd FlowchartEditorClient
./gradlew clean

# В Android Studio: File → Invalidate Caches → Invalidate and Restart
```

### База данных

**Миграции не применяются:**
```bash
# Проверьте строку подключения в appsettings.Development.json
# Убедитесь, что база данных существует
# Проверьте права пользователя PostgreSQL
```

**Ошибка "column does not exist":**
```bash
# Примените миграции заново
dotnet ef database update --project src/WebApi/DAOSS.WebApi.csproj --verbose
```

---

## 📝 Дополнительные настройки

### Переменные окружения (альтернатива appsettings)

Можно использовать переменные окружения вместо файла конфигурации:

**Windows PowerShell:**
```powershell
$env:ConnectionStrings__Default="Host=localhost;Port=5432;Database=daoss_db;Username=postgres;Password=your_password"
$env:Jwt__Key="your_jwt_key_here"
dotnet run --project src/WebApi/DAOSS.WebApi.csproj
```

**Linux/macOS:**
```bash
export ConnectionStrings__Default="Host=localhost;Port=5432;Database=daoss_db;Username=postgres;Password=your_password"
export Jwt__Key="your_jwt_key_here"
dotnet run --project src/WebApi/DAOSS.WebApi.csproj
```

### Использование Docker для PostgreSQL (опционально)

```bash
# Запуск PostgreSQL в Docker
docker run --name daoss-postgres \
  -e POSTGRES_PASSWORD=your_password \
  -e POSTGRES_DB=daoss_db \
  -p 5432:5432 \
  -d postgres:14

# Остановка
docker stop daoss-postgres

# Удаление
docker rm daoss-postgres
```

---

## 🔐 Безопасность

⚠️ **Важно:**
- Никогда не коммитьте `appsettings.Development.json` с реальными паролями
- Используйте сильные пароли для JWT ключей (минимум 32 символа)
- В продакшене используйте переменные окружения или секреты
- Регулярно обновляйте зависимости

---

## 📚 Полезные команды

### Backend

```bash
# Создать новую миграцию
dotnet ef migrations add MigrationName --project src/WebApi/DAOSS.WebApi.csproj

# Откатить последнюю миграцию
dotnet ef database update PreviousMigrationName --project src/WebApi/DAOSS.WebApi.csproj

# Удалить последнюю миграцию (если не применена)
dotnet ef migrations remove --project src/WebApi/DAOSS.WebApi.csproj

# Просмотр SQL миграции
dotnet ef migrations script --project src/WebApi/DAOSS.WebApi.csproj
```

### Android

```bash
# Очистить сборку
cd FlowchartEditorClient
./gradlew clean

# Собрать APK
./gradlew assembleDebug

# Установить на устройство
./gradlew installDebug
```

---

## 🆘 Получение помощи

Если возникли проблемы:
1. Проверьте логи Backend в консоли
2. Проверьте логи Android в Logcat (Android Studio)
3. Убедитесь, что все зависимости установлены
4. Проверьте версии .NET SDK и Android SDK

---

**Удачи в разработке! 🚀**

