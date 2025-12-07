namespace DAOSS.Domain.Entities;

public abstract class VersionedEntity : BaseEntity
{
	public int VersionIndex { get; set; }
	public Guid AuthorId { get; set; }
	public string AuthorName { get; set; } = string.Empty;
	public string Message { get; set; } = string.Empty;
}


