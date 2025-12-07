namespace DAOSS.Domain.Entities;

public class Language : BaseEntity
{
	public string Code { get; set; } = string.Empty;
	public string Name { get; set; } = string.Empty;
	public string VersionHint { get; set; } = string.Empty;
	public string FileExtensions { get; set; } = string.Empty;
	public string CommentMarkerOpen { get; set; } = string.Empty;
	public string CommentMarkerClose { get; set; } = string.Empty;
}


