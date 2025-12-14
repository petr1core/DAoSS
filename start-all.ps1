# Скрипт для последовательного запуска всех модулей проекта
# Запускает: Parser -> Backend -> Frontend
#
# Параметры:
#   -Verbose          - Подробный вывод
#   -AddGitToPath     - Автоматически добавить Git в PATH
#   -BuildParser      - Собрать Parser перед запуском (cmake + build)
#   -BuildBackend     - Собрать Backend перед запуском (dotnet build)
#   -UpdateMigrations - Применить миграции базы данных перед запуском Backend

param(
    [switch]$Verbose,
    [switch]$AddGitToPath,
    [switch]$BuildParser,
    [switch]$BuildBackend,
    [switch]$UpdateMigrations
)

# Цвета для вывода
function Write-Info {
    param([string]$Message)
    Write-Host $Message -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host $Message -ForegroundColor Green
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host $Message -ForegroundColor Red
}

function Write-Warning-Custom {
    param([string]$Message)
    Write-Host $Message -ForegroundColor Yellow
}

function Write-Verbose-Custom {
    param([string]$Message)
    if ($Verbose) {
        Write-Host "  [VERBOSE] $Message" -ForegroundColor DarkGray
    }
}

# Цвета для вывода логов модулей
$script:ModuleColors = @{
    "Parser"   = "Magenta"
    "Backend"  = "Yellow"
    "Frontend" = "Cyan"
}

# Маппинг цветов PowerShell на ConsoleColor для быстрого вывода
$script:ColorMap = @{
    "Black"       = [ConsoleColor]::Black
    "DarkBlue"    = [ConsoleColor]::DarkBlue
    "DarkGreen"   = [ConsoleColor]::DarkGreen
    "DarkCyan"    = [ConsoleColor]::DarkCyan
    "DarkRed"     = [ConsoleColor]::DarkRed
    "DarkMagenta" = [ConsoleColor]::DarkMagenta
    "DarkYellow"  = [ConsoleColor]::DarkYellow
    "Gray"        = [ConsoleColor]::Gray
    "DarkGray"    = [ConsoleColor]::DarkGray
    "Blue"        = [ConsoleColor]::Blue
    "Green"       = [ConsoleColor]::Green
    "Cyan"        = [ConsoleColor]::Cyan
    "Red"         = [ConsoleColor]::Red
    "Magenta"     = [ConsoleColor]::Magenta
    "Yellow"      = [ConsoleColor]::Yellow
    "White"       = [ConsoleColor]::White
}

# Быстрая функция для цветного вывода (использует прямой доступ к консоли)
function Write-FastColor {
    param(
        [string]$Message,
        [string]$ForegroundColor = "White",
        [switch]$NoNewline
    )
    
    $originalColor = [Console]::ForegroundColor
    $consoleColor = $script:ColorMap[$ForegroundColor]
    if ($null -eq $consoleColor) {
        $consoleColor = [ConsoleColor]::White
    }
    
    [Console]::ForegroundColor = $consoleColor
    if ($NoNewline) {
        [Console]::Write($Message)
    } else {
        [Console]::WriteLine($Message)
    }
    [Console]::ForegroundColor = $originalColor
}

# Функция для сборки парсера
function Build-Parser {
    param([string]$ParserPath)
    
    Write-Info "=== Сборка Parser ==="
    
    $buildDir = Join-Path $ParserPath "build"
    
    # Ищем CMakeLists.txt в корне или в поддиректориях
    $cmakeLists = Join-Path $ParserPath "CMakeLists.txt"
    if (-not (Test-Path $cmakeLists)) {
        # Пробуем найти в поддиректориях
        $foundCmake = Get-ChildItem -Path $ParserPath -Filter "CMakeLists.txt" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($foundCmake) {
            $cmakeLists = $foundCmake.FullName
            $ParserPath = $foundCmake.DirectoryName
            $buildDir = Join-Path $ParserPath "build"
            Write-Verbose-Custom "Найден CMakeLists.txt: $cmakeLists"
        }
    }
    
    if (-not (Test-Path $cmakeLists)) {
        Write-Warning-Custom "CMakeLists.txt не найден в $ParserPath"
        Write-Warning-Custom "Пропускаем сборку Parser"
        return $false
    }
    
    # Проверяем наличие cmake
    $cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
    if (-not $cmakeCommand) {
        Write-Error-Custom "Ошибка: cmake не найден. Установите CMake для сборки Parser."
        return $false
    }
    
    try {
        # Создаем директорию build, если её нет
        if (-not (Test-Path $buildDir)) {
            Write-Info "Создаю директорию build..."
            New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
        }
        
        Push-Location $buildDir
        
        # Генерируем CMake файлы
        Write-Info "Генерирую CMake файлы..."
        $cmakeResult = & cmake .. 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Custom "Ошибка при генерации CMake файлов:"
            Write-Host $cmakeResult -ForegroundColor Red
            Pop-Location
            return $false
        }
        
        # Собираем проект
        Write-Info "Собираю проект..."
        $buildResult = & cmake --build . --config Release 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Custom "Ошибка при сборке проекта:"
            Write-Host $buildResult -ForegroundColor Red
            Pop-Location
            return $false
        }
        
        Pop-Location
        Write-Success "✓ Parser успешно собран"
        return $true
        
    }
    catch {
        Write-Error-Custom "Критическая ошибка при сборке Parser: $_"
        Pop-Location
        return $false
    }
}

