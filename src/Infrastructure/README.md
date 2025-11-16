Слой Infrastructure
===================

Содержит реализацию доступа к данным, внешние интеграции и инфраструктурные детали.

Структура:
- Persistence/AppDbContext.cs — DbContext (EF Core), без бизнес-логики.
- Repositories/ — место для реализаций репозиториев (заготовка).
- Migrations/ — будущие миграции БД.

Ссылки:
- Зависит от `DAOSS.Domain` и `DAOSS.Application`.


