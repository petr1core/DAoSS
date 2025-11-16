namespace DAOSS.Domain.Entities;

public class AuditLog : BaseEntity
{
	public Guid? ProjectId { get; set; }
	public Guid? ActorId { get; set; }
	public string Action { get; set; } = string.Empty;
	public string TargetType { get; set; } = string.Empty;
	public Guid? TargetId { get; set; }
	public DateTime At { get; set; } = DateTime.UtcNow;
	public string? MetaJson { get; set; }
}


