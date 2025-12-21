namespace DAOSS.Domain.Entities;

public class ReviewItem : BaseEntity
{
	public Guid ReviewId { get; set; }
	public string Kind { get; set; } = string.Empty; // 'comment' | 'issue'
	public string AnchorType { get; set; } = string.Empty; // 'code' | 'diagram'
	public string AnchorRef { get; set; } = string.Empty;
	public string Body { get; set; } = string.Empty;
	public string Status { get; set; } = "open"; // 'open' | 'resolved'
	public Guid CreatedBy { get; set; }
}


