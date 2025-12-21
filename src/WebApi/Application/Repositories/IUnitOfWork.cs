namespace DAOSS.Application.Repositories;

public interface IUnitOfWork
{
	System.Threading.Tasks.Task<int> SaveChangesAsync(System.Threading.CancellationToken cancellationToken = default);
}


