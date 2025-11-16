using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class DiagramRepository : EfRepository<Diagram>, IDiagramRepository
{
	private readonly AppDbContext _dbContext;

	public DiagramRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Diagram>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Diagrams
			.Where(d => d.ProjectId == projectId)
			.ToListAsync(cancellationToken);
	}
}


