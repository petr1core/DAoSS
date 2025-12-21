Слой Domain
===========

Содержит доменные сущности и бизнес-модели приложения. Это ядро системы, не зависящее от внешних библиотек и фреймворков.

Структура:
- Entities/ — доменные сущности:
  * BaseEntity — базовый класс для всех сущностей (Id, CreatedAt, UpdatedAt)
  * VersionedEntity — базовый класс для версионируемых сущностей
  * Project, User, ProjectMember — основные сущности проекта
  * SourceFile, SourceFileVersion — файлы исходного кода
  * Review, ReviewItem, ReviewRule — система ревью кода
  * Diagram, DiagramNode, DiagramEdge, DiagramVersion — диаграммы
  * Comment, CodeRegion — комментарии и регионы кода
  * Invitation — приглашения в проекты
  * Language — поддерживаемые языки программирования
  * AuditLog — логи аудита

Ссылки:
- Не имеет зависимостей от других слоев проекта.
- Используется всеми остальными слоями.

