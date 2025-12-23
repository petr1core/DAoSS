# Скрипт для автоматизации инициализации и настройки подмодулей
# Автоматически переключает подмодули на нужные ветки
# Кодировка: UTF-8 with BOM

param(
    [switch]$Verbose,
    [switch]$AddGitToPath
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

# Функция для поиска и добавления Git в PATH
function Ensure-GitInPath {
    # Проверяем, доступен ли git
    $gitCommand = Get-Command git -ErrorAction SilentlyContinue
    if ($gitCommand) {
        Write-Verbose-Custom "Git найден в PATH: $($gitCommand.Source)"
        return $true
    }
    
    # Стандартные пути установки Git для Windows
    $gitPaths = @(
        "C:\Program Files\Git\cmd",
        "C:\Program Files (x86)\Git\cmd",
        "C:\Program Files\Git\bin",
        "$env:LOCALAPPDATA\Programs\Git\cmd"
    )
    
    foreach ($gitPath in $gitPaths) {
        $gitExe = Join-Path $gitPath "git.exe"
        if (Test-Path $gitExe) {
            Write-Verbose-Custom "Git найден: $gitExe, добавляем в PATH"
            $env:PATH = "$gitPath;$env:PATH"
            
            # Проверяем, что теперь git доступен
            $gitCommand = Get-Command git -ErrorAction SilentlyContinue
            if ($gitCommand) {
                Write-Success "✓ Git добавлен в PATH: $gitPath"
                return $true
            }
        }
    }
    
    Write-Error-Custom "Ошибка: Git не найден. Убедитесь, что Git для Windows установлен."
    Write-Info "Скачать Git: https://git-scm.com/download/win"
    return $false
}

# Функция для проверки, является ли директория git репозиторием
function Test-GitRepository {
    param([string]$Path = ".")
    $gitPath = Join-Path $Path ".git"
    return (Test-Path $gitPath) -or (Test-Path (Join-Path $gitPath "HEAD"))
}

# Функция для безопасного переключения ветки
function Switch-ToBranch {
    param(
        [string]$BranchName,
        [string]$RemoteName = "origin",
        [string]$Context = ""
    )
    
    $success = $false
    $errorMessage = ""
    
    # Проверяем, что мы в git репозитории
    if (-not (Test-GitRepository)) {
        $errorMessage = "Директория не является git репозиторием"
        return @{ Success = $false; Message = $errorMessage }
    }
    
    # Получаем информацию о ветках
    Write-Verbose-Custom "Получение информации о ветках из $RemoteName..."
    $fetchResult = git fetch $RemoteName 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Verbose-Custom "Предупреждение: не удалось выполнить git fetch: $fetchResult"
    }
    
    # Проверяем текущую ветку
    $currentBranchOutput = git rev-parse --abbrev-ref HEAD 2>&1
    $currentBranch = $currentBranchOutput | Where-Object { $_ -notmatch "^(fatal|error)" }
    
    if ($currentBranch -eq $BranchName) {
        return @{ Success = $true; Message = "уже на ветке $BranchName"; AlreadyOnBranch = $true }
    }
    
    Write-Verbose-Custom "Текущая ветка: $currentBranch (ожидается: $BranchName)"
    
    # Проверяем существование ветки в удаленном репозитории
    $remoteBranchCheck = git ls-remote --heads $RemoteName $BranchName 2>&1
    if (-not $remoteBranchCheck -or ($remoteBranchCheck -match "^(fatal|error)")) {
        $errorMessage = "Ветка $BranchName не найдена в удаленном репозитории $RemoteName"
        return @{ Success = $false; Message = $errorMessage }
    }
    
    # Пытаемся переключиться на ветку
    Write-Verbose-Custom "Попытка переключения на ветку $BranchName..."
    $checkoutResult = git checkout $BranchName 2>&1
    if ($LASTEXITCODE -eq 0) {
        return @{ Success = $true; Message = "переключен на ветку $BranchName"; AlreadyOnBranch = $false }
    }
    
    # Если не удалось, возможно ветка существует только удаленно
    Write-Verbose-Custom "Локальная ветка не найдена, создание отслеживающей ветки..."
    $checkoutTrackResult = git checkout -b $BranchName "$RemoteName/$BranchName" 2>&1
    if ($LASTEXITCODE -eq 0) {
        return @{ Success = $true; Message = "создана и переключена ветка $BranchName"; AlreadyOnBranch = $false }
    }
    
    $errorMessage = "Не удалось переключиться на ветку $BranchName. Ошибка: $checkoutTrackResult"
    return @{ Success = $false; Message = $errorMessage }
}

# Сохраняем текущую директорию
$OriginalLocation = Get-Location

