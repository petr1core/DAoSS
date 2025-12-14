using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace DAOSS.Infrastructure.Migrations
{
    /// <inheritdoc />
    public partial class SeedLanguages : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            // Фиксированные GUID для предсказуемости (генерируются один раз и не меняются)
            // Вставляем языки только если их ещё нет (по коду)
            migrationBuilder.Sql(@"
                INSERT INTO ""Languages"" (""Id"", ""Code"", ""Name"", ""VersionHint"", ""FileExtensions"", ""CommentMarkerOpen"", ""CommentMarkerClose"", ""CreatedAt"")
                SELECT 
                    '11111111-1111-1111-1111-111111111111'::uuid, 'pascal', 'Pascal', 'pas', '.pas, .pp, .p', '{', '}', NOW()
                WHERE NOT EXISTS (SELECT 1 FROM ""Languages"" WHERE ""Code"" = 'pascal');
                
                INSERT INTO ""Languages"" (""Id"", ""Code"", ""Name"", ""VersionHint"", ""FileExtensions"", ""CommentMarkerOpen"", ""CommentMarkerClose"", ""CreatedAt"")
                SELECT 
                    '22222222-2222-2222-2222-222222222222'::uuid, 'c', 'C', 'c11', '.c, .h', '/*', '*/', NOW()
                WHERE NOT EXISTS (SELECT 1 FROM ""Languages"" WHERE ""Code"" = 'c');
                
                INSERT INTO ""Languages"" (""Id"", ""Code"", ""Name"", ""VersionHint"", ""FileExtensions"", ""CommentMarkerOpen"", ""CommentMarkerClose"", ""CreatedAt"")
                SELECT 
                    '33333333-3333-3333-3333-333333333333'::uuid, 'cpp', 'C++', 'c++17', '.cpp, .hpp, .h, .cc', '/*', '*/', NOW()
                WHERE NOT EXISTS (SELECT 1 FROM ""Languages"" WHERE ""Code"" = 'cpp');
            ");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            // Удаляем seed данные
            migrationBuilder.Sql(@"
                DELETE FROM ""Languages"" 
                WHERE ""Code"" IN ('pascal', 'c', 'cpp');
            ");
        }
    }
}
