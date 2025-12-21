namespace DAOSS.Domain.Entities;

public class Diagram : BaseEntity
{
	public Guid ProjectId { get; set; }
	public Guid RegionId { get; set; }
	public string Name { get; set; } = string.Empty;
	public Guid? LatestVersionId { get; set; }
	public string Status { get; set; } = "active";
}


