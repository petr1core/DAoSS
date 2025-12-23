namespace DAOSS.Application.DTOs;

public sealed class UserResponseDto
{
	public Guid Id { get; set; }
	public string Name { get; set; } = string.Empty;
	public string Email { get; set; } = string.Empty;
	public string Login { get; set; } = string.Empty;
}

