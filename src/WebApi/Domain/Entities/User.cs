namespace DAOSS.Domain.Entities;

public class User : BaseEntity
{
	public string Login { get; set; } = string.Empty;
	public string Email { get; set; } = string.Empty;
	public string Name { get; set; } = string.Empty;
	public string PasswordHash { get; set; } = string.Empty;
	public string AuthProvider { get; set; } = string.Empty;
	public string AuthSubject { get; set; } = string.Empty;
	public string AuthMetadata { get; set; } = string.Empty;
	public bool NotifyEnabled { get; set; }
	public bool IsActive { get; set; }
	public DateTime? LastLoginAt { get; set; }
}


