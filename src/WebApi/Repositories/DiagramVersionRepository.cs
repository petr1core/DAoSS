using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class DiagramVersionRepository : EfRepository<DiagramVersion>, IDiagramVersionRepository
{
	private readonly AppDbContext _dbContext;

	public DiagramVersionRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<DiagramVersion>> GetByDiagramIdAsync(Guid diagramId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.DiagramVersions
			.Where(v => v.DiagramId == diagramId)
			.OrderBy(v => v.VersionIndex)
			.ToListAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<DiagramVersion?> GetLatestByDiagramIdAsync(Guid diagramId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.DiagramVersions
			.Where(v => v.DiagramId == diagramId)
			.OrderByDescending(v => v.VersionIndex)
			.FirstOrDefaultAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<int> GetNextVersionIndexAsync(Guid diagramId, System.Threading.CancellationToken cancellationToken = default)
	{
		var maxVersion = await _dbContext.DiagramVersions
			.Where(v => v.DiagramId == diagramId)
			.Select(v => (int?)v.VersionIndex)
			.MaxAsync(cancellationToken);

		return (maxVersion ?? 0) + 1;
	}
}

