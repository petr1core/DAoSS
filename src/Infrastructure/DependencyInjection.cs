using DAOSS.Application.Repositories;
using DAOSS.Infrastructure.Repositories;
using DAOSS.Application.Interfaces;
using DAOSS.Infrastructure.Services;
using Microsoft.Extensions.DependencyInjection;

namespace DAOSS.Infrastructure;

public static class DependencyInjection
{
	public static IServiceCollection AddInfrastructure(this IServiceCollection services)
	{
		services.AddScoped<IProjectRepository, ProjectRepository>();
		services.AddScoped<IDiagramRepository, DiagramRepository>();
		services.AddScoped<IUserRepository, UserRepository>();
		services.AddScoped<ISourceFileRepository, SourceFileRepository>();
		services.AddScoped<IReviewRepository, ReviewRepository>();
		services.AddScoped<ICommentRepository, CommentRepository>();
		services.AddScoped<ILanguageRepository, LanguageRepository>();
		services.AddScoped<IProjectMemberRepository, ProjectMemberRepository>();
		services.AddScoped<IUnitOfWork, UnitOfWork>();
		services.AddScoped<IAuthService, AuthService>();
		return services;
	}
}


