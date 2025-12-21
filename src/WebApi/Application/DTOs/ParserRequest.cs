using System.ComponentModel.DataAnnotations;

namespace DAOSS.Application.DTOs;

public sealed class ParserRequest
{
	[Required(ErrorMessage = "Код не может быть пустым")]
	public string Code { get; set; } = string.Empty;

	[Required(ErrorMessage = "Язык программирования не указан")]
	[RegularExpression("^(pascal|c|cpp)$", ErrorMessage = "Поддерживаемые языки: pascal, c, cpp")]
	public string Language { get; set; } = "pascal";
}
