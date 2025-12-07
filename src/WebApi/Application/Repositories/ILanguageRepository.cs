namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface ILanguageRepository : IRepository<Language>
{
	System.Threading.Tasks.Task<Language?> GetByCodeAsync(string code, System.Threading.CancellationToken cancellationToken = default);
}


