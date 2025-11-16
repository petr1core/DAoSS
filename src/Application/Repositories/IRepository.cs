namespace DAOSS.Application.Repositories;

public interface IRepository<TEntity>
{
	System.Threading.Tasks.Task<TEntity?> GetByIdAsync(Guid id, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task AddAsync(TEntity entity, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task UpdateAsync(TEntity entity, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task DeleteAsync(Guid id, System.Threading.CancellationToken cancellationToken = default);
}
