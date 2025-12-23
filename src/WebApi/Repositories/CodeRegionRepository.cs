using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class CodeRegionRepository : EfRepository<CodeRegion>, ICodeRegionRepository
{
	private readonly AppDbContext _dbContext;

	public CodeRegionRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<CodeRegion>> GetBySourceFileIdAsync(Guid sourceFileId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Set<CodeRegion>()
			.Where(cr => cr.SourceFileId == sourceFileId)
			.ToListAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<CodeRegion?> GetByDiagramIdAsync(Guid diagramId, System.Threading.CancellationToken cancellationToken = default)
	{
		var diagram = await _dbContext.Diagrams
			.FirstOrDefaultAsync(d => d.Id == diagramId, cancellationToken);
		
		if (diagram == null)
			return null;

		return await _dbContext.Set<CodeRegion>()
			.FirstOrDefaultAsync(cr => cr.Id == diagram.RegionId, cancellationToken);
	}
}

