using DAOSS.Application.DTOs;
using DAOSS.Application.Repositories;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DAOSS.WebApi.Controllers;

[ApiController]
[Route("api/[controller]")]
[Authorize]
public class UsersController : ControllerBase
{
	private readonly IUserRepository _userRepository;

	public UsersController(IUserRepository userRepository)
	{
		_userRepository = userRepository;
	}

	[HttpGet("{id:guid}")]
	public async Task<ActionResult<UserResponseDto>> GetById(Guid id, CancellationToken ct)
	{
		var user = await _userRepository.GetByIdAsync(id, ct);
		if (user == null)
		{
			return NotFound(new { message = "User not found" });
		}

		var response = new UserResponseDto
		{
			Id = user.Id,
			Name = user.Name,
			Email = user.Email,
			Login = user.Login
		};

		return Ok(response);
	}

	[HttpGet("by-email/{email}")]
	public async Task<ActionResult<UserResponseDto>> GetByEmail(string email, CancellationToken ct)
	{
		var user = await _userRepository.GetByEmailAsync(email, ct);
		if (user == null)
		{
			return NotFound(new { message = "User not found" });
		}

		var response = new UserResponseDto
		{
			Id = user.Id,
			Name = user.Name,
			Email = user.Email,
			Login = user.Login
		};

		return Ok(response);
	}
}

