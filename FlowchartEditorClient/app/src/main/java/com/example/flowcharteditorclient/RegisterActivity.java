package com.example.flowcharteditorclient;

import android.os.Bundle;
import android.util.Patterns;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.textfield.TextInputLayout;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class RegisterActivity extends AppCompatActivity {

    private EditText etName, etLogin, etEmail, etPassword;
    private TextInputLayout nameInputLayout, loginInputLayout, emailInputLayout, passwordInputLayout;
    private MaterialButton btnRegister;

    private AuthApi authApi;

    private static final int MIN_PASSWORD_LENGTH = 6;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_register);

        nameInputLayout = findViewById(R.id.nameInputLayout);
        loginInputLayout = findViewById(R.id.loginInputLayout);
        emailInputLayout = findViewById(R.id.emailInputLayout);
        passwordInputLayout = findViewById(R.id.passwordInputLayout);

        etName = findViewById(R.id.etName);
        etLogin = findViewById(R.id.etLogin);
        etEmail = findViewById(R.id.etEmail);
        etPassword = findViewById(R.id.etPassword);
        btnRegister = findViewById(R.id.btnRegister);

        authApi = ApiClient.getClient(this).create(AuthApi.class);

        btnRegister.setOnClickListener(v -> register());
    }

    private void register() {
        String name = etName.getText().toString().trim();
        String login = etLogin.getText().toString().trim();
        String email = etEmail.getText().toString().trim();
        String password = etPassword.getText().toString().trim();

        // Очищаем предыдущие ошибки
        nameInputLayout.setError(null);
        loginInputLayout.setError(null);
        emailInputLayout.setError(null);
        passwordInputLayout.setError(null);

        boolean hasErrors = false;

        // Валидация email (обязательное поле)
        if (email.isEmpty()) {
            emailInputLayout.setError("Email не может быть пустым");
            hasErrors = true;
        } else if (!Patterns.EMAIL_ADDRESS.matcher(email).matches()) {
            emailInputLayout.setError("Введите корректный email адрес");
            hasErrors = true;
        }

        // Валидация пароля (обязательное поле)
        if (password.isEmpty()) {
            passwordInputLayout.setError("Пароль не может быть пустым");
            hasErrors = true;
        } else if (password.length() < MIN_PASSWORD_LENGTH) {
            passwordInputLayout.setError("Пароль должен содержать минимум " + MIN_PASSWORD_LENGTH + " символов");
            hasErrors = true;
        }

        // Валидация имени (опциональное, но если заполнено - проверяем)
        if (!name.isEmpty() && name.length() < 1) {
            nameInputLayout.setError("Имя не может быть пустым");
            hasErrors = true;
        }

        // Валидация логина (опциональное, но если заполнено - проверяем)
        if (!login.isEmpty() && login.length() < 1) {
            loginInputLayout.setError("Логин не может быть пустым");
            hasErrors = true;
        }

        if (hasErrors) {
            return;
        }

        RegisterRequest request = new RegisterRequest(email, password, 
                name.isEmpty() ? null : name, 
                login.isEmpty() ? null : login);

        authApi.register(request).enqueue(new Callback<AuthResponse>() {
            @Override
            public void onResponse(@NonNull Call<AuthResponse> call,
                                   @NonNull Response<AuthResponse> response) {

                if (response.isSuccessful() && response.body() != null) {
                    // Сохраняем токен после успешной регистрации
                    TokenManager.saveToken(RegisterActivity.this, response.body().token);
                    Toast.makeText(RegisterActivity.this,
                            "Регистрация успешна",
                            Toast.LENGTH_SHORT).show();
                    finish(); // возвращаемся на Login
                } else {
                    String errorMessage = "Ошибка регистрации";
                    if (response.code() == 400) {
                        errorMessage = "Некорректные данные";
                    } else if (response.code() == 409) {
                        errorMessage = "Пользователь с таким email уже существует";
                    }
                    Toast.makeText(RegisterActivity.this,
                            errorMessage,
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<AuthResponse> call,
                                  @NonNull Throwable t) {
                Toast.makeText(RegisterActivity.this,
                        "Ошибка сети: " + t.getMessage(),
                        Toast.LENGTH_SHORT).show();
            }
        });
    }
}

