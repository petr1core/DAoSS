using System.ComponentModel.DataAnnotations;
using DAOSS.Application.DTOs;
using DAOSS.Application.Interfaces;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DAOSS.WebApi.Controllers;

[ApiController]
[Route("api/[controller]")]
[Authorize]
public class ParserController : ControllerBase
{
	private readonly IParserService _parserService;
	private readonly ILogger<ParserController> _logger;

	public ParserController(IParserService parserService, ILogger<ParserController> logger)
	{
		_parserService = parserService;
		_logger = logger;
	}

	/// <summary>
	/// Парсит код в представление программы: SPR (Structured Program Representation) для Pascal, AST (Abstract Syntax Tree) для C/C++
	/// </summary>
	[HttpPost("parse")]
	[ProducesResponseType(typeof(ParserResponse), StatusCodes.Status200OK)]
	[ProducesResponseType(StatusCodes.Status400BadRequest)]
	[ProducesResponseType(StatusCodes.Status503ServiceUnavailable)]
	[ProducesResponseType(StatusCodes.Status504GatewayTimeout)]
	public async Task<ActionResult<ParserResponse>> Parse(
		[FromBody] ParserRequest request,
		CancellationToken cancellationToken)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		try
		{
			var result = await _parserService.ParseToAstAsync(
				request.Code,
				request.Language,
				cancellationToken);

			return Ok(result);
		}
		catch (HttpRequestException ex)
		{
			_logger.LogError(ex, "HTTP error while parsing code");
			
			if (ex.Message.Contains("timed out", StringComparison.OrdinalIgnoreCase))
			{
				return StatusCode(StatusCodes.Status504GatewayTimeout, 
					new { error = "Parser service request timed out" });
			}
			
			return StatusCode(StatusCodes.Status503ServiceUnavailable, 
				new { error = "Parser service is unavailable", details = ex.Message });
		}
		catch (Exception ex)
		{
			_logger.LogError(ex, "Unexpected error while parsing code");
			return StatusCode(StatusCodes.Status500InternalServerError, 
				new { error = "An unexpected error occurred" });
		}
	}

	/// <summary>
	/// Валидирует синтаксис кода с детальными ошибками
	/// </summary>
	[HttpPost("validate")]
	[ProducesResponseType(typeof(ValidationResponse), StatusCodes.Status200OK)]
	[ProducesResponseType(StatusCodes.Status400BadRequest)]
	[ProducesResponseType(StatusCodes.Status503ServiceUnavailable)]
	[ProducesResponseType(StatusCodes.Status504GatewayTimeout)]
	public async Task<ActionResult<ValidationResponse>> Validate(
		[FromBody] ParserRequest request,
		CancellationToken cancellationToken)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		try
		{
			var result = await _parserService.ValidateAsync(
				request.Code,
				request.Language,
				cancellationToken);

			return Ok(result);
		}
		catch (HttpRequestException ex)
		{
			_logger.LogError(ex, "HTTP error while validating code");
			
			if (ex.Message.Contains("timed out", StringComparison.OrdinalIgnoreCase))
			{
				return StatusCode(StatusCodes.Status504GatewayTimeout, 
					new { error = "Parser service request timed out" });
			}
			
			return StatusCode(StatusCodes.Status503ServiceUnavailable, 
				new { error = "Parser service is unavailable", details = ex.Message });
		}
		catch (Exception ex)
		{
			_logger.LogError(ex, "Unexpected error while validating code");
			return StatusCode(StatusCodes.Status500InternalServerError, 
				new { error = "An unexpected error occurred" });
		}
	}

	/// <summary>
	/// Упрощенная валидация кода для быстрой проверки валидности
	/// </summary>
	[HttpPost("validate/simple")]
	[ProducesResponseType(typeof(SimpleValidationResponse), StatusCodes.Status200OK)]
	[ProducesResponseType(StatusCodes.Status400BadRequest)]
	[ProducesResponseType(StatusCodes.Status503ServiceUnavailable)]
	[ProducesResponseType(StatusCodes.Status504GatewayTimeout)]
	public async Task<ActionResult<SimpleValidationResponse>> ValidateSimple(
		[FromBody] ParserRequest request,
		CancellationToken cancellationToken)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		try
		{
			var result = await _parserService.ValidateSimpleAsync(
				request.Code,
				request.Language,
				cancellationToken);

			return Ok(result);
		}
		catch (HttpRequestException ex)
		{
			_logger.LogError(ex, "HTTP error while validating code (simple)");
			
			if (ex.Message.Contains("timed out", StringComparison.OrdinalIgnoreCase))
			{
				return StatusCode(StatusCodes.Status504GatewayTimeout, 
					new { error = "Parser service request timed out" });
			}
			
			return StatusCode(StatusCodes.Status503ServiceUnavailable, 
				new { error = "Parser service is unavailable", details = ex.Message });
		}
		catch (Exception ex)
		{
			_logger.LogError(ex, "Unexpected error while validating code (simple)");
			return StatusCode(StatusCodes.Status500InternalServerError, 
				new { error = "An unexpected error occurred" });
		}
	}
}




