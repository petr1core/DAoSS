namespace DAOSS.Domain.Entities;

public class Review : BaseEntity
{
	public Guid ProjectId { get; set; }
	public string TargetType { get; set; } = string.Empty;
	public Guid TargetId { get; set; }
	public string Status { get; set; } = "changes_requested"; // "approved" | "changes_requested"
	public Guid CreatedBy { get; set; }
	public int ReviewerPriority { get; set; } // Приоритет ревьюера: Owner=100, Admin=50, Reviewer=25, Participant=20, Public=10
}


