namespace DAOSS.Domain.Entities;

public class SourceFile : BaseEntity
{
	public Guid ProjectId { get; set; }
	public string Path { get; set; } = string.Empty;
	public Guid LanguageId { get; set; }
	public Guid? LatestVersionId { get; set; }
}


