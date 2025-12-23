6. Требования к инсталляции и деинсталляции

6.1. Требования к серверной части

Системные требования к серверу:
• Операционная система: 
  - Windows 10/11 (для разработки)
  - Linux (Ubuntu 22.04 LTS, Debian 11 или выше) — для production
  - macOS (для разработки)
• Процессор: Минимум 2 ядра (рекомендуется 4+)
• Оперативная память: Минимум 4 ГБ (рекомендуется 8 ГБ и более)
• Дисковое пространство: Минимум 10 ГБ свободного места (рекомендуется SSD)
• Сетевое подключение: Стабильное интернет-соединение со скоростью минимум 10 Мбит/с
• Порты: Открытые порты 5173 (Frontend), 5143/7143 (Backend HTTP/HTTPS), 8080 (Parser), 5432 (PostgreSQL, если внешний доступ необходим)
• Примечание: Android-клиент подключается к Backend через порт 5143 (HTTP) или 7143 (HTTPS)

Программное обеспечение на сервере:

Обязательные компоненты:
• Git (2.30+) — для работы с репозиторием и подмодулями
• Node.js (v18+) и npm — для Frontend
• .NET 8 SDK — для Backend
• PostgreSQL 14+ — база данных для Backend
• PowerShell 5.1+ или PowerShell 7+ (для Windows) — для выполнения скриптов автоматизации

Для сборки Parser:
• CMake (3.15+)
• C++ компилятор с поддержкой C++17:
  - Windows: MSVC (Visual Studio 2019+) или MinGW
  - Linux: GCC 7+ или Clang 5+
  - macOS: Xcode Command Line Tools

Дополнительные инструменты (опционально):
• Entity Framework Core Tools (dotnet ef) — для управления миграциями базы данных
• Docker и Docker Compose (для контейнеризированного развёртывания, опционально)

Процесс установки серверной части:

1. Подготовка сервера:
   • Установка операционной системы
   • Обновление системы:
     - Windows: через Windows Update
     - Linux: `sudo apt update && sudo apt upgrade` (для Debian/Ubuntu)
   • Настройка файрвола (открытие необходимых портов)

2. Установка зависимостей:

   Windows:
   • Установка Git: скачать с https://git-scm.com/download/win
   • Установка Node.js: скачать с https://nodejs.org/ (LTS версия)
   • Установка .NET 8 SDK: скачать с https://dotnet.microsoft.com/download
   • Установка PostgreSQL: скачать с https://www.postgresql.org/download/windows/
   • Установка CMake: скачать с https://cmake.org/download/ или через `winget install Kitware.CMake`
   • Установка Visual Studio Build Tools (для компиляции Parser) или MinGW

   Linux (Ubuntu/Debian):
   # Обновление пакетов
   sudo apt update && sudo apt upgrade -y
   
   # Установка Git
   sudo apt install git -y
   
   # Установка Node.js (через NodeSource)
   curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
   sudo apt install -y nodejs
   
   # Установка .NET 8 SDK
   wget https://dot.net/v1/dotnet-install.sh
   chmod +x dotnet-install.sh
   ./dotnet-install.sh --channel 8.0
   export PATH=$PATH:$HOME/.dotnet
   
   # Установка PostgreSQL
   sudo apt install postgresql postgresql-contrib -y
   
   # Установка CMake и компилятора
   sudo apt install cmake build-essential -y
   3. Клонирование репозитория:ash
   # Клонировать с подмодулями
   git clone --recurse-submodules https://github.com/petr1core/DAoSS.git
   cd DAoSS
      Если репозиторий уже склонирован без подмодулей:
   
   git submodule update --init --recursive
   4. Настройка подмодулей:wershell
   # Windows (PowerShell)
   .\setup-submodules.ps1 -AddGitToPath
   
   # Или с подробным выводом
   .\setup-submodules.ps1 -AddGitToPath -Verbose
      Скрипт автоматически:
   - Инициализирует все подмодули (включая вложенные)
   - Переключает `backend_and_parser` на ветку `backend_and_parser`
   - Переключает `Parser` на ветку `http-server_wip`

5. Настройка базы данных:
   • Создание базы данных PostgreSQL:
     CREATE DATABASE daoss;
     CREATE USER daoss_user WITH PASSWORD 'your_password';
     GRANT ALL PRIVILEGES ON DATABASE daoss TO daoss_user;
        • Настройка строки подключения в `backend_and_parser/src/WebApi/appsettings.Development.json`:n
     {
       "ConnectionStrings": {
         "Default": "Host=localhost;Port=5432;Database=daoss;Username=daoss_user;Password=your_password"
       }
     }
        Или использование переменных окружения:
   - `POSTGRES_HOST` (по умолчанию: localhost)
   - `POSTGRES_PORT` (по умолчанию: 5432)
   - `POSTGRES_DB` (по умолчанию: postgres)
   - `POSTGRES_USER` (по умолчанию: postgres)
   - `POSTGRES_PASSWORD` (по умолчанию: из appsettings)

