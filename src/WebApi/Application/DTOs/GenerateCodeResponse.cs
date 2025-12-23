namespace DAOSS.Application.DTOs;

public sealed class GenerateCodeResponse
{
	public bool Success { get; set; }
	public string? Code { get; set; }
	public string? Language { get; set; }
	public string? Error { get; set; }
}

