namespace DAOSS.Application.DTOs;

public sealed class ParserError
{
	public int Position { get; set; }
	public int Line { get; set; }
	public int Column { get; set; }
	public string Type { get; set; } = string.Empty;
	public string Message { get; set; } = string.Empty;
	public string? Value { get; set; }
	public string? Expected { get; set; }
	public string? Found { get; set; }
}

