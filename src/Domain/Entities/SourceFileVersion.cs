namespace DAOSS.Domain.Entities;

public class SourceFileVersion : VersionedEntity
{
	public Guid SourceFileId { get; set; }
	public string Content { get; set; } = string.Empty;
	public bool IsVerified { get; set; } = false; // Версия принята и соответствует правилам ревью
}


