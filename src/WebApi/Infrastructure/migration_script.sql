START TRANSACTION;

ALTER TABLE "Projects" ADD "RequiredReviewersRules" text;

ALTER TABLE "Reviews" ADD "ReviewerPriority" integer NOT NULL DEFAULT 10;

ALTER TABLE "SourceFileVersions" ADD "IsVerified" boolean NOT NULL DEFAULT FALSE;

INSERT INTO "__EFMigrationsHistory" ("MigrationId", "ProductVersion")
VALUES ('20251214212816_AddReviewRulesAndVerifiedStatus', '8.0.22');

COMMIT;

