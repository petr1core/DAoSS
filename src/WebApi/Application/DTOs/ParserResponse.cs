using System.Text.Json;

namespace DAOSS.Application.DTOs;

public sealed class ParserResponse
{
	public bool Success { get; set; }
	public JsonElement? Representation { get; set; }
	public string? RepresentationType { get; set; }
	public string? Error { get; set; }
	public List<ParserError> LexerErrors { get; set; } = new();
	public List<ParserError> ParserErrors { get; set; } = new();
}
