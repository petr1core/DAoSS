namespace DAOSS.Domain.Entities;

public class DiagramVersion : VersionedEntity
{
	public Guid DiagramId { get; set; }
	public string JsonSnapshot { get; set; } = string.Empty;
}


