using System.ComponentModel.DataAnnotations;

namespace DAOSS.Application.DTOs;

public sealed class CreateSourceFileDto
{
	[Required(ErrorMessage = "Путь к файлу обязателен")]
	public string Path { get; set; } = string.Empty;

	[Required(ErrorMessage = "Содержимое файла обязательно")]
	public string Content { get; set; } = string.Empty;

	public string? Message { get; set; }
}

public sealed class SourceFileResponseDto
{
	public Guid Id { get; set; }
	public Guid ProjectId { get; set; }
	public string Path { get; set; } = string.Empty;
	public Guid LanguageId { get; set; }
	public Guid? LatestVersionId { get; set; }
	public DateTime CreatedAt { get; set; }
}

public sealed class CreateSourceFileVersionDto
{
	[Required(ErrorMessage = "Содержимое файла обязательно")]
	public string Content { get; set; } = string.Empty;

	public string? Message { get; set; }
}

public sealed class SourceFileVersionResponseDto
{
	public Guid Id { get; set; }
	public Guid SourceFileId { get; set; }
	public int VersionIndex { get; set; }
	public string Content { get; set; } = string.Empty;
	public Guid AuthorId { get; set; }
	public string AuthorName { get; set; } = string.Empty;
	public string Message { get; set; } = string.Empty;
	public DateTime CreatedAt { get; set; }
	public bool IsVerified { get; set; }
}

