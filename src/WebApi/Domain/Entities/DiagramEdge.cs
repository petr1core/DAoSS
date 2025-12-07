namespace DAOSS.Domain.Entities;

public class DiagramEdge : BaseEntity
{
	public string Uid { get; set; } = string.Empty;
	public string FromNodeUid { get; set; } = string.Empty;
	public string ToNodeUid { get; set; } = string.Empty;
	public string Label { get; set; } = string.Empty;
	public string Condition { get; set; } = string.Empty;
}


