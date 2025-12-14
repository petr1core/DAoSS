-- Добавляем колонки, которые отсутствуют в базе данных

-- Добавляем колонку RequiredReviewersRules в Projects (если её нет)
ALTER TABLE "Projects"
ADD COLUMN IF NOT EXISTS "RequiredReviewersRules" text;

-- Добавляем колонку ReviewerPriority в Reviews (если её нет)
ALTER TABLE "Reviews"
ADD COLUMN IF NOT EXISTS "ReviewerPriority" integer NOT NULL DEFAULT 10;

-- Добавляем колонку IsVerified в SourceFileVersions (если её нет)
ALTER TABLE "SourceFileVersions"
ADD COLUMN IF NOT EXISTS "IsVerified" boolean NOT NULL DEFAULT FALSE;