6. Установка зависимостей Frontend:
  
   npm install
   7. Установка Entity Framework Core Tools (для миграций):
   dotnet tool install --global dotnet-ef
   8. Применение миграций базы данных:h
   cd backend_and_parser/src/WebApi
   dotnet ef database update --startup-project DAOSS.WebApi.csproj --project ../Infrastructure/DAOSS.Infrastructure.csproj
      Или автоматически при первом запуске (в Development режиме миграции применяются автоматически).

9. Сборка компонентов (опционально, можно выполнить при запуске):
   • Сборка Parser:
     cd backend_and_parser/src/parser/Parser/backend/parser
     mkdir build && cd build
     cmake ..
     cmake --build . --config Release
        • Сборка Backend:
     cd backend_and_parser/src/WebApi
     dotnet restore
     dotnet build --configuration Release
     10. Запуск проекта:
    Использование скрипта автоматизации (рекомендуется):
    
    # Полный запуск с подготовкой
    .\start-all.ps1 -BuildParser -BuildBackend -UpdateMigrations -Verbose
        Скрипт автоматически:
    - Соберёт Parser (если указан флаг `-BuildParser`)
    - Соберёт Backend (если указан флаг `-BuildBackend`)
    - Применит миграции (если указан флаг `-UpdateMigrations`)
    - Запустит все модули в правильном порядке: Parser → Backend → Frontend

6.2. Процесс деинсталляции серверной части

1. Остановка сервисов:
   • Остановка всех модулей (если запущены через `start-all.ps1`):
     - Нажать `Ctrl+C` в терминале, где запущен скрипт
     - Скрипт автоматически остановит все процессы
   
   • Ручная остановка (если запущены отдельно):
     - Остановка Frontend: закрыть терминал или `Ctrl+C`
     - Остановка Backend: закрыть терминал или `Ctrl+C`
     - Остановка Parser: закрыть терминал или `Ctrl+C`

2. Резервное копирование данных (если требуется сохранение):
   • Создание финальной резервной копии базы данных:
    
     pg_dump -U daoss_user -d daoss > daoss_backup_$(date +%Y%m%d).sql
        • Экспорт конфигурационных файлов:
     - `backend_and_parser/src/WebApi/appsettings.Development.json`
     - `backend_and_parser/src/WebApi/appsettings.json`

3. Удаление приложений:
   • Удаление директории проекта:
     
     cd ..
     rm -rf DAoSS  # Linux/macOS
     # или
     Remove-Item -Recurse -Force DAoSS  # Windows PowerShell
        • Удаление собранных артефактов:
     - `backend_and_parser/src/parser/Parser/backend/parser/build/` (собранный Parser)
     - `backend_and_parser/src/WebApi/bin/` и `obj/` (собранный Backend)
     - `node_modules/` (зависимости Frontend, можно восстановить через `npm install`)

4. Удаление базы данных (опционально):
   DROP DATABASE daoss;
   DROP USER daoss_user;
      
   Или через командную строку:
  
   psql -U postgres -c "DROP DATABASE daoss;"
   psql -U postgres -c "DROP USER daoss_user;"
   5. Удаление зависимостей (опционально):
   • Если Node.js, .NET SDK, PostgreSQL, CMake не используются другими приложениями:
     - Windows: через "Программы и компоненты" или `winget uninstall`
     - Linux: `sudo apt remove nodejs dotnet-sdk-8.0 postgresql cmake`
   
   • В противном случае оставить установленные

6. Очистка системы:
   • Удаление логов приложения (если есть)
   • Удаление конфигурационных файлов (если не сохранены)
   • Очистка кэша npm: `npm cache clean --force`
   • Очистка кэша .NET: `dotnet nuget locals all --clear`

6.3. Дополнительные требования

6.3.1. Безопасность
• Все соединения между компонентами должны использовать HTTPS в production (настроить SSL-сертификаты)
• Хранение конфиденциальных данных (JWT ключи, пароли БД) должно быть зашифровано
• Использование переменных окружения для чувствительных данных вместо хранения в конфигурационных файлах
• Регулярное обновление зависимостей и исправлений безопасности:
  - `npm audit fix` для Frontend
  - `dotnet list package --outdated` для Backend
  - Обновление системных пакетов

