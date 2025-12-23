using System.ComponentModel.DataAnnotations;
using System.Text.Json;

namespace DAOSS.Application.DTOs;

public sealed class GenerateCodeRequest
{
	[Required(ErrorMessage = "Representation не может быть пустым")]
	public JsonElement Representation { get; set; }

	[Required(ErrorMessage = "Язык программирования не указан")]
	[RegularExpression("^(pascal|c|cpp)$", ErrorMessage = "Поддерживаемые языки: pascal, c, cpp")]
	public string Language { get; set; } = "pascal";
}

