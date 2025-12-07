using System.Text.Json;
using DAOSS.Application.DTOs;

namespace DAOSS.Infrastructure.Services;

/// <summary>
/// Маппер для преобразования ответов парсера в DTOs
/// </summary>
public static class ParserResponseMapper
{
    /// <summary>
    /// Преобразует JSON ответ от парсера в ParserResponse
    /// </summary>
    public static ParserResponse MapToParserResponse(JsonElement jsonResponse, string language)
    {
        var response = new ParserResponse
        {
            Success = jsonResponse.TryGetProperty("success", out var successProp) && successProp.GetBoolean(),
            Error = jsonResponse.TryGetProperty("error", out var errorProp) && errorProp.ValueKind == JsonValueKind.String 
                ? errorProp.GetString() 
                : null
        };

        // Маппинг ошибок
        if (jsonResponse.TryGetProperty("lexerErrors", out var lexerErrorsProp) && lexerErrorsProp.ValueKind == JsonValueKind.Array)
        {
            response.LexerErrors = MapErrors(lexerErrorsProp);
        }

        if (jsonResponse.TryGetProperty("parserErrors", out var parserErrorsProp) && parserErrorsProp.ValueKind == JsonValueKind.Array)
        {
            response.ParserErrors = MapErrors(parserErrorsProp);
        }

        // Маппинг representation (SPR для Pascal, AST для C/C++)
        if (jsonResponse.TryGetProperty("representation", out var representationProp) && representationProp.ValueKind != JsonValueKind.Null)
        {
            // Нормализуем representation в зависимости от языка
            response.Representation = NormalizeAst(representationProp, language);
        }

        // Маппинг типа представления
        if (jsonResponse.TryGetProperty("representationType", out var representationTypeProp) && representationTypeProp.ValueKind == JsonValueKind.String)
        {
            response.RepresentationType = representationTypeProp.GetString();
        }

        return response;
    }

    /// <summary>
    /// Преобразует JSON ответ от парсера в ValidationResponse
    /// </summary>
    public static ValidationResponse MapToValidationResponse(JsonElement jsonResponse)
    {
        var response = new ValidationResponse
        {
            Valid = jsonResponse.TryGetProperty("valid", out var validProp) && validProp.GetBoolean()
        };

        if (jsonResponse.TryGetProperty("lexerErrors", out var lexerErrorsProp) && lexerErrorsProp.ValueKind == JsonValueKind.Array)
        {
            response.LexerErrors = MapErrors(lexerErrorsProp);
        }

        if (jsonResponse.TryGetProperty("parserErrors", out var parserErrorsProp) && parserErrorsProp.ValueKind == JsonValueKind.Array)
        {
            response.ParserErrors = MapErrors(parserErrorsProp);
        }

        return response;
    }

    /// <summary>
    /// Преобразует JSON ответ от парсера в SimpleValidationResponse
    /// </summary>
    public static SimpleValidationResponse MapToSimpleValidationResponse(JsonElement jsonResponse)
    {
        return new SimpleValidationResponse
        {
            Valid = jsonResponse.TryGetProperty("valid", out var validProp) && validProp.GetBoolean(),
            HasErrors = jsonResponse.TryGetProperty("hasErrors", out var hasErrorsProp) && hasErrorsProp.GetBoolean(),
            LexerErrorsCount = jsonResponse.TryGetProperty("lexerErrorsCount", out var lexerCountProp) 
                ? lexerCountProp.GetInt32() 
                : 0,
            ParserErrorsCount = jsonResponse.TryGetProperty("parserErrorsCount", out var parserCountProp) 
                ? parserCountProp.GetInt32() 
                : 0
        };
    }

    /// <summary>
    /// Маппит массив ошибок из JSON в список ParserError
    /// </summary>
    private static List<ParserError> MapErrors(JsonElement errorsArray)
    {
        var errors = new List<ParserError>();

        foreach (var errorElement in errorsArray.EnumerateArray())
        {
            var error = new ParserError();

            if (errorElement.TryGetProperty("position", out var posProp))
            {
                error.Position = posProp.GetInt32();
            }

            if (errorElement.TryGetProperty("line", out var lineProp))
            {
                error.Line = lineProp.GetInt32();
            }

            if (errorElement.TryGetProperty("column", out var columnProp))
            {
                error.Column = columnProp.GetInt32();
            }

            if (errorElement.TryGetProperty("type", out var typeProp) && typeProp.ValueKind == JsonValueKind.String)
            {
                error.Type = typeProp.GetString() ?? string.Empty;
            }

            if (errorElement.TryGetProperty("message", out var messageProp) && messageProp.ValueKind == JsonValueKind.String)
            {
                error.Message = messageProp.GetString() ?? string.Empty;
            }

            if (errorElement.TryGetProperty("value", out var valueProp) && valueProp.ValueKind == JsonValueKind.String)
            {
                error.Value = valueProp.GetString();
            }

            if (errorElement.TryGetProperty("expected", out var expectedProp) && expectedProp.ValueKind == JsonValueKind.String)
            {
                error.Expected = expectedProp.GetString();
            }

            if (errorElement.TryGetProperty("found", out var foundProp) && foundProp.ValueKind == JsonValueKind.String)
            {
                error.Found = foundProp.GetString();
            }

            errors.Add(error);
        }

        return errors;
    }

    /// <summary>
    /// Нормализует representation (SPR для Pascal, AST для C/C++) в зависимости от языка программирования
    /// Pascal имеет формат {"program": {...}}, C/C++ имеют формат {"type": "Program", ...}
    /// </summary>
    private static JsonElement? NormalizeAst(JsonElement ast, string language)
    {
        if (ast.ValueKind != JsonValueKind.Object)
        {
            return ast;
        }

        // Для Pascal: формат {"program": {...}} - оставляем как есть или можем обернуть
        // Для C/C++: формат {"type": "Program", ...} - оставляем как есть
        // Оба формата уже совместимы с JsonElement, просто возвращаем как есть
        
        // Если нужно, можно добавить дополнительную нормализацию здесь
        // Например, привести Pascal формат к формату C/C++ или наоборот
        
        return ast;
    }
}




