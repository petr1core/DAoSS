namespace DAOSS.Domain.Entities;

public class CodeRegion : BaseEntity
{
	public Guid SourceFileId { get; set; }
	public int StartLine { get; set; }
	public int EndLine { get; set; }
	public string TagName { get; set; } = string.Empty;
	public string RegionType { get; set; } = string.Empty;
}


