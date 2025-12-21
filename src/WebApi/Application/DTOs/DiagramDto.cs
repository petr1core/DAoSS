using System.ComponentModel.DataAnnotations;

namespace DAOSS.Application.DTOs;

public sealed class GenerateDiagramDto
{
	[Required(ErrorMessage = "ID исходного файла обязателен")]
	public Guid SourceFileId { get; set; }

	[Required(ErrorMessage = "Код обязателен")]
	public string Code { get; set; } = string.Empty;

	[Required(ErrorMessage = "Язык программирования обязателен")]
	[RegularExpression("^(pascal|c|cpp)$", ErrorMessage = "Поддерживаемые языки: pascal, c, cpp")]
	public string Language { get; set; } = string.Empty;

	public int? StartLine { get; set; }
	public int? EndLine { get; set; }
	public string? Name { get; set; }
	public string? Message { get; set; }
}

public sealed class DiagramResponseDto
{
	public Guid Id { get; set; }
	public Guid ProjectId { get; set; }
	public Guid RegionId { get; set; }
	public string Name { get; set; } = string.Empty;
	public Guid? LatestVersionId { get; set; }
	public string Status { get; set; } = string.Empty;
	public DateTime CreatedAt { get; set; }
	public DateTime? UpdatedAt { get; set; }
}

public sealed class CreateDiagramVersionDto
{
	[Required(ErrorMessage = "JSON снимок диаграммы обязателен")]
	public string JsonSnapshot { get; set; } = string.Empty;

	public string? Message { get; set; }
}

public sealed class DiagramVersionResponseDto
{
	public Guid Id { get; set; }
	public Guid DiagramId { get; set; }
	public int VersionIndex { get; set; }
	public string JsonSnapshot { get; set; } = string.Empty;
	public Guid AuthorId { get; set; }
	public string AuthorName { get; set; } = string.Empty;
	public string Message { get; set; } = string.Empty;
	public DateTime CreatedAt { get; set; }
}

