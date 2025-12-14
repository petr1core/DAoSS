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
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
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
