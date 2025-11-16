namespace DAOSS.Domain.Entities;

public class Project : BaseEntity
{
	public string Name { get; set; } = string.Empty;
	public string Description { get; set; } = string.Empty;
	public Guid OwnerId { get; set; }
	public Guid DefaultLanguageId { get; set; }
	public string Visibility { get; set; } = "private";
}


