using DAOSS.Application.DTOs;

namespace DAOSS.Application.Interfaces;

public interface IParserService
{
	Task<ParserResponse> ParseToAstAsync(string code, string language, CancellationToken cancellationToken = default);
	Task<ValidationResponse> ValidateAsync(string code, string language, CancellationToken cancellationToken = default);
	Task<SimpleValidationResponse> ValidateSimpleAsync(string code, string language, CancellationToken cancellationToken = default);
}

