namespace DAOSS.Domain.Entities;

public class Comment : BaseEntity
{
	public Guid ReviewItemId { get; set; }
	public string Body { get; set; } = string.Empty;
	public Guid AuthorId { get; set; }
}


