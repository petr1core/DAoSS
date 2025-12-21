namespace DAOSS.Application.DTOs;

public sealed class SimpleValidationResponse
{
	public bool Valid { get; set; }
	public bool HasErrors { get; set; }
	public int LexerErrorsCount { get; set; }
	public int ParserErrorsCount { get; set; }
}