# Функция для сборки бэкенда
function Build-Backend {
    param([string]$BackendPath)
    
    Write-Info "=== Сборка Backend ==="
    
    $dotnetCommand = Get-Command dotnet -ErrorAction SilentlyContinue
    if (-not $dotnetCommand) {
        Write-Error-Custom "Ошибка: .NET SDK не найден. Установите .NET SDK для сборки Backend."
        return $false
    }
    
    try {
        Push-Location $BackendPath
        
        Write-Info "Восстанавливаю зависимости..."
        $restoreResult = & dotnet restore 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Custom "Ошибка при восстановлении зависимостей:"
            Write-Host $restoreResult -ForegroundColor Red
            Pop-Location
            return $false
        }
        
        Write-Info "Собираю проект..."
        $buildResult = & dotnet build --configuration Release 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Custom "Ошибка при сборке проекта:"
            Write-Host $buildResult -ForegroundColor Red
            Pop-Location
            return $false
        }
        
        Pop-Location
        Write-Success "✓ Backend успешно собран"
        return $true
        
    }
    catch {
        Write-Error-Custom "Критическая ошибка при сборке Backend: $_"
        Pop-Location
        return $false
    }
}

# Функция для применения миграций бэкенда
function Update-BackendMigrations {
    param([string]$BackendPath)
    
    Write-Info "=== Применение миграций Backend ==="
    
    $dotnetCommand = Get-Command dotnet -ErrorAction SilentlyContinue
    if (-not $dotnetCommand) {
        Write-Error-Custom "Ошибка: .NET SDK не найден. Установите .NET SDK для применения миграций."
        return $false
    }
    
    # Проверяем наличие dotnet ef
    $efCommand = & dotnet ef --version 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Custom "Ошибка: dotnet ef не найден. Установите EF Core tools:"
        Write-Info "  dotnet tool install --global dotnet-ef"
        return $false
    }
    
    try {
        Push-Location $BackendPath
        
        Write-Info "Применяю миграции к базе данных..."
        $updateResult = & dotnet ef database update --startup-project DAOSS.WebApi.csproj --project Infrastructure/DAOSS.Infrastructure.csproj 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Custom "Ошибка при применении миграций:"
            Write-Host $updateResult -ForegroundColor Red
            Pop-Location
            return $false
        }
        
        Pop-Location
        Write-Success "✓ Миграции успешно применены"
        return $true
        
    }
    catch {
        Write-Error-Custom "Критическая ошибка при применении миграций: $_"
        Pop-Location
        return $false
    }
}

# Функция для поиска и добавления Git в PATH (если нужно)
function Ensure-GitInPath {
    $gitCommand = Get-Command git -ErrorAction SilentlyContinue
    if ($gitCommand) {
        return $true
    }
    
    $gitPaths = @(
        "C:\Program Files\Git\cmd",
        "C:\Program Files (x86)\Git\cmd",
        "C:\Program Files\Git\bin",
        "$env:LOCALAPPDATA\Programs\Git\cmd"
    )
    
    foreach ($gitPath in $gitPaths) {
        $gitExe = Join-Path $gitPath "git.exe"
        if (Test-Path $gitExe) {
            $env:PATH = "$gitPath;$env:PATH"
            $gitCommand = Get-Command git -ErrorAction SilentlyContinue
            if ($gitCommand) {
                Write-Success "✓ Git добавлен в PATH: $gitPath"
                return $true
            }
        }
    }
    
    return $false
}

