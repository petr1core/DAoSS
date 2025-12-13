//
// Created by пк on 06.12.2025.
// Упрощенный валидатор JWT токенов
//

#ifndef AISDLAB_JWTVALIDATOR_H
#define AISDLAB_JWTVALIDATOR_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "../Scripts/json.hpp"

using json = nlohmann::json;

class JwtValidator {
private:
    std::string secretKey;
    std::string issuer;
    std::string audience;

    // Base64 декодирование (упрощенная версия)
    std::string base64Decode(const std::string &encoded) const {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string decoded;
        int val = 0, valb = -8;
        
        for (unsigned char c : encoded) {
            if (c == '=') break;
            size_t pos = chars.find(c);
            if (pos == std::string::npos) continue;
            
            val = (val << 6) + static_cast<int>(pos);
            valb += 6;
            
            if (valb >= 0) {
                decoded.push_back(static_cast<char>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        
        return decoded;
    }

    // Удаляет padding из base64 строки
    std::string removeBase64Padding(const std::string &str) const {
        std::string result = str;
        // Заменяем - на + и _ на / для URL-safe base64
        std::replace(result.begin(), result.end(), '-', '+');
        std::replace(result.begin(), result.end(), '_', '/');
        
        // Добавляем padding если нужно
        int padding = 4 - (result.length() % 4);
        if (padding != 4) {
            result.append(padding, '=');
        }
        
        return result;
    }

    // Парсит JWT payload
    json parsePayload(const std::string &token) const {
        try {
            size_t firstDot = token.find('.');
            size_t secondDot = token.find('.', firstDot + 1);
            
            if (firstDot == std::string::npos || secondDot == std::string::npos) {
                return json();
            }
            
            std::string payloadPart = token.substr(firstDot + 1, secondDot - firstDot - 1);
            payloadPart = removeBase64Padding(payloadPart);
            std::string decoded = base64Decode(payloadPart);
            
            return json::parse(decoded);
        } catch (...) {
            return json();
        }
    }

    // Проверяет срок действия токена
    bool isTokenExpired(const json &payload) const {
        if (!payload.contains("exp")) {
            return false; // Если exp нет, считаем токен валидным (для тестов)
        }
        
        try {
            int64_t exp = payload["exp"].get<int64_t>();
            std::time_t currentTime = std::time(nullptr);
            
            // Добавляем небольшой запас (30 секунд) для clock skew
            return currentTime >= (exp - 30);
        } catch (...) {
            return true; // Если не можем прочитать exp, считаем истекшим
        }
    }

public:
    JwtValidator(const std::string &key, const std::string &iss, const std::string &aud)
        : secretKey(key), issuer(iss), audience(aud) {}

    // Валидирует JWT токен
    // Возвращает true если токен валиден, false иначе
    bool validate(const std::string &token) const {
        std::cout << "[JWT] Starting token validation..." << std::endl;
        
        if (token.empty()) {
            std::cout << "[JWT] Token is empty" << std::endl;
            return false;
        }
        std::cout << "[JWT] Token length: " << token.length() << std::endl;

        // Проверяем формат (должно быть 3 части, разделенные точками)
        size_t dotCount = std::count(token.begin(), token.end(), '.');
        std::cout << "[JWT] Dot count: " << dotCount << std::endl;
        if (dotCount != 2) {
            std::cout << "[JWT] Invalid format: expected 2 dots, got " << dotCount << std::endl;
            return false;
        }

        // Парсим payload
        std::cout << "[JWT] Parsing payload..." << std::endl;
        json payload = parsePayload(token);
        if (payload.is_null() || payload.empty()) {
            std::cout << "[JWT] Payload is null or empty" << std::endl;
            return false;
        }
        std::cout << "[JWT] Payload: " << payload.dump(2) << std::endl;

        // Проверяем срок действия
        std::cout << "[JWT] Checking expiration..." << std::endl;
        // ВРЕМЕННО ОТКЛЮЧЕНО ДЛЯ ОТЛАДКИ
        if (isTokenExpired(payload)) {
            std::cout << "[JWT] Token is expired" << std::endl;
            return false;
        }
        std::cout << "[JWT] Token expiration check SKIPPED (debug mode)" << std::endl;

        // Проверяем issuer
        if (!issuer.empty() && payload.contains("iss")) {
            std::string tokenIssuer = payload["iss"].get<std::string>();
            std::cout << "[JWT] Checking issuer: expected '" << issuer << "', got '" << tokenIssuer << "'" << std::endl;
            if (tokenIssuer != issuer) {
                std::cout << "[JWT] Issuer mismatch" << std::endl;
                return false;
            }
        }

        // Проверяем audience
        if (!audience.empty() && payload.contains("aud")) {
            std::string tokenAudience;
            if (payload["aud"].is_string()) {
                tokenAudience = payload["aud"].get<std::string>();
            } else if (payload["aud"].is_array()) {
                // audience может быть массивом
                auto audArray = payload["aud"].get<std::vector<std::string>>();
                std::cout << "[JWT] Checking audience array with " << audArray.size() << " entries" << std::endl;
                bool found = false;
                for (const auto &aud : audArray) {
                    if (aud == audience) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::cout << "[JWT] Audience not found in array" << std::endl;
                    return false;
                }
                std::cout << "[JWT] Token validation PASSED" << std::endl;
                return true; // Если нашли в массиве, токен валиден
            }
            
            std::cout << "[JWT] Checking audience: expected '" << audience << "', got '" << tokenAudience << "'" << std::endl;
            if (tokenAudience != audience) {
                std::cout << "[JWT] Audience mismatch" << std::endl;
                return false;
            }
        }

        // ВАЖНО: Здесь нет проверки подписи!
        // Для полной валидации нужно использовать библиотеку jwt-cpp
        // или реализовать проверку HMAC-SHA256 подписи
        
        std::cout << "[JWT] Token validation PASSED" << std::endl;
        return true;
    }

    // Извлекает Bearer токен из заголовка Authorization
    static std::string extractTokenFromHeader(const std::string &authHeader) {
        const std::string bearerPrefix = "Bearer ";
        if (authHeader.length() < bearerPrefix.length()) {
            return "";
        }
        
        if (authHeader.substr(0, bearerPrefix.length()) == bearerPrefix) {
            return authHeader.substr(bearerPrefix.length());
        }
        
        return "";
    }
};

#endif //AISDLAB_JWTVALIDATOR_H

