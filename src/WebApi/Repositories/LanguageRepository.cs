using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class LanguageRepository : EfRepository<Language>, ILanguageRepository
{
	private readonly AppDbContext _dbContext;

	public LanguageRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<Language?> GetByCodeAsync(string code, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Languages.FirstOrDefaultAsync(l => l.Code == code, cancellationToken);
	}
}