# Функция для запуска процесса с перехватом ошибок
function Start-Module {
    param(
        [string]$Name,
        [string]$WorkingDirectory,
        [string]$Command,
        [string[]]$Arguments,
        [int]$Port = 0
    )
    
    Write-Info "=== Запуск $Name ==="
    Write-Verbose-Custom "Рабочая директория: $WorkingDirectory"
    Write-Verbose-Custom "Команда: $Command $($Arguments -join ' ')"
    
    if (-not (Test-Path $WorkingDirectory)) {
        Write-Error-Custom "Ошибка: Директория не найдена: $WorkingDirectory"
        return $null
    }
    
    # Проверяем наличие команды
    $commandExists = Get-Command $Command -ErrorAction SilentlyContinue
    if (-not $commandExists) {
        Write-Error-Custom "Ошибка: Команда не найдена: $Command"
        return $null
    }
    
    # Разрешаем команду в полный путь и корректно запускаем .ps1/.cmd/.bat
    $resolvedCommand = $null
    if (Test-Path $Command) {
        $resolvedCommand = (Resolve-Path $Command).Path
    }
    elseif ($commandExists.Source) {
        $resolvedCommand = $commandExists.Source
    }
    elseif ($commandExists.Path) {
        $resolvedCommand = $commandExists.Path
    }
    elseif ($commandExists.Definition -and (Test-Path $commandExists.Definition)) {
        $resolvedCommand = $commandExists.Definition
    }
    else {
        # Пробуем найти .cmd/.bat/.exe версию команды в PATH
        $extensions = @(".cmd", ".bat", ".exe", "")
        foreach ($ext in $extensions) {
            $cmdWithExt = $Command + $ext
            $found = Get-Command $cmdWithExt -ErrorAction SilentlyContinue
            if ($found -and $found.Source) {
                $resolvedCommand = $found.Source
                break
            }
        }
        if (-not $resolvedCommand) {
            $resolvedCommand = $Command
        }
    }

    $resolvedExt = [System.IO.Path]::GetExtension($resolvedCommand).ToLowerInvariant()

    $processInfo = New-Object System.Diagnostics.ProcessStartInfo
    if ($resolvedExt -eq ".ps1") {
        # ps1 нельзя запускать как exe — запускаем через powershell
        $pwsh = (Get-Command powershell -ErrorAction SilentlyContinue)
        $processInfo.FileName = if ($pwsh) { $pwsh.Source } else { "powershell" }
        $processInfo.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$resolvedCommand`" " + ($Arguments -join ' ')
    }
    elseif ($resolvedExt -in @(".cmd", ".bat")) {
        # cmd/bat запускаем через cmd.exe
        $processInfo.FileName = $env:ComSpec
        $processInfo.Arguments = "/c `"$resolvedCommand`" " + ($Arguments -join ' ')
    }
    else {
        $processInfo.FileName = $resolvedCommand
        $processInfo.Arguments = $Arguments -join ' '
    }
    $processInfo.WorkingDirectory = $WorkingDirectory
    $processInfo.UseShellExecute = $false
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true
    $processInfo.CreateNoWindow = $false
    
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $processInfo
    
    # Буферы для вывода
    $outputBuilder = New-Object System.Text.StringBuilder
    $errorBuilder = New-Object System.Text.StringBuilder
    
    # Обработчики событий для перехвата вывода
    # Используем прямой доступ к консоли для максимальной производительности
    $color = $script:ModuleColors[$Name]
    $consoleColor = $script:ColorMap[$color]
    if ($null -eq $consoleColor) {
        $consoleColor = [ConsoleColor]::White
    }
    
    $outputEvent = Register-ObjectEvent -InputObject $process -EventName OutputDataReceived -Action {
        if ($EventArgs.Data) {
            [void]$Event.MessageData.Builder.AppendLine($EventArgs.Data)
            
            # Быстрый вывод через прямой доступ к консоли
            $originalColor = [Console]::ForegroundColor
            $prefix = "[$($Event.MessageData.Name)]"
            [Console]::ForegroundColor = $Event.MessageData.ConsoleColor
            [Console]::Write($prefix + " ")
            [Console]::ForegroundColor = $originalColor
            [Console]::WriteLine($EventArgs.Data)
        }
    } -MessageData @{ Builder = $outputBuilder; Name = $Name; Color = $color; ConsoleColor = $consoleColor }
    
    $errorEvent = Register-ObjectEvent -InputObject $process -EventName ErrorDataReceived -Action {
        if ($EventArgs.Data) {
            [void]$Event.MessageData.Builder.AppendLine($EventArgs.Data)
            
            # Быстрый вывод через прямой доступ к консоли
            $originalColor = [Console]::ForegroundColor
            $prefix = "[$($Event.MessageData.Name) ERROR]"
            [Console]::ForegroundColor = $Event.MessageData.ConsoleColor
            [Console]::Write($prefix + " ")
            [Console]::ForegroundColor = [ConsoleColor]::Red
            [Console]::WriteLine($EventArgs.Data)
            [Console]::ForegroundColor = $originalColor
        }
    } -MessageData @{ Builder = $errorBuilder; Name = $Name; Color = $color; ConsoleColor = $consoleColor }
    
    try {
        Write-Info "Запускаю $Name..."
        $process.Start() | Out-Null
        $process.BeginOutputReadLine()
        $process.BeginErrorReadLine()
        
        # Ждем немного, чтобы проверить, не упал ли процесс сразу
        Start-Sleep -Milliseconds 2000
        
        if ($process.HasExited) {
            $exitCode = $process.ExitCode
            $stdout = $outputBuilder.ToString()
            $stderr = $errorBuilder.ToString()
            
            Write-Error-Custom ""
            Write-Error-Custom "Ошибка: $Name завершился с кодом $exitCode"
            Write-Error-Custom "----------------------------------------"
            if ($stdout) {
                Write-Error-Custom "STDOUT:"
                Write-Host $stdout -ForegroundColor Red
            }
            if ($stderr) {
                Write-Error-Custom "STDERR:"
                Write-Host $stderr -ForegroundColor Red
            }
            Write-Error-Custom "----------------------------------------"
            
            # Очищаем события
            Unregister-Event -SourceIdentifier $outputEvent.Name -ErrorAction SilentlyContinue
            Unregister-Event -SourceIdentifier $errorEvent.Name -ErrorAction SilentlyContinue
            return $null
        }
        
        Write-Success "✓ $Name запущен (PID: $($process.Id))"
        if ($Port -gt 0) {
            Write-Info "  Ожидаемый порт: $Port"
        }
        
        return @{
            Process       = $process
            Name          = $Name
            OutputBuilder = $outputBuilder
            ErrorBuilder  = $errorBuilder
            OutputEvent   = $outputEvent
            ErrorEvent    = $errorEvent
        }
        
    }
    catch {
        Write-Error-Custom "Критическая ошибка при запуске $Name : $_"
        Write-Error-Custom "Детали ошибки: $($_.Exception.Message)"
        Unregister-Event -SourceIdentifier $outputEvent.Name -ErrorAction SilentlyContinue
        Unregister-Event -SourceIdentifier $errorEvent.Name -ErrorAction SilentlyContinue
        return $null
    }
}