6.3.2. Резервное копирование
• Автоматическое ежедневное резервное копирование базы данных:
  # Пример cron job для Linux
  0 2 * * * pg_dump -U daoss_user -d daoss > /backups/daoss_$(date +\%Y\%m\%d).sql
  • Хранение резервных копий в течение минимум 30 дней
• Возможность восстановления из резервной копии:
 
  psql -U daoss_user -d daoss < daoss_backup_YYYYMMDD.sql
  6.3.3. Мониторинг и логирование
• Настройка логирования всех критических операций:
  - Backend: логи через ASP.NET Core ILogger (настроено в appsettings.json)
  - Frontend: логи в консоли браузера (Development) или внешний сервис (Production)
  - Parser: логи в stdout/stderr
  - Android-клиент: логи через Logcat в Android Studio (теги: `OkHttp`, `Retrofit`)
• Мониторинг доступности сервисов:
  - Проверка портов 5173, 5143, 8080
  - Health check эндпоинты (если реализованы)
  - Мониторинг подключений от Android-клиентов через Backend логи
• Уведомления об ошибках и сбоях (опционально: интеграция с внешними системами мониторинга)

6.3.4. Особенности развёртывания
• Автоматическое применение миграций при старте Backend (только в Development режиме)
• Использование скрипта `start-all.ps1` для упрощения запуска всех модулей
• Поддержка работы с Git подмодулями через скрипт `setup-submodules.ps1`
• Возможность сборки компонентов по отдельности или через флаги скрипта запуска

6.4. Требования к Android-клиенту

6.4.1. Системные требования для разработки

Системные требования к рабочей станции разработчика:
• Операционная система:
  - Windows 10/11
  - Linux (Ubuntu 22.04 LTS, Debian 11 или выше)
  - macOS 10.15+
• Процессор: Минимум 4 ядра (рекомендуется 8+)
• Оперативная память: Минимум 8 ГБ (рекомендуется 16 ГБ и более)
• Дисковое пространство: Минимум 20 ГБ свободного места (рекомендуется SSD)
  - Android SDK: ~10 ГБ
  - Android Studio: ~1 ГБ
  - Эмуляторы: ~5-10 ГБ (в зависимости от количества)

Программное обеспечение для разработки:

Обязательные компоненты:
• Android Studio (последняя стабильная версия) — интегрированная среда разработки
• JDK 11 или выше — для компиляции Android-приложения
• Android SDK:
  - Platform SDK: Android 13 (API 33) или выше
  - Build Tools: последняя версия
  - Android SDK Command-line Tools
• Gradle 8.10+ (обычно устанавливается автоматически с Android Studio)

Требования к целевому устройству:
• Минимальная версия Android: Android 7.0 (API 24, minSdk 24)
• Целевая версия Android: Android 14 (API 35, targetSdk 35)
• Компиляция для: Android 15 (API 36, compileSdk 36)
• Для тестирования: Android-эмулятор или физическое устройство с Android 7.0+

6.4.2. Процесс установки Android-клиента

1. Установка Android Studio:
   • Скачать Android Studio с https://developer.android.com/studio
   • Запустить установщик и следовать инструкциям
   • При первом запуске Android Studio автоматически установит:
     - Android SDK
     - Android SDK Platform-Tools
     - Android Emulator
     - Gradle

2. Настройка Android SDK:
   • Открыть Android Studio
   • Перейти в Settings/Preferences → Appearance & Behavior → System Settings → Android SDK
   • Установить следующие компоненты:
     - Android SDK Platform 36 (Android 15)
     - Android SDK Platform 35 (Android 14)
     - Android SDK Build-Tools (последняя версия)
     - Android SDK Command-line Tools
     - Android Emulator
     - Intel x86 Emulator Accelerator (HAXM) — для Windows/Linux
     - Google Play services (опционально)

3. Настройка переменных окружения (опционально, для командной строки):
   Windows:
   • Добавить в PATH:
     - `%LOCALAPPDATA%\Android\Sdk\platform-tools`
     - `%LOCALAPPDATA%\Android\Sdk\tools`
   
   Linux/macOS:
   • Добавить в `~/.bashrc` или `~/.zshrc`:
     ```bash
     export ANDROID_HOME=$HOME/Android/Sdk
     export PATH=$PATH:$ANDROID_HOME/platform-tools
     export PATH=$PATH:$ANDROID_HOME/tools
     ```

