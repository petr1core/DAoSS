using System.ComponentModel.DataAnnotations;
using System.Security.Claims;
using DAOSS.Application.Interfaces;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DAOSS.WebApi.Controllers;

[ApiController]
[Route("api/[controller]")]
public class AuthController : ControllerBase
{
	private readonly IAuthService _authService;

	public AuthController(IAuthService authService)
	{
		_authService = authService;
	}

	public sealed class LoginDto
	{
		[Required]
		public string Login { get; set; } = string.Empty;
		[Required]
		public string Password { get; set; } = string.Empty;
	}

	public sealed class RegisterDto
	{
		[Required]
		public string Email { get; set; } = string.Empty;
		[Required]
		public string Password { get; set; } = string.Empty;
		public string? Name { get; set; }
		public string? Login { get; set; }
	}

	[HttpPost("login")]
	[AllowAnonymous]
	public async Task<IActionResult> Login([FromBody] LoginDto dto, CancellationToken ct)
	{
		var (success, token, message) = await _authService.LoginAsync(dto.Login, dto.Password, ct);
		if (!success) return BadRequest(new { message });
		return Ok(new { token });
	}

	[HttpPost("register")]
	[AllowAnonymous]
	public async Task<IActionResult> Register([FromBody] RegisterDto dto, CancellationToken ct)
	{
		var (success, token, message) = await _authService.RegisterAsync(dto.Email, dto.Password, dto.Name, dto.Login, ct);
		if (!success) return BadRequest(new { message });
		return Ok(new { token });
	}

	[HttpGet("me")]
	[Authorize]
	public IActionResult Me()
	{
		return Ok(new
		{
			sub = User.FindFirstValue(ClaimTypes.NameIdentifier),
			name = User.FindFirstValue(ClaimTypes.Name),
			email = User.FindFirstValue(ClaimTypes.Email)
		});
	}

	[HttpGet("validate")]
	[Authorize]
	public IActionResult Validate()
	{
		return Ok(new { isValid = true });
	}
}


