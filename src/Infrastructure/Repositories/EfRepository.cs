using DAOSS.Application.Repositories;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class EfRepository<TEntity> : IRepository<TEntity> where TEntity : class
{
	private readonly AppDbContext _dbContext;
	private readonly DbSet<TEntity> _dbSet;

	public EfRepository(AppDbContext dbContext)
	{
		_dbContext = dbContext;
		_dbSet = _dbContext.Set<TEntity>();
	}

	public async System.Threading.Tasks.Task<TEntity?> GetByIdAsync(Guid id, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbSet.FindAsync(new object?[] { id }, cancellationToken);
	}

	public async System.Threading.Tasks.Task AddAsync(TEntity entity, System.Threading.CancellationToken cancellationToken = default)
	{
		await _dbSet.AddAsync(entity, cancellationToken);
	}

	public System.Threading.Tasks.Task UpdateAsync(TEntity entity, System.Threading.CancellationToken cancellationToken = default)
	{
		_dbSet.Update(entity);
		return System.Threading.Tasks.Task.CompletedTask;
	}

	public async System.Threading.Tasks.Task DeleteAsync(Guid id, System.Threading.CancellationToken cancellationToken = default)
	{
		var entity = await GetByIdAsync(id, cancellationToken);
		if (entity != null)
		{
			_dbSet.Remove(entity);
		}
	}
}