4. Клонирование репозитория:
   • Проект Android-клиента находится в папке `FlowchartEditorClient` корневого репозитория
   • Если репозиторий уже склонирован (см. раздел 6.1), папка `FlowchartEditorClient` уже доступна

5. Настройка подключения к серверу:
   • Открыть проект в Android Studio:
     - File → Open → выбрать папку `FlowchartEditorClient`
   • Настроить BASE_URL в `app/build.gradle`:
     
     Для Android Emulator (рекомендуется для разработки):
     ```gradle
     buildConfigField "String", "BASE_URL", "\"http://10.0.2.2:5143/\""
     ```
     Примечание: `10.0.2.2` — специальный IP-адрес эмулятора для обращения к localhost компьютера
     
     Для реального Android-устройства:
     ```gradle
     buildConfigField "String", "BASE_URL", "\"http://192.168.x.x:5143/\""
     ```
     (заменить `192.168.x.x` на IP-адрес компьютера в локальной сети)
     
     Для production:
     ```gradle
     buildTypes {
         release {
             buildConfigField "String", "BASE_URL", "\"https://your-production-server.com/\""
         }
     }
     ```

6. Синхронизация проекта:
   • Android Studio автоматически синхронизирует Gradle при открытии проекта
   • Если синхронизация не произошла автоматически:
     - File → Sync Project with Gradle Files
     - Или нажать кнопку "Sync Now" в уведомлении

7. Запуск приложения:
   • Подключить Android-устройство через USB (с включённой отладкой по USB) или запустить эмулятор
   • Создать виртуальное устройство (AVD):
     - Tools → Device Manager → Create Device
     - Выбрать устройство (например, Pixel 5)
     - Выбрать системный образ (Android 13 или выше)
     - Завершить создание и запустить эмулятор
   • Нажать кнопку Run (Shift+F10) или выбрать Run → Run 'app'
   • Приложение установится и запустится на устройстве/эмуляторе

8. Проверка работы:
   • Убедиться, что Backend сервер запущен (см. раздел 6.1)
   • В приложении выполнить регистрацию или вход
   • Проверить загрузку списка проектов
   • Проверить создание нового проекта

6.4.3. Процесс деинсталляции Android-клиента

1. Удаление приложения с устройства:
   • На устройстве: Настройки → Приложения → FlowchartEditorClient → Удалить
   • Или через Android Studio: Run → Uninstall 'app'

2. Удаление проекта (опционально):
   • Удаление директории проекта:
     
     cd ..
     rm -rf FlowchartEditorClient  # Linux/macOS
     # или
     Remove-Item -Recurse -Force FlowchartEditorClient  # Windows PowerShell
   
   • Удаление собранных артефактов:
     - `app/build/` (собранные APK и промежуточные файлы)
     - `.gradle/` (кэш Gradle, можно восстановить)
     - `.idea/` (настройки Android Studio, опционально)

3. Удаление Android Studio и SDK (опционально):
   • Если Android Studio не используется для других проектов:
     - Windows: через "Программы и компоненты" или `winget uninstall Google.AndroidStudio`
     - Linux: `sudo apt remove android-studio` (если установлен через пакетный менеджер)
     - macOS: удалить из папки Applications и удалить `~/Library/Android`
   
   • Удаление Android SDK:
     - Windows: `%LOCALAPPDATA%\Android\Sdk`
     - Linux/macOS: `~/Android/Sdk`

4. Очистка кэша:
   • Очистка кэша Gradle:
     - Windows: `%USERPROFILE%\.gradle\caches`
     - Linux/macOS: `~/.gradle/caches`
   
   • Очистка кэша Android Studio:
     - File → Invalidate Caches / Restart → Invalidate and Restart

6.4.4. Особенности развёртывания Android-клиента

Настройка для разных окружений:
• Development: использование `http://10.0.2.2:5143/` для эмулятора или локального IP для реального устройства
• Production: использование HTTPS с реальным доменом сервера

Сборка релизной версии:
• Build → Generate Signed Bundle / APK
• Выбрать APK или Android App Bundle (AAB)
• Создать или использовать существующий ключ подписи
• Выбрать release build variant
• Убедиться, что BASE_URL настроен для production

Отладка и логирование:
• Просмотр логов через Logcat в Android Studio
• HTTP-запросы логируются через `HttpLoggingInterceptor` (теги: `OkHttp`, `Retrofit`)
• Проверка работы API через Swagger UI на сервере

Безопасность:
• Для production использовать HTTPS вместо HTTP
• Настроить Network Security Config для доверия к сертификатам (если требуется)
• Хранение токенов аутентификации в Secure SharedPreferences или EncryptedSharedPreferences