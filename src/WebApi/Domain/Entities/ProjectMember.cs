namespace DAOSS.Domain.Entities;

public class ProjectMember : BaseEntity
{
	public Guid ProjectId { get; set; }
	public Guid UserId { get; set; }
	public string Role { get; set; } = string.Empty; // 'owner' | 'admin' | 'reviewer'
	public Guid? AssignedBy { get; set; }
}


