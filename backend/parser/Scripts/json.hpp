// Заголовочный файл-обертка для nlohmann/json
// Этот файл позволяет использовать #include "Scripts/json.hpp" вместо #include <nlohmann/json.hpp>

#ifndef SCRIPTS_JSON_HPP
#define SCRIPTS_JSON_HPP

#include <nlohmann/json.hpp>

// Для удобства используем пространство имен nlohmann
using json = nlohmann::json;

#endif // SCRIPTS_JSON_HPP





