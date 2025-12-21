namespace DAOSS.Domain.Entities;

/// <summary>
/// Правило ревью для проекта. Определяет необходимое количество ревью от определенной роли.
/// </summary>
public class ReviewRule
{
	/// <summary>
	/// Роль ревьюера: "Owner", "Admin", или "Reviewer"
	/// </summary>
	public string Role { get; set; } = string.Empty;

	/// <summary>
	/// Необходимое количество ревью от этой роли. Если не указано, считается 1.
	/// </summary>
	public int? Count { get; set; }
}


