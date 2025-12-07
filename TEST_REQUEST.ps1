# PowerShell скрипт для тестирования парсера
# Использование: .\TEST_REQUEST.ps1 {-TOKEN "jwt auth token"} {-Endpoint "parse" || "validate" || "validate/simple"} {-Url "endpoint"}

param(
    [Parameter(Mandatory=$false)]
    [string]$Token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiI5YjA1YTcyNy1iNjBmLTRlNzItOTQ0Yy0yNDFjNTgyYjQ1NGYiLCJodHRwOi8vc2NoZW1hcy54bWxzb2FwLm9yZy93cy8yMDA1LzA1L2lkZW50aXR5L2NsYWltcy9uYW1laWRlbnRpZmllciI6IjliMDVhNzI3LWI2MGYtNGU3Mi05NDRjLTI0MWM1ODJiNDU0ZiIsImh0dHA6Ly9zY2hlbWFzLnhtbHNvYXAub3JnL3dzLzIwMDUvMDUvaWRlbnRpdHkvY2xhaW1zL2VtYWlsYWRkcmVzcyI6InN0cmluZyIsImh0dHA6Ly9zY2hlbWFzLnhtbHNvYXAub3JnL3dzLzIwMDUvMDUvaWRlbnRpdHkvY2xhaW1zL25hbWUiOiJzdHJpbmciLCJpYXQiOjE3NjUwMzI0MDQsIm5iZiI6MTc2NTAzMjQwNCwiZXhwIjoxNzY1MDM2MDA0LCJpc3MiOiJkYW9zcy1kZXYiLCJhdWQiOiJkYW9zcy1jbGllbnQifQ.1i9NycBwwAzow_CsBlMLkBle2vxXgMjl2bdpfzr9P80",
    
    [Parameter(Mandatory=$false)]
    [string]$Url = "https://localhost:7143/api/parser/parse",
    
    [Parameter(Mandatory=$false)]
    [ValidateSet("parse", "validate", "validate/simple")]
    [string]$Endpoint = "parse"
)

# Код Pascal в одной строке с экранированными кавычками
$pascalCode = @"
program qq;
function AddNumbers ( a , b : integer ) : integer ;
begin
   AddNumbers := a + b;
end;
procedure GreetUser ( name : string ) ;
begin
   Writeln ( 'Hello,' , name , '!' );
end;
const
   PI: real = 3.1415926;
var
   num1, num2, i: integer;
   Res, d: real;
   res2: string;
begin
   Read ( Res ) ;
   Writeln ('From Read ' , Res ) ;
   num1 := 12 div 2;
   num1 := AddNumbers ( PI , PI);
   if 5 mod 3 > 0 then begin
       Writeln ('Yes,if 1 ');
       Writeln ('Yes,if 2 ');
   end
   case num1 of
   begin
       1 , 2 , 3 :
       begin
           Writeln ( 'Switch works 1' );
       end;
       4 , 5 :
       begin
           Writeln ( 'Switch works 2' );
       end;
       else :
       begin
           Writeln ( 'Switch no works' );
       end;
   end;
   if 5 mod 3 > 0 then begin
       Writeln ('Yes,if 1 ');
       Writeln ('Yes,if 2 ');
   end
   else begin
       Writeln ('No,else 1 ');
       Writeln ('No,else 2 ');
   end;
   if 5 mod 3 > 0 then begin
       Writeln ('Yes,if 1 ');
       Writeln ('Yes,if 2 ');
   end
   else begin
       Writeln ('No,else 1 ');
       Writeln ('No,else 2 ');
   end;
   res2 := 'Hello world' ;
   num1 := 2 ;
   Writeln ('From table ' , num1);
   if PI <> num1 then begin
       Writeln( 'Pim' );
       if PI <> num1 then begin
           Writeln ('Pam');
       end
   end
   else begin
       Writeln ('Pum');
   end;
   if PI <> num1 then begin
       Writeln( 'Pim' );
       if 5 mod 3 > 0 then begin
           Writeln ('Yes,if 1 ');
           Writeln ('Yes,if 2 ');
       end
       else begin
           Writeln ('No,else 1 ');
           Writeln ('No,else 2 ');
       end;
   end
   else begin
       Writeln ('Pum');
   end;
   for i := 1 to 8 do begin
       Write ( '3' ) ;
   end;
   while num1 < 6 do begin
       Write('Yes');
       num1 := num1 + 1;
   end;
   if 5 mod 3 > 0 then begin
       Writeln ('Yes,if 1 ');
       Writeln ('Yes,if 2 ');
   end
   else begin
       Writeln ('No,else 1 ');
       Writeln ('No,else 2 ');
   end;
   repeat begin
       Write('3');
       num1 := num1 + 1;
   end;
   until num1 < 7 ;
   num1 := num1 + 3;
end.
"@

# Формирование JSON тела запроса
$body = @{
    code = $pascalCode
    language = "pascal"
} | ConvertTo-Json

# Заголовки
$headers = @{
    "Authorization" = "Bearer $token"
    "Content-Type" = "application/json"
    "accept" = "application/json"
}

# Отправка запроса
try {
    $response = Invoke-RestMethod -Uri $url -Method Post -Headers $headers -Body $body
    $response | ConvertTo-Json -Depth 10
} catch {
    Write-Host "Ошибка: $_" -ForegroundColor Red
    if ($_.Exception.Response) {
        $reader = New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream())
        $responseBody = $reader.ReadToEnd()
        Write-Host "Response: $responseBody" -ForegroundColor Yellow
    }
}

