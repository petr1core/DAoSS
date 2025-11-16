# DAoSS
Проектирование и архитектура программных систем

Backend (скелет) — .NET 8, 3-tier архитектура
============================================

Структура каталогов:

```
DAOSS gh/
└─ src/
   ├─ Domain/                 // Доменная модель (сущности), без реализации логики
   │  └─ Entities/            // BaseEntity, User, Project, Diagram, ...
   ├─ Application/            // Контракты (интерфейсы сервисов), DTO
   │  └─ Interfaces/          // IParserService, IDiagramService, ISyncService, IReviewService
   ├─ Infrastructure/         // Доступ к данным, EF Core, внешние интеграции
   │  ├─ Persistence/         // AppDbContext
   │  └─ Repositories/        // заглушки реализаций репозиториев
   └─ WebApi/                 // Веб-API (минимальный Program.cs)
```

Сборка и запуск (из корня `DAOSS gh`):

```bash
dotnet build DAOSS.sln
dotnet run --project src/WebApi/DAOSS.WebApi.csproj
```

Архитектура слоёв:
- Domain — не зависит от других слоёв.
- Application — зависит от Domain.
- Infrastructure — зависит от Domain и Application.
- WebApi — зависит от Application и Infrastructure.

Примечание
- Реализации сервисов и репозиториев намеренно отсутствуют (только структура).
- Сущности сведены к полям из диаграмм, без поведения.