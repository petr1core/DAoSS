using System.Net;
using System.Net.Http;
using System.Net.Http.Json;
using System.Text.Json;
using DAOSS.Application.DTOs;
using DAOSS.Application.Interfaces;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Http;
using Microsoft.Extensions.Logging;

namespace DAOSS.Infrastructure.Services;

public class ParserService : IParserService
{
	private readonly HttpClient _httpClient;
	private readonly ILogger<ParserService> _logger;
	private readonly IHttpContextAccessor _httpContextAccessor;
	private readonly string _baseUrl;
	private readonly int _timeoutSeconds;

	public ParserService(
		IHttpClientFactory httpClientFactory,
		IConfiguration configuration,
		ILogger<ParserService> logger,
		IHttpContextAccessor httpContextAccessor)
	{
		_httpClient = httpClientFactory.CreateClient(nameof(ParserService));
		_logger = logger;
		_httpContextAccessor = httpContextAccessor;
		
		var parserSection = configuration.GetSection("Parser");
		_baseUrl = parserSection.GetValue<string>("BaseUrl") ?? "http://localhost:8080";
		var timeout = parserSection.GetValue<int?>("TimeoutSeconds");
		_timeoutSeconds = timeout.HasValue && timeout.Value > 0 ? timeout.Value : 30;
		
		_httpClient.BaseAddress = new Uri(_baseUrl);
		_httpClient.Timeout = TimeSpan.FromSeconds(_timeoutSeconds);
		
		_logger.LogInformation("ParserService initialized with BaseUrl: {BaseUrl}, Timeout: {TimeoutSeconds}s", 
			_baseUrl, _timeoutSeconds);
	}
	
	private string? GetAuthToken()
	{
		var authHeader = _httpContextAccessor.HttpContext?.Request.Headers["Authorization"].ToString();
		if (string.IsNullOrEmpty(authHeader))
		{
			return null;
		}
		
		// Убираем префикс "Bearer " если он есть
		return authHeader.StartsWith("Bearer ", StringComparison.OrdinalIgnoreCase)
			? authHeader.Substring(7)
			: authHeader;
	}

