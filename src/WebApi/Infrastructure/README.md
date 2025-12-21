Слой Infrastructure
===================

Содержит реализацию доступа к данным, внешние интеграции и инфраструктурные детали.

Структура:
- Persistence/AppDbContext.cs — DbContext (EF Core), без бизнес-логики.
- Services/ — инфраструктурные сервисы (ParserResponseMapper и др.).
- Migrations/ — миграции базы данных EF Core.
- DependencyInjection.cs — регистрация сервисов и зависимостей.

Примечание: Реализации репозиториев и сервисов (Repositories/, Services/) физически находятся в WebApi, но логически относятся к Infrastructure слою (namespace: DAOSS.Infrastructure.Repositories, DAOSS.Infrastructure.Services).

Ссылки:
- Зависит от `DAOSS.Domain` и `DAOSS.Application`.


