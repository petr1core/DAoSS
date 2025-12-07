using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace DAOSS.Infrastructure.Migrations
{
    /// <inheritdoc />
    public partial class AddReviewRulesAndVerifiedStatus : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            // Добавляем колонку RequiredReviewersRules в Projects
            migrationBuilder.AddColumn<string>(
                name: "RequiredReviewersRules",
                table: "Projects",
                type: "text",
                nullable: true);

            // Добавляем колонку ReviewerPriority в Reviews
            migrationBuilder.AddColumn<int>(
                name: "ReviewerPriority",
                table: "Reviews",
                type: "integer",
                nullable: false,
                defaultValue: 10); // По умолчанию приоритет публичного пользователя

            // Добавляем колонку IsVerified в SourceFileVersions
            migrationBuilder.AddColumn<bool>(
                name: "IsVerified",
                table: "SourceFileVersions",
                type: "boolean",
                nullable: false,
                defaultValue: false);

            // Обновляем существующие записи: меняем статусы на новые
            // "open" -> "changes_requested", "closed" -> "approved", "rejected" -> "changes_requested"
            migrationBuilder.Sql(@"
                UPDATE ""Reviews""
                SET ""Status"" = CASE 
                    WHEN ""Status"" = 'closed' THEN 'approved'
                    WHEN ""Status"" = 'open' THEN 'changes_requested'
                    WHEN ""Status"" = 'rejected' THEN 'changes_requested'
                    ELSE 'changes_requested'
                END
            ");

            // Удаляем старый CHECK constraint если он существует (PostgreSQL)
            migrationBuilder.Sql(@"
                ALTER TABLE ""Reviews""
                DROP CONSTRAINT IF EXISTS ""CK_Reviews_Status"";
            ");

            // Добавляем новый CHECK constraint для статусов
            migrationBuilder.Sql(@"
                ALTER TABLE ""Reviews""
                ADD CONSTRAINT ""CK_Reviews_Status"" 
                CHECK (""Status"" IN ('approved', 'changes_requested'));
            ");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            // Удаляем CHECK constraint
            migrationBuilder.Sql(@"
                ALTER TABLE ""Reviews""
                DROP CONSTRAINT IF EXISTS ""CK_Reviews_Status"";
            ");

            // Восстанавливаем старые статусы (обратное преобразование)
            migrationBuilder.Sql(@"
                UPDATE ""Reviews""
                SET ""Status"" = CASE 
                    WHEN ""Status"" = 'approved' THEN 'closed'
                    WHEN ""Status"" = 'changes_requested' THEN 'open'
                    ELSE 'open'
                END
            ");

            // Удаляем добавленные колонки
            migrationBuilder.DropColumn(
                name: "IsVerified",
                table: "SourceFileVersions");

            migrationBuilder.DropColumn(
                name: "ReviewerPriority",
                table: "Reviews");

            migrationBuilder.DropColumn(
                name: "RequiredReviewersRules",
                table: "Projects");
        }
    }
}