	public async Task<ParserResponse> ParseToAstAsync(string code, string language, CancellationToken cancellationToken = default)
	{
		try
		{
			_logger.LogDebug("Parsing code (language: {Language}, length: {CodeLength})", language, code?.Length ?? 0);
			
			// Логируем первые и последние символы кода для проверки целостности
			if (!string.IsNullOrEmpty(code) && code.Length > 100)
			{
				_logger.LogDebug("Code preview: first 100 chars: {FirstChars}... last 100 chars: ...{LastChars}", 
					code.Substring(0, Math.Min(100, code.Length)),
					code.Substring(Math.Max(0, code.Length - 100)));
			}

		var request = new ParserRequest
		{
			Code = code,
			Language = language
		};

		// Добавляем JWT токен в заголовок Authorization
		var token = GetAuthToken();
		if (!string.IsNullOrEmpty(token))
		{
			_httpClient.DefaultRequestHeaders.Authorization = 
				new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);
		}

		_logger.LogInformation("Sending request to parser service at {BaseUrl}/api/parse", _httpClient.BaseAddress);
		_logger.LogDebug("Request payload size: {Size} bytes", System.Text.Encoding.UTF8.GetByteCount(System.Text.Json.JsonSerializer.Serialize(request)));
		
		var response = await _httpClient.PostAsJsonAsync("/api/parse", request, cancellationToken);
		
		_logger.LogInformation("Parser service responded with status: {StatusCode}", response.StatusCode);
			
			if (!response.IsSuccessStatusCode)
			{
				_logger.LogWarning("Parser service returned error status: {StatusCode}", response.StatusCode);
				
				// Пытаемся получить детали ошибки из ответа
				string errorDetails = string.Empty;
				try
				{
					var errorContent = await response.Content.ReadAsStringAsync(cancellationToken);
					if (!string.IsNullOrEmpty(errorContent))
					{
						errorDetails = errorContent;
						_logger.LogWarning("Parser service error details: {ErrorDetails}", errorDetails);
					}
				}
				catch
				{
					// Игнорируем ошибки чтения тела ответа
				}
				
				if (response.StatusCode == HttpStatusCode.ServiceUnavailable || 
				    response.StatusCode == HttpStatusCode.GatewayTimeout)
				{
					var message = $"Parser service unavailable: {response.StatusCode}";
					if (!string.IsNullOrEmpty(errorDetails))
					{
						message += $". Details: {errorDetails}";
					}
					throw new HttpRequestException(message);
				}
				
				// Для других ошибок тоже включаем детали
				if (!string.IsNullOrEmpty(errorDetails))
				{
					throw new HttpRequestException($"Parser service error: {response.StatusCode}. {errorDetails}");
				}
			}

		response.EnsureSuccessStatusCode();
		
		// Читаем ответ как JsonElement для использования маппера
		var jsonResponse = await response.Content.ReadFromJsonAsync<JsonElement>(
			new JsonSerializerOptions { PropertyNameCaseInsensitive = true }, 
			cancellationToken);

		if (jsonResponse.ValueKind == JsonValueKind.Null || jsonResponse.ValueKind == JsonValueKind.Undefined)
		{
			_logger.LogError("Failed to deserialize parser response");
			throw new InvalidOperationException("Parser service returned null response");
		}

		// Используем маппер для преобразования ответа
		var result = ParserResponseMapper.MapToParserResponse(jsonResponse, language);

		_logger.LogDebug("Parsing completed. Success: {Success}", result.Success);
		return result;
		}
		catch (TaskCanceledException ex) when (ex.InnerException is TimeoutException)
		{
			_logger.LogError(ex, "Timeout while parsing code");
			throw new HttpRequestException("Parser service request timed out", ex);
		}
		catch (TaskCanceledException ex) when (!(ex.InnerException is TimeoutException))
		{
			// Обработка отмены запроса (может быть связано с падением парсера)
			_logger.LogError(ex, "Request was cancelled - parser may have crashed");
			throw new HttpRequestException(
				"Parser service request was cancelled. The parser may have crashed while processing the code.", ex);
		}
		catch (HttpRequestException ex)
		{
			_logger.LogError(ex, "HTTP error while calling parser service");
			
			// Проверяем, не связана ли ошибка с разрывом соединения (падение парсера)
			if (ex.Message.Contains("connection", StringComparison.OrdinalIgnoreCase) ||
			    ex.Message.Contains("refused", StringComparison.OrdinalIgnoreCase) ||
			    ex.Message.Contains("reset", StringComparison.OrdinalIgnoreCase))
			{
				_logger.LogError("Parser service connection error - parser may have crashed");
				throw new HttpRequestException(
					"Parser service connection error. The parser may have crashed while processing the code. " +
					"This can happen with complex code that causes memory allocation errors.", ex);
			}
			
			throw;
		}
		catch (JsonException ex)
		{
			_logger.LogError(ex, "Failed to deserialize parser service response");
			throw new InvalidOperationException("Failed to parse parser service response", ex);
		}
		catch (Exception ex)
		{
			_logger.LogError(ex, "Unexpected error while parsing code");
			throw;
		}
	}

	public async Task<ValidationResponse> ValidateAsync(string code, string language, CancellationToken cancellationToken = default)
	{
		try
		{
			_logger.LogDebug("Validating code (language: {Language}, length: {CodeLength})", language, code?.Length ?? 0);

		var request = new ParserRequest
		{
			Code = code,
			Language = language
		};

		// Добавляем JWT токен в заголовок Authorization
		var token = GetAuthToken();
		if (!string.IsNullOrEmpty(token))
		{
			_httpClient.DefaultRequestHeaders.Authorization = 
				new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);
		}

		var response = await _httpClient.PostAsJsonAsync("/api/validate", request, cancellationToken);
			
			if (!response.IsSuccessStatusCode)
			{
				_logger.LogWarning("Parser service returned error status: {StatusCode}", response.StatusCode);
				
				if (response.StatusCode == HttpStatusCode.ServiceUnavailable || 
				    response.StatusCode == HttpStatusCode.GatewayTimeout)
				{
					throw new HttpRequestException($"Parser service unavailable: {response.StatusCode}");
				}
			}

		response.EnsureSuccessStatusCode();
		
		// Читаем ответ как JsonElement для использования маппера
		var jsonResponse = await response.Content.ReadFromJsonAsync<JsonElement>(
			new JsonSerializerOptions { PropertyNameCaseInsensitive = true }, 
			cancellationToken);

		if (jsonResponse.ValueKind == JsonValueKind.Null || jsonResponse.ValueKind == JsonValueKind.Undefined)
		{
			_logger.LogError("Failed to deserialize validation response");
			throw new InvalidOperationException("Parser service returned null response");
		}

		// Используем маппер для преобразования ответа
		var result = ParserResponseMapper.MapToValidationResponse(jsonResponse);

		_logger.LogDebug("Validation completed. Valid: {Valid}, LexerErrors: {LexerCount}, ParserErrors: {ParserCount}", 
			result.Valid, result.LexerErrors.Count, result.ParserErrors.Count);
		
		return result;
		}
		catch (TaskCanceledException ex) when (ex.InnerException is TimeoutException)
		{
			_logger.LogError(ex, "Timeout while validating code");
			throw new HttpRequestException("Parser service request timed out", ex);
		}
		catch (HttpRequestException ex)
		{
			_logger.LogError(ex, "HTTP error while calling parser service");
			throw;
		}
		catch (JsonException ex)
		{
			_logger.LogError(ex, "Failed to deserialize parser service response");
			throw new InvalidOperationException("Failed to parse parser service response", ex);
		}
		catch (Exception ex)
		{
			_logger.LogError(ex, "Unexpected error while validating code");
			throw;
		}
	}

	public async Task<SimpleValidationResponse> ValidateSimpleAsync(string code, string language, CancellationToken cancellationToken = default)
	{
		try
		{
			_logger.LogDebug("Validating code (simple) (language: {Language}, length: {CodeLength})", language, code?.Length ?? 0);

			var request = new ParserRequest
			{
				Code = code,
				Language = language
			};

			// Добавляем JWT токен в заголовок Authorization
			var token = GetAuthToken();
			if (!string.IsNullOrEmpty(token))
			{
				_httpClient.DefaultRequestHeaders.Authorization = 
					new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token.Replace("Bearer ", ""));
			}

			var response = await _httpClient.PostAsJsonAsync("/api/validate/simple", request, cancellationToken);
			
			if (!response.IsSuccessStatusCode)
			{
				_logger.LogWarning("Parser service returned error status: {StatusCode}", response.StatusCode);
				
				if (response.StatusCode == HttpStatusCode.ServiceUnavailable || 
				    response.StatusCode == HttpStatusCode.GatewayTimeout)
				{
					throw new HttpRequestException($"Parser service unavailable: {response.StatusCode}");
				}
			}

			response.EnsureSuccessStatusCode();
			
			// Читаем ответ как JsonElement для использования маппера
			var jsonResponse = await response.Content.ReadFromJsonAsync<JsonElement>(
				new JsonSerializerOptions { PropertyNameCaseInsensitive = true }, 
				cancellationToken);

			if (jsonResponse.ValueKind == JsonValueKind.Null || jsonResponse.ValueKind == JsonValueKind.Undefined)
			{
				_logger.LogError("Failed to deserialize simple validation response");
				throw new InvalidOperationException("Parser service returned null response");
			}

			// Используем маппер для преобразования ответа
			var result = ParserResponseMapper.MapToSimpleValidationResponse(jsonResponse);
			
			_logger.LogDebug("Simple validation completed. Valid: {Valid}, HasErrors: {HasErrors}", 
				result.Valid, result.HasErrors);
			
			return result;
		}
		catch (TaskCanceledException ex) when (ex.InnerException is TimeoutException)
		{
			_logger.LogError(ex, "Timeout while validating code (simple)");
			throw new HttpRequestException("Parser service request timed out", ex);
		}
		catch (HttpRequestException ex)
		{
			_logger.LogError(ex, "HTTP error while calling parser service (simple)");
			throw;
		}
		catch (JsonException ex)
		{
			_logger.LogError(ex, "Failed to deserialize parser service response (simple)");
			throw new InvalidOperationException("Failed to parse parser service response", ex);
		}
		catch (Exception ex)
		{
			_logger.LogError(ex, "Error in simple validation");
			throw;
		}
	}

	public async Task<GenerateCodeResponse> GenerateCodeAsync(System.Text.Json.JsonElement representation, string language, CancellationToken cancellationToken = default)
	{
		try
		{
			_logger.LogDebug("Generating code from representation (language: {Language})", language);

			// Создаем JSON объект с representation и language
			// JsonElement можно сериализовать напрямую через JsonSerializer
			var requestJson = JsonSerializer.Serialize(new
			{
				representation = representation,
				language = language
			});

			// Добавляем JWT токен в заголовок Authorization
			var token = GetAuthToken();
			if (!string.IsNullOrEmpty(token))
			{
				_httpClient.DefaultRequestHeaders.Authorization = 
					new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);
			}

			_logger.LogInformation("Sending request to parser service at {BaseUrl}/api/generate", _httpClient.BaseAddress);
			
			var content = new StringContent(requestJson, System.Text.Encoding.UTF8, "application/json");
			var response = await _httpClient.PostAsync("/api/generate", content, cancellationToken);
			
			_logger.LogInformation("Parser service responded with status: {StatusCode}", response.StatusCode);
			
			if (!response.IsSuccessStatusCode)
			{
				_logger.LogWarning("Parser service returned error status: {StatusCode}", response.StatusCode);
				
				string errorDetails = string.Empty;
				try
				{
					var errorContent = await response.Content.ReadAsStringAsync(cancellationToken);
					if (!string.IsNullOrEmpty(errorContent))
					{
						errorDetails = errorContent;
						_logger.LogWarning("Parser service error details: {ErrorDetails}", errorDetails);
					}
				}
				catch
				{
					// Игнорируем ошибки чтения тела ответа
				}
				
				if (response.StatusCode == HttpStatusCode.ServiceUnavailable || 
				    response.StatusCode == HttpStatusCode.GatewayTimeout)
				{
					var message = $"Parser service unavailable: {response.StatusCode}";
					if (!string.IsNullOrEmpty(errorDetails))
					{
						message += $". Details: {errorDetails}";
					}
					throw new HttpRequestException(message);
				}
				
				if (!string.IsNullOrEmpty(errorDetails))
				{
					throw new HttpRequestException($"Parser service error: {response.StatusCode}. {errorDetails}");
				}
			}

			response.EnsureSuccessStatusCode();
			
			var result = await response.Content.ReadFromJsonAsync<GenerateCodeResponse>(
				new JsonSerializerOptions { PropertyNameCaseInsensitive = true }, 
				cancellationToken);

			if (result == null)
			{
				_logger.LogError("Failed to deserialize generate code response");
				throw new InvalidOperationException("Parser service returned null response");
			}

			_logger.LogDebug("Code generation completed. Success: {Success}", result.Success);
			return result;
		}
		catch (TaskCanceledException ex) when (ex.InnerException is TimeoutException)
		{
			_logger.LogError(ex, "Timeout while generating code");
			throw new HttpRequestException("Parser service request timed out", ex);
		}
		catch (HttpRequestException ex)
		{
			_logger.LogError(ex, "HTTP error while calling parser service for code generation");
			throw;
		}
		catch (JsonException ex)
		{
			_logger.LogError(ex, "Failed to deserialize parser service response");
			throw new InvalidOperationException("Failed to parse parser service response", ex);
		}
		catch (Exception ex)
		{
			_logger.LogError(ex, "Unexpected error while generating code");
			throw;
		}
	}
}

