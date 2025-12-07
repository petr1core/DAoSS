using DAOSS.Application.Repositories;
using DAOSS.Infrastructure.Persistence;

namespace DAOSS.Infrastructure.Repositories;

public class UnitOfWork : IUnitOfWork
{
	private readonly AppDbContext _dbContext;

	public UnitOfWork(AppDbContext dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<int> SaveChangesAsync(System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.SaveChangesAsync(cancellationToken);
	}
}


