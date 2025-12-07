Слой Application
================

Содержит бизнес-логику приложения, интерфейсы сервисов и репозиториев, а также DTOs для передачи данных.

Структура:
- DTOs/ — объекты передачи данных (Data Transfer Objects) для API запросов и ответов.
- Interfaces/ — интерфейсы сервисов и бизнес-логики (IAuthService, IParserService, IReviewService и др.).
- Repositories/ — интерфейсы репозиториев для работы с данными (IProjectRepository, IUserRepository и др.).

Ссылки:
- Зависит от `DAOSS.Domain`.
- Используется слоями `DAOSS.Infrastructure` и `DAOSS.WebApi`.

