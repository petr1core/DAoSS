using Microsoft.AspNetCore.Builder;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.EntityFrameworkCore;
using DAOSS.Infrastructure.Persistence;
using DAOSS.Infrastructure;
using Microsoft.AspNetCore.Authentication.JwtBearer;
using Microsoft.IdentityModel.Tokens;
using System.Text;
using Microsoft.OpenApi.Models;
using Microsoft.AspNetCore.Authorization;
using DAOSS.WebApi.Authorization;

var builder = WebApplication.CreateBuilder(args);

// Services registration (скелет, без реализации)
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen(c =>
{
	c.SwaggerDoc("v1", new OpenApiInfo { Title = "DAOSS API", Version = "v1" });
	var securityScheme = new OpenApiSecurityScheme
	{
		Name = "Authorization",
		Description = "Введите JWT токен в формате: Bearer {token}",
		In = ParameterLocation.Header,
		Type = SecuritySchemeType.Http,
		Scheme = "bearer",
		BearerFormat = "JWT"
	};
	c.AddSecurityDefinition("Bearer", securityScheme);
	c.AddSecurityRequirement(new OpenApiSecurityRequirement
	{
		{
			new OpenApiSecurityScheme
			{
				Reference = new OpenApiReference { Type = ReferenceType.SecurityScheme, Id = "Bearer" }
			},
			new string[] {}
		}
	});
});
builder.Services.AddControllers();

// Database: PostgreSQL (Env → appsettings fallback)
var configuration = builder.Configuration;
var host = Environment.GetEnvironmentVariable("POSTGRES_HOST") ?? "localhost";
var port = Environment.GetEnvironmentVariable("POSTGRES_PORT") ?? "5432";
var database = Environment.GetEnvironmentVariable("POSTGRES_DB") ?? "postgres";
var username = Environment.GetEnvironmentVariable("POSTGRES_USER") ?? "postgres";
var password = Environment.GetEnvironmentVariable("POSTGRES_PASSWORD")
	?? configuration.GetValue<string>("ConnectionStrings:DefaultPassword")
	?? string.Empty;

var connFromConfig = configuration.GetConnectionString("Default");
var connectionString = string.IsNullOrWhiteSpace(connFromConfig)
	? $"Host={host};Port={port};Database={database};Username={username};Password={password}"
	: connFromConfig;

builder.Services.AddDbContext<AppDbContext>(options =>
	options.UseNpgsql(connectionString));
builder.Services.AddInfrastructure();
// AuthN/AuthZ (JWT)
var jwtSection = builder.Configuration.GetSection("Jwt");
var key = jwtSection.GetValue<string>("Key") ?? "dev_secret_key_change_me_please_1234567890";
var issuer = jwtSection.GetValue<string>("Issuer") ?? "daoss";
var audience = jwtSection.GetValue<string>("Audience") ?? "daoss-client";

builder.Services
	.AddAuthentication(JwtBearerDefaults.AuthenticationScheme)
	.AddJwtBearer(options =>
	{
		options.TokenValidationParameters = new TokenValidationParameters
		{
			ValidateIssuer = true,
			ValidateAudience = true,
			ValidateLifetime = true,
			ValidateIssuerSigningKey = true,
			ValidIssuer = issuer,
			ValidAudience = audience,
			IssuerSigningKey = new SymmetricSecurityKey(Encoding.UTF8.GetBytes(key)),
			ClockSkew = TimeSpan.FromSeconds(30)
		};
	});
builder.Services.AddAuthorization(options =>
{
	// ProjectRead: для публичных проектов - любой авторизованный (public), для приватных - только участники
	options.AddPolicy("ProjectRead", p => p.Requirements.Add(new ProjectRoleRequirement("public", "reviewer", "admin", "owner")));
	// ProjectWrite: для публичных проектов - любой авторизованный (public), для приватных - только участники
	options.AddPolicy("ProjectWrite", p => p.Requirements.Add(new ProjectRoleRequirement("public", "reviewer", "admin", "owner")));
	// ProjectAdmin: только admin и owner (не зависит от видимости)
	options.AddPolicy("ProjectAdmin", p => p.Requirements.Add(new ProjectRoleRequirement("admin", "owner")));
});
builder.Services.AddScoped<IAuthorizationHandler, ProjectRoleHandler>();

var app = builder.Build();

if (app.Environment.IsDevelopment())
{
	app.UseSwagger();
	app.UseSwaggerUI();
}

app.UseAuthentication();
app.UseAuthorization();

app.MapGet("/", () => "Бэкенд сервиса запущен").WithName("Root");
app.MapControllers();

app.Run();


