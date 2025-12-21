Слой WebApi
===========

Основной слой приложения, содержащий точку входа, контроллеры, конфигурацию и композицию всех компонентов.

Структура:
- Controllers/ — API контроллеры (AuthController, ParserController, ProjectsController, ReviewsController и др.).
- Services/ — реализации сервисов (AuthService, ParserService, ReviewService, InvitationService). Логически относятся к Infrastructure (namespace: DAOSS.Infrastructure.Services).
- Repositories/ — реализации репозиториев для работы с данными через EF Core. Логически относятся к Infrastructure (namespace: DAOSS.Infrastructure.Repositories).
- Authorization/ — политики авторизации и обработчики требований (ProjectRoleHandler, ProjectRoleRequirement).
- Program.cs — точка входа приложения, конфигурация сервисов, middleware, маршрутизация.
- appsettings.json — конфигурационные файлы приложения.

Ссылки:
- Зависит от `DAOSS.Domain`, `DAOSS.Application` и `DAOSS.Infrastructure`.
- Является точкой входа приложения.