# Сохраняем текущую директорию
$OriginalLocation = Get-Location
$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptRoot

$processes = @()

try {
    Write-Info "=========================================="
    Write-Info "  Запуск всех модулей проекта DAOSS"
    Write-Info "=========================================="
    Write-Info ""
    
    # Проверяем Git, если нужно
    if ($AddGitToPath) {
        Ensure-GitInPath | Out-Null
    }
    
    # 1. Запуск Parser
    # Реальный путь к CMakeLists парсера и сборке
    $parserPath = Join-Path $ScriptRoot "backend_and_parser\src\parser\Parser\backend\parser"
    $parserExe = Join-Path $parserPath "parser-server.exe"
    $parserBuildPath = Join-Path $parserPath "build\parser-server.exe"
    $parserBuildReleasePath = Join-Path $parserPath "build\Release\parser-server.exe"
    $parserBuildDebugPath = Join-Path $parserPath "build\Debug\parser-server.exe"
    $parserBuildDir = Join-Path $parserPath "build"
    
    # Проверяем, нужно ли собирать парсер
    $parserNeedsBuild = $false
    $parserExists = (Test-Path $parserExe) -or (Test-Path $parserBuildPath) -or (Test-Path $parserBuildReleasePath) -or (Test-Path $parserBuildDebugPath)
    
    if ($BuildParser) {
        $parserNeedsBuild = $true
    }
    elseif (-not $parserExists) {
        # Если исполняемый файл не найден, проверяем наличие build директории
        if (-not (Test-Path $parserBuildDir) -or ((Get-ChildItem $parserBuildDir -Filter "*.exe" -Recurse -ErrorAction SilentlyContinue).Count -eq 0)) {
            Write-Warning-Custom "Parser не найден. Используйте флаг -BuildParser для сборки."
        }
    }
    
    if ($parserNeedsBuild) {
        Write-Info ""
        if (-not (Build-Parser -ParserPath $parserPath)) {
            Write-Warning-Custom "Не удалось собрать Parser, пропускаем запуск..."
        }
        else {
            Write-Info ""
        }
    }
    
    # Определяем путь к исполняемому файлу парсера
    $parserExecutable = $null
    if (Test-Path $parserExe) {
        $parserExecutable = $parserExe
    }
    elseif (Test-Path $parserBuildReleasePath) {
        $parserExecutable = $parserBuildReleasePath
    }
    elseif (Test-Path $parserBuildDebugPath) {
        $parserExecutable = $parserBuildDebugPath
    }
    elseif (Test-Path $parserBuildPath) {
        $parserExecutable = $parserBuildPath
    }
    else {
        # Ищем любой .exe файл в build директории
        $foundExe = Get-ChildItem -Path $parserBuildDir -Filter "*.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($foundExe) {
            $parserExecutable = $foundExe.FullName
        }
    }
    
    if ($parserExecutable) {
        $parserProcess = Start-Module -Name "Parser" `
            -WorkingDirectory $parserPath `
            -Command $parserExecutable `
            -Arguments @() `
            -Port 8080
        if ($parserProcess) {
            $processes += $parserProcess
        }
        else {
            Write-Warning-Custom "Не удалось запустить Parser, продолжаем..."
        }
    }
    else {
        Write-Warning-Custom "Parser не найден. Пропускаем запуск Parser."
        Write-Info "  Ожидаемые пути:"
        Write-Info "    - $parserExe"
        Write-Info "    - $parserBuildPath"
        Write-Info "    - $parserBuildReleasePath"
        Write-Info "    - $parserBuildDebugPath"
    }
    
    Write-Info ""
    Start-Sleep -Seconds 2
    
    # 2. Запуск Backend
    $backendPath = Join-Path $ScriptRoot "backend_and_parser\src\WebApi"
    $backendProject = Join-Path $backendPath "DAOSS.WebApi.csproj"
    
    if (Test-Path $backendProject) {
        # Применяем миграции, если нужно
        if ($UpdateMigrations) {
            Write-Info ""
            if (-not (Update-BackendMigrations -BackendPath $backendPath)) {
                Write-Warning-Custom "Не удалось применить миграции, продолжаем запуск..."
            }
            else {
                Write-Info ""
            }
        }
        
        # Собираем бэкенд, если нужно
        if ($BuildBackend) {
            Write-Info ""
            if (-not (Build-Backend -BackendPath $backendPath)) {
                Write-Warning-Custom "Не удалось собрать Backend, продолжаем запуск..."
            }
            else {
                Write-Info ""
            }
        }
        
        $dotnetCommand = Get-Command dotnet -ErrorAction SilentlyContinue
        if (-not $dotnetCommand) {
            Write-Error-Custom "Ошибка: .NET SDK не найден. Установите .NET SDK для запуска Backend."
        }
        else {
            $backendProcess = Start-Module -Name "Backend" `
                -WorkingDirectory $backendPath `
                -Command "dotnet" `
                -Arguments @("run", "--project", "DAOSS.WebApi.csproj") `
                -Port 5143
            if ($backendProcess) {
                $processes += $backendProcess
            }
            else {
                Write-Error-Custom "Не удалось запустить Backend!"
            }
        }
    }
    else {
        Write-Error-Custom "Ошибка: Backend проект не найден: $backendProject"
    }
    
    Write-Info ""
    Start-Sleep -Seconds 3
    
    # 3. Запуск Frontend
    $frontendPath = $ScriptRoot
    $packageJson = Join-Path $frontendPath "package.json"
    
    if (Test-Path $packageJson) {
        $npmCommand = Get-Command npm -ErrorAction SilentlyContinue
        if (-not $npmCommand) {
            Write-Error-Custom "Ошибка: npm не найден. Установите Node.js для запуска Frontend."
        }
        else {
            # Убеждаемся, что зависимости установлены (vite приходит как devDependency)
            $nodeModulesDir = Join-Path $frontendPath "node_modules"
            $viteBin = Join-Path $nodeModulesDir ".bin\vite.cmd"
            if (-not (Test-Path $viteBin)) {
                Write-Info "Зависимости Frontend не найдены (vite отсутствует). Устанавливаю зависимости..."
                Push-Location $frontendPath
                try {
                    & cmd.exe /c npm install 2>&1 | ForEach-Object { Write-Host $_ }
                    if ($LASTEXITCODE -ne 0) {
                        Write-Error-Custom "Ошибка: не удалось установить зависимости Frontend (npm exit code: $LASTEXITCODE)"
                    }
                }
                finally {
                    Pop-Location
                }
            }

            $frontendProcess = Start-Module -Name "Frontend" `
                -WorkingDirectory $frontendPath `
                -Command "cmd.exe" `
                -Arguments @("/c", "npm", "run", "dev") `
                -Port 5173
            if ($frontendProcess) {
                $processes += $frontendProcess
            }
            else {
                Write-Error-Custom "Не удалось запустить Frontend!"
            }
        }
    }
    else {
        Write-Error-Custom "Ошибка: Frontend package.json не найден: $packageJson"
    }
    
    Write-Info ""
    Write-Info "=========================================="
    Write-Success "Все модули запущены!"
    Write-Info "=========================================="
    Write-Info ""
    Write-Info "Запущенные процессы:"
    foreach ($proc in $processes) {
        Write-Info "  - $($proc.Name) (PID: $($proc.Process.Id))"
    }
    Write-Info ""
    Write-Warning-Custom "Нажмите Ctrl+C для остановки всех модулей"
    Write-Info ""
    
    # Ожидаем завершения или прерывания
    try {
        while ($true) {
            Start-Sleep -Seconds 1
            
            # Проверяем, не завершились ли процессы
            foreach ($proc in $processes) {
                if ($proc.Process.HasExited) {
                    $exitCode = $proc.Process.ExitCode
                    $stdout = $proc.OutputBuilder.ToString()
                    $stderr = $proc.ErrorBuilder.ToString()
                    
                    Write-Error-Custom ""
                    Write-Error-Custom "=========================================="
                    Write-Error-Custom "  $($proc.Name) завершился с кодом $exitCode"
                    Write-Error-Custom "=========================================="
                    
                    if ($stdout) {
                        Write-Error-Custom "STDOUT:"
                        Write-Host $stdout -ForegroundColor Red
                    }
                    if ($stderr) {
                        Write-Error-Custom "STDERR:"
                        Write-Host $stderr -ForegroundColor Red
                    }
                    
                    # Удаляем завершившийся процесс из списка
                    $processes = $processes | Where-Object { $_.Process.Id -ne $proc.Process.Id }
                }
            }
            
            if ($processes.Count -eq 0) {
                Write-Warning-Custom "Все процессы завершены."
                break
            }
        }
    }
    catch {
        # Пользователь нажал Ctrl+C или произошла ошибка
        Write-Info ""
        Write-Warning-Custom "Остановка всех модулей..."
    }
    
}
catch {
    Write-Error-Custom "Критическая ошибка: $_"
}
finally {
    # Останавливаем все процессы
    Write-Info ""
    Write-Info "Остановка всех модулей..."
    
    foreach ($proc in $processes) {
        if (-not $proc.Process.HasExited) {
            Write-Info "Останавливаю $($proc.Name)..."
            try {
                $proc.Process.Kill()
                $proc.Process.WaitForExit(5000)
                Write-Success "✓ $($proc.Name) остановлен"
            }
            catch {
                Write-Warning-Custom "Не удалось корректно остановить $($proc.Name): $_"
            }
        }
        
        # Очищаем события
        Unregister-Event -SourceIdentifier $proc.OutputEvent.Name -ErrorAction SilentlyContinue
        Unregister-Event -SourceIdentifier $proc.ErrorEvent.Name -ErrorAction SilentlyContinue
    }
    
    # Возвращаемся в исходную директорию
    Set-Location $OriginalLocation
    
    Write-Info ""
    Write-Success "Все модули остановлены."
}
