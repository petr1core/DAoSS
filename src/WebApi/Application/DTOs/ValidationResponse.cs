namespace DAOSS.Application.DTOs;

public sealed class ValidationResponse
{
	public bool Valid { get; set; }
	public List<ParserError> LexerErrors { get; set; } = new();
	public List<ParserError> ParserErrors { get; set; } = new();
}

