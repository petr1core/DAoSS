using System.IdentityModel.Tokens.Jwt;
using System.Security.Claims;
using System.Text;
using BCrypt.Net;
using DAOSS.Application.Interfaces;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Configuration;
using Microsoft.IdentityModel.Tokens;

namespace DAOSS.Infrastructure.Services;

public class AuthService : IAuthService
{
	private readonly AppDbContext _dbContext;
	private readonly IConfiguration _configuration;

	public AuthService(AppDbContext dbContext, IConfiguration configuration)
	{
		_dbContext = dbContext;
		_configuration = configuration;
	}

	public async System.Threading.Tasks.Task<(bool success, string? token, string message)> LoginAsync(string loginOrEmail, string password, System.Threading.CancellationToken cancellationToken = default)
	{
		var user = await _dbContext.Users
			.FirstOrDefaultAsync(u => u.Email == loginOrEmail || u.Name == loginOrEmail || u.Login == loginOrEmail, cancellationToken);

		if (user is null)
		{
			return (false, null, "Пользователь не найден");
		}

		if (!BCrypt.Net.BCrypt.Verify(password, user.PasswordHash))
		{
			return (false, null, "Неверный логин или пароль");
		}

		user.LastLoginAt = DateTime.UtcNow;
		await _dbContext.SaveChangesAsync(cancellationToken);

		var token = GenerateJwtToken(user.Id, user.Email, user.Name);
		return (true, token, "OK");
	}

	public async System.Threading.Tasks.Task<(bool success, string? token, string message)> RegisterAsync(string email, string password, string? name = null, string? login = null, System.Threading.CancellationToken cancellationToken = default)
	{
		email = email?.Trim() ?? string.Empty;
		login = string.IsNullOrWhiteSpace(login) ? email : login!.Trim();

		var exists = await _dbContext.Users.AnyAsync(u => u.Email == email || u.Login == login, cancellationToken);
		if (exists)
		{
			return (false, null, "Пользователь с таким email или логином уже существует");
		}

		var user = new User
		{
			Id = Guid.NewGuid(),
			Email = email,
			Login = login!,
			Name = name ?? login!,
			PasswordHash = BCrypt.Net.BCrypt.HashPassword(password),
			IsActive = true,
			NotifyEnabled = true,
			CreatedAt = DateTime.UtcNow
		};
		_dbContext.Users.Add(user);
		await _dbContext.SaveChangesAsync(cancellationToken);

		var token = GenerateJwtToken(user.Id, user.Email, user.Name);
		return (true, token, "OK");
	}

	private string GenerateJwtToken(Guid userId, string email, string name)
	{
		var jwtSection = _configuration.GetSection("Jwt");
		var key = jwtSection.GetValue<string>("Key") ?? throw new InvalidOperationException("Jwt:Key not set");
		var issuer = jwtSection.GetValue<string>("Issuer") ?? "daoss";
		var audience = jwtSection.GetValue<string>("Audience") ?? "daoss-client";
		var expiryMinutes = jwtSection.GetValue<int?>("ExpiryInMinutes") ?? 60;

		var securityKey = new SymmetricSecurityKey(Encoding.UTF8.GetBytes(key));
		var credentials = new SigningCredentials(securityKey, SecurityAlgorithms.HmacSha256);

		var claims = new List<Claim>
		{
			new(JwtRegisteredClaimNames.Sub, userId.ToString()),
			new(ClaimTypes.NameIdentifier, userId.ToString()),
			new(ClaimTypes.Email, email ?? string.Empty),
			new(ClaimTypes.Name, name ?? string.Empty),
			new(JwtRegisteredClaimNames.Iat, DateTimeOffset.UtcNow.ToUnixTimeSeconds().ToString(), ClaimValueTypes.Integer64)
		};

		var token = new JwtSecurityToken(
			issuer: issuer,
			audience: audience,
			claims: claims,
			notBefore: DateTime.UtcNow,
			expires: DateTime.UtcNow.AddMinutes(expiryMinutes),
			signingCredentials: credentials
		);

		return new JwtSecurityTokenHandler().WriteToken(token);
	}
}


