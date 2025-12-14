-- Удаляем запись о миграции из истории (если она есть)
DELETE FROM "__EFMigrationsHistory"
WHERE
    "MigrationId" = '20251214212816_AddReviewRulesAndVerifiedStatus';

-- Добавляем колонки напрямую
ALTER TABLE "Projects"
ADD COLUMN IF NOT EXISTS "RequiredReviewersRules" text;

ALTER TABLE "Reviews"
ADD COLUMN IF NOT EXISTS "ReviewerPriority" integer NOT NULL DEFAULT 10;

ALTER TABLE "SourceFileVersions"
ADD COLUMN IF NOT EXISTS "IsVerified" boolean NOT NULL DEFAULT FALSE;

-- Если колонка уже была добавлена, но значение по умолчанию не установлено для существующих записей
UPDATE "Reviews"
SET
    "ReviewerPriority" = 10
WHERE
    "ReviewerPriority" IS NULL;

UPDATE "SourceFileVersions"
SET
    "IsVerified" = FALSE
WHERE
    "IsVerified" IS NULL;

-- Возвращаем запись в историю миграций
INSERT INTO
    "__EFMigrationsHistory" (
        "MigrationId",
        "ProductVersion"
    )
VALUES (
        '20251214212816_AddReviewRulesAndVerifiedStatus',
        '8.0.22'
    ) ON CONFLICT ("MigrationId") DO NOTHING;
