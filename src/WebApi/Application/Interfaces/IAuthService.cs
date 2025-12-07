namespace DAOSS.Application.Interfaces;

public interface IAuthService
{
	System.Threading.Tasks.Task<(bool success, string? token, string message)> LoginAsync(string loginOrEmail, string password, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<(bool success, string? token, string message)> RegisterAsync(string email, string password, string? name = null, string? login = null, System.Threading.CancellationToken cancellationToken = default);
}


