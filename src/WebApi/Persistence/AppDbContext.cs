using DAOSS.Domain.Entities;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Persistence;

public class AppDbContext : DbContext
{
	public AppDbContext(DbContextOptions<AppDbContext> options) : base(options)
	{
	}

	public DbSet<User> Users => Set<User>();
	public DbSet<Project> Projects => Set<Project>();
	public DbSet<Diagram> Diagrams => Set<Diagram>();
	public DbSet<SourceFile> SourceFiles => Set<SourceFile>();
	public DbSet<Comment> Comments => Set<Comment>();
	public DbSet<Review> Reviews => Set<Review>();
	public DbSet<Language> Languages => Set<Language>();
	public DbSet<ProjectMember> ProjectMembers => Set<ProjectMember>();
	public DbSet<Invitation> Invitations => Set<Invitation>();
	public DbSet<SourceFileVersion> SourceFileVersions => Set<SourceFileVersion>();
	public DbSet<DiagramVersion> DiagramVersions => Set<DiagramVersion>();
	public DbSet<ReviewItem> ReviewItems => Set<ReviewItem>();
	public DbSet<AuditLog> AuditLog => Set<AuditLog>();
	public DbSet<CodeRegion> CodeRegions => Set<CodeRegion>();
}


