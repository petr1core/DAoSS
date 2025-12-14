using DAOSS.Infrastructure.Persistence;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.WebApi.Controllers;

[ApiController]
[Route("api/[controller]")]
public class LanguagesController : ControllerBase
{
	private readonly AppDbContext _dbContext;

	public LanguagesController(AppDbContext dbContext)
	{
		_dbContext = dbContext;
	}

	[HttpGet]
	[AllowAnonymous] // Список языков доступен всем (нужен для создания проектов)
	public async Task<ActionResult<IEnumerable<object>>> GetAll(CancellationToken ct)
	{
		var languages = await _dbContext.Languages.ToListAsync(ct);
		var result = languages.Select(l => new
		{
			id = l.Id,
			code = l.Code,
			name = l.Name,
			versionHint = l.VersionHint,
			fileExtensions = l.FileExtensions
		}).ToList();

		return Ok(result);
	}
}
