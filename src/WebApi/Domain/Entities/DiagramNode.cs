namespace DAOSS.Domain.Entities;

public class DiagramNode : BaseEntity
{
	public string Uid { get; set; } = string.Empty;
	public string Type { get; set; } = string.Empty;
	public double X { get; set; }
	public double Y { get; set; }
	public double Width { get; set; }
	public double Height { get; set; }
	public string Text { get; set; } = string.Empty;
}


