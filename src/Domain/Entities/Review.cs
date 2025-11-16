namespace DAOSS.Domain.Entities;

public class Review : BaseEntity
{
	public Guid ProjectId { get; set; }
	public string TargetType { get; set; } = string.Empty;
	public Guid TargetId { get; set; }
	public string Status { get; set; } = "open";
	public Guid CreatedBy { get; set; }
}


