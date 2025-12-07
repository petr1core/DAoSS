namespace DAOSS.Domain.Entities;

public class Invitation : BaseEntity
{
	public Guid ProjectId { get; set; }
	public Guid InvitedUserId { get; set; }
	public Guid InvitedByUserId { get; set; }
	public string Role { get; set; } = string.Empty; // 'admin' | 'reviewer'
	public string Status { get; set; } = "pending"; // 'pending' | 'accepted' | 'rejected' | 'expired'
	public DateTime ExpiresAt { get; set; }
}