try {
    Write-Info "=== Настройка подмодулей ==="
    Write-Info ""

    # Проверяем доступность Git
    $gitCommand = Get-Command git -ErrorAction SilentlyContinue
    if (-not $gitCommand) {
        if ($AddGitToPath) {
            # Пытаемся добавить Git в PATH, если флаг установлен
            if (-not (Ensure-GitInPath)) {
                exit 1
            }
            Write-Info ""
        }
        else {
            Write-Error-Custom "Ошибка: Git не найден в PATH."
            Write-Info ""
            Write-Info "Варианты решения:"
            Write-Info "  1. Добавьте Git в системную переменную PATH вручную"
            Write-Info "  2. Запустите скрипт с флагом -AddGitToPath для автоматического добавления:"
            Write-Info "     .\setup-submodules.ps1 -AddGitToPath"
            Write-Info ""
            Write-Info "Стандартные пути установки Git:"
            Write-Info "  - C:\Program Files\Git\cmd"
            Write-Info "  - C:\Program Files (x86)\Git\cmd"
            Write-Info ""
            exit 1
        }
    }
    else {
        Write-Verbose-Custom "Git найден в PATH: $($gitCommand.Source)"
    }
    Write-Info ""

    # Проверяем, что мы в git репозитории
    if (-not (Test-Path ".git")) {
        Write-Error-Custom "Ошибка: .git директория не найдена. Убедитесь, что вы находитесь в корне git репозитория."
        exit 1
    }

    # Шаг 1: Инициализация и обновление всех подмодулей
    Write-Info "Шаг 1: Инициализация и обновление всех подмодулей..."
    Write-Verbose-Custom "Выполнение: git submodule update --init --recursive"
    $result = git submodule update --init --recursive 2>&1
    if ($Verbose) {
        Write-Host $result
    }
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Custom "Ошибка при инициализации подмодулей:"
        Write-Host $result
        Write-Error-Custom "Код возврата: $LASTEXITCODE"
        exit 1
    }
    Write-Success "✓ Подмодули инициализированы и обновлены"
    Write-Info ""

    # Шаг 2: Переключение backend_and_parser на ветку backend_and_parser
    if (Test-Path "backend_and_parser") {
        Write-Info "Шаг 2: Переключение backend_and_parser на ветку backend_and_parser..."
        Push-Location "backend_and_parser"
        
        try {
            if (-not (Test-GitRepository)) {
                Write-Error-Custom "Ошибка: backend_and_parser не является git репозиторием"
                exit 1
            }
            
            $switchResult = Switch-ToBranch -BranchName "backend_and_parser" -Context "backend_and_parser"
            if ($switchResult.Success) {
                if ($switchResult.AlreadyOnBranch) {
                    Write-Success "✓ backend_and_parser уже на ветке backend_and_parser"
                }
                else {
                    Write-Success "✓ backend_and_parser $($switchResult.Message)"
                }
                
                # Инициализируем подмодули внутри backend_and_parser
                Write-Info "Инициализация подмодулей внутри backend_and_parser..."
                $submoduleResult = git submodule update --init --recursive 2>&1
                if ($LASTEXITCODE -eq 0) {
                    Write-Success "✓ Подмодули backend_and_parser инициализированы"
                }
                else {
                    Write-Warning-Custom "Предупреждение: не удалось инициализировать подмодули backend_and_parser"
                    if ($Verbose) {
                        Write-Host $submoduleResult
                    }
                }
            }
            else {
                Write-Error-Custom "Ошибка: $($switchResult.Message)"
                exit 1
            }
        }
        catch {
            Write-Error-Custom "Критическая ошибка при работе с backend_and_parser: $_"
            exit 1
        }
        finally {
            Pop-Location
        }
        Write-Info ""
    }
    else {
        Write-Warning-Custom "Директория backend_and_parser не найдена, пропускаем..."
        Write-Info ""
    }

    # Шаг 3: Переключение вложенного подмодуля src/parser/Parser на ветку http-server_wip
    $nestedSubmodulePath = "backend_and_parser\src\parser\Parser"
    if (Test-Path "backend_and_parser") {
        # Проверяем, инициализирован ли подмодуль
        if (-not (Test-Path $nestedSubmodulePath)) {
            Write-Warning-Custom "Директория $nestedSubmodulePath не найдена."
            Write-Info "Попытка инициализации подмодулей backend_and_parser..."
            Push-Location "backend_and_parser"
            try {
                $submoduleResult = git submodule update --init --recursive 2>&1
                if ($LASTEXITCODE -eq 0) {
                    Write-Success "✓ Подмодули backend_and_parser инициализированы"
                }
                else {
                    Write-Warning-Custom "Не удалось инициализировать подмодули:"
                    if ($Verbose) {
                        Write-Host $submoduleResult
                    }
                }
            }
            finally {
                Pop-Location
            }
        }
        
        if (Test-Path $nestedSubmodulePath) {
            Write-Info "Шаг 3: Переключение вложенного подмодуля Parser на ветку http-server_wip..."
            Push-Location $nestedSubmodulePath
            
            try {
                if (-not (Test-GitRepository)) {
                    Write-Warning-Custom "Предупреждение: Parser не является git репозиторием."
                    Write-Info "Возможно, подмодуль не был правильно инициализирован."
                    Write-Info "Попробуйте выполнить вручную:"
                    Write-Info "  cd backend_and_parser"
                    Write-Info "  git submodule update --init --recursive"
                }
                else {
                    $switchResult = Switch-ToBranch -BranchName "http-server_wip" -Context "Parser"
                    if ($switchResult.Success) {
                        if ($switchResult.AlreadyOnBranch) {
                            Write-Success "✓ Parser уже на ветке http-server_wip"
                        }
                        else {
                            Write-Success "✓ Parser $($switchResult.Message)"
                        }
                    }
                    else {
                        Write-Warning-Custom "Предупреждение: $($switchResult.Message). Продолжаем работу..."
                    }
                }
            }
            catch {
                Write-Warning-Custom "Предупреждение при работе с Parser: $_. Продолжаем работу..."
            }
            finally {
                Pop-Location
            }
            Write-Info ""
        }
        else {
            Write-Warning-Custom "Директория $nestedSubmodulePath не найдена после инициализации, пропускаем..."
            Write-Info ""
        }
    }
    else {
        Write-Warning-Custom "Директория backend_and_parser не найдена, пропускаем..."
        Write-Info ""
    }

    Write-Success "=== Настройка подмодулей завершена успешно! ==="

}
catch {
    Write-Error-Custom "Критическая ошибка: $_"
    exit 1
}
finally {
    # Возвращаемся в исходную директорию
    Set-Location $OriginalLocation
}
