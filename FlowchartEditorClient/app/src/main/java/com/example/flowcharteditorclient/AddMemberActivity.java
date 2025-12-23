package com.example.flowcharteditorclient;

import android.content.Intent;
import android.os.Bundle;
import android.util.Patterns;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.textfield.TextInputLayout;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class AddMemberActivity extends AppCompatActivity {

    private EditText etEmail;
    private TextInputLayout emailInputLayout;
    private Spinner spinnerRole;
    private MaterialButton btnAdd;
    private InvitationsApi invitationsApi;
    private String projectId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_add_member);

        projectId = getIntent().getStringExtra("PROJECT_ID");
        android.util.Log.d("AddMemberActivity", "Received PROJECT_ID: " + projectId);
        if (projectId == null || projectId.isEmpty()) {
            Toast.makeText(this, "Project ID not provided", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        // Убираем возможные пробелы и проверяем формат
        projectId = projectId.trim();
        android.util.Log.d("AddMemberActivity", "Trimmed PROJECT_ID: " + projectId);

        androidx.appcompat.widget.Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setDisplayShowHomeEnabled(true);
            getSupportActionBar().setTitle("Пригласить участника");
        }

        emailInputLayout = findViewById(R.id.emailInputLayout);
        etEmail = findViewById(R.id.etEmail);
        spinnerRole = findViewById(R.id.spinnerRole);
        btnAdd = findViewById(R.id.btnAdd);

        // Настраиваем Spinner с ролями (без owner)
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
                this,
                R.array.member_roles_add,
                android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerRole.setAdapter(adapter);

        invitationsApi = ApiClient.getClient(this).create(InvitationsApi.class);

        btnAdd.setOnClickListener(v -> sendInvitation());
    }

    private void sendInvitation() {
        String email = etEmail.getText().toString().trim();

        // Очищаем предыдущие ошибки
        emailInputLayout.setError(null);

        // Валидация email
        if (email.isEmpty()) {
            emailInputLayout.setError("Email не может быть пустым");
            return;
        }

        if (!Patterns.EMAIL_ADDRESS.matcher(email).matches()) {
            emailInputLayout.setError("Введите корректный email адрес");
            return;
        }

        // Получаем выбранную роль из Spinner
        String role = spinnerRole.getSelectedItem().toString().toLowerCase();

        // Сначала находим пользователя по email
        android.util.Log.d("AddMemberActivity", "Looking up user by email: " + email);
        // Retrofit автоматически кодирует Path параметры

        invitationsApi.getUserByEmail(email)
                .enqueue(new Callback<UserResponse>() {
                    @Override
                    public void onResponse(@NonNull Call<UserResponse> call,
                            @NonNull Response<UserResponse> response) {
                        android.util.Log.d("AddMemberActivity", "getUserByEmail response code: " + response.code());

                        if (response.isSuccessful() && response.body() != null) {
                            UserResponse user = response.body();
                            android.util.Log.d("AddMemberActivity", "User found - Id: " + user.id + ", Name: " + user.name + ", Email: " + user.email);
                            
                            // Проверяем, что ID пользователя не пустой
                            if (user.id == null || user.id.isEmpty()) {
                                emailInputLayout.setError("ID пользователя не найден");
                                android.util.Log.e("AddMemberActivity", "User ID is null or empty");
                                return;
                            }
                            
                            // Отправляем приглашение
                            sendInvitationRequest(user.id.trim(), role);
                        } else if (response.code() == 404) {
                            emailInputLayout.setError("Пользователь с таким email не найден");
                        } else if (response.code() == 401) {
                            handleUnauthorized();
                        } else {
                            String errorMessage = "Не удалось найти пользователя (код: " + response.code() + ")";
                            try {
                                if (response.errorBody() != null) {
                                    String errorBody = response.errorBody().string();
                                    android.util.Log.e("AddMemberActivity", "Error body: " + errorBody);
                                }
                            } catch (Exception e) {
                                android.util.Log.e("AddMemberActivity", "Error reading error body", e);
                            }
                            Toast.makeText(AddMemberActivity.this,
                                    errorMessage,
                                    Toast.LENGTH_SHORT).show();
                        }
                    }

                    @Override
                    public void onFailure(@NonNull Call<UserResponse> call,
                            @NonNull Throwable t) {
                        Toast.makeText(AddMemberActivity.this,
                                "Ошибка сети: " + t.getMessage(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
    }

    private void sendInvitationRequest(String userId, String role) {
        // Проверяем, что userId не пустой
        if (userId == null || userId.isEmpty()) {
            Toast.makeText(this, "Ошибка: ID пользователя не найден", Toast.LENGTH_SHORT).show();
            android.util.Log.e("AddMemberActivity", "UserId is null or empty");
            return;
        }
        
        SendInvitationRequest request = new SendInvitationRequest(userId.trim(), role);

        // Логируем для отладки
        android.util.Log.d("AddMemberActivity",
                "Sending invitation - ProjectId: " + projectId + ", UserId: " + userId + ", Role: " + role);
        android.util.Log.d("AddMemberActivity",
                "Request object - invitedUserId: " + request.invitedUserId + ", role: " + request.role);

        invitationsApi.sendInvitation(projectId, request)
                .enqueue(new Callback<InvitationResponse>() {
                    @Override
                    public void onResponse(@NonNull Call<InvitationResponse> call,
                            @NonNull Response<InvitationResponse> response) {

                        android.util.Log.d("AddMemberActivity", "Response code: " + response.code());

                        if (response.isSuccessful()) {
                            Toast.makeText(AddMemberActivity.this,
                                    "Приглашение отправлено успешно",
                                    Toast.LENGTH_SHORT).show();
                            finish(); // возвращаемся к списку участников
                        } else if (response.code() == 401) {
                            handleUnauthorized();
                        } else if (response.code() == 404) {
                            // Проверяем тело ответа для более точного сообщения
                            String errorMessage = "Проект не найден";
                            try {
                                if (response.errorBody() != null) {
                                    String errorBody = response.errorBody().string();
                                    android.util.Log.e("AddMemberActivity", "Error body: " + errorBody);
                                    if (errorBody.contains("User not found")) {
                                        errorMessage = "Пользователь не найден";
                                    } else if (errorBody.contains("Project")) {
                                        errorMessage = "Проект не найден";
                                    }
                                }
                            } catch (Exception e) {
                                android.util.Log.e("AddMemberActivity", "Error reading error body", e);
                            }
                            Toast.makeText(AddMemberActivity.this,
                                    errorMessage,
                                    Toast.LENGTH_SHORT).show();
                        } else if (response.code() == 409) {
                            Toast.makeText(AddMemberActivity.this,
                                    "Приглашение уже существует",
                                    Toast.LENGTH_SHORT).show();
                        } else {
                            String errorMessage = "Не удалось отправить приглашение (код: " + response.code() + ")";
                            try {
                                if (response.errorBody() != null) {
                                    String errorBody = response.errorBody().string();
                                    android.util.Log.e("AddMemberActivity", "Error body: " + errorBody);
                                }
                            } catch (Exception e) {
                                android.util.Log.e("AddMemberActivity", "Error reading error body", e);
                            }
                            Toast.makeText(AddMemberActivity.this,
                                    errorMessage,
                                    Toast.LENGTH_SHORT).show();
                        }
                    }

                    @Override
                    public void onFailure(@NonNull Call<InvitationResponse> call,
                            @NonNull Throwable t) {
                        Toast.makeText(AddMemberActivity.this,
                                "Ошибка сети: " + t.getMessage(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
    }

    private void handleUnauthorized() {
        TokenManager.clear(this);
        android.content.Intent intent = new android.content.Intent(this, LoginActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(intent);
        finish();
    }

    @Override
    public boolean onSupportNavigateUp() {
        finish();
        return true;
    }
}
