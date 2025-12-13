cmake_minimum_required(VERSION 3.15)

project(DAOSSParser VERSION 1.0.0 LANGUAGES CXX)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Настройки для разных конфигураций
if(MSVC)
    add_compile_options(/W4 /WX-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # Определяем Windows 10 для поддержки httplib     КОСТЫЛЬ!!!!
    add_compile_definitions(_WIN32_WINNT=0x0A00)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Для MinGW тоже определяем Windows 10
    add_compile_definitions(_WIN32_WINNT=0x0A00)     КОСТЫЛЬ!!!!
endif()

# Путь к исходным файлам парсера
set(PARSER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(JSON_LIB_DIR "${PARSER_SOURCE_DIR}/json-3.12.0")

# Подключаем nlohmann/json как subdirectory
# Отключаем тесты и другие опции, которые нам не нужны
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders OFF CACHE INTERNAL "")

add_subdirectory("${JSON_LIB_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/json")

# Создаем интерфейсную библиотеку для nlohmann/json, если нужно
# Обычно это делается автоматически через add_subdirectory

# Путь к thirdparty библиотекам (cpp-httplib header-only)
set(THIRDPARTY_DIR "${PARSER_SOURCE_DIR}/thirdparty")

# Путь к основному исходному файлу
set(MAIN_SOURCE "${PARSER_SOURCE_DIR}/main.cpp")

# Проверяем существование файла
if(NOT EXISTS "${MAIN_SOURCE}")
    message(FATAL_ERROR "Файл main.cpp не найден: ${MAIN_SOURCE}")
endif()

# Создаем исполняемый файл
add_executable(parser-server "${MAIN_SOURCE}")

# Добавляем include директории
target_include_directories(parser-server PRIVATE
    "${PARSER_SOURCE_DIR}"
    "${PARSER_SOURCE_DIR}/Ast"
    "${PARSER_SOURCE_DIR}/CPPAst"
    "${PARSER_SOURCE_DIR}/CodeGen"
    "${PARSER_SOURCE_DIR}/Expression"
    "${PARSER_SOURCE_DIR}/Flowchart"
    "${PARSER_SOURCE_DIR}/Parser"
    "${PARSER_SOURCE_DIR}/Scripts"
    "${PARSER_SOURCE_DIR}/Auth"
    "${THIRDPARTY_DIR}"
)

# Подключаем nlohmann/json через интерфейсную библиотеку
target_link_libraries(parser-server PRIVATE
    nlohmann_json::nlohmann_json
)

# На Windows httplib требует ws2_32 и другие системные библиотеки
if(WIN32)
    target_link_libraries(parser-server PRIVATE ws2_32 wsock32)
endif()

# Установка исполняемого файла (опционально)
install(TARGETS parser-server
    RUNTIME DESTINATION bin
)

# Вывод информации
message(STATUS "=== Конфигурация DAOSS Parser ===")
message(STATUS "Версия CMake: ${CMAKE_VERSION}")
message(STATUS "Компилятор C++: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Стандарт C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "Путь к исходникам: ${PARSER_SOURCE_DIR}")
message(STATUS "Путь к nlohmann/json: ${JSON_LIB_DIR}")
message(STATUS "Исполняемый файл: parser-server")
