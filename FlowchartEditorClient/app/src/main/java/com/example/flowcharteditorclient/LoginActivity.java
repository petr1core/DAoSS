package com.example.flowcharteditorclient;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.textfield.TextInputLayout;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class LoginActivity extends AppCompatActivity {
    private EditText etLogin, etPassword;
    private TextInputLayout loginInputLayout, passwordInputLayout;
    private MaterialButton btnLogin;
    private TextView tvRegister;

    private AuthApi authApi;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        loginInputLayout = findViewById(R.id.loginInputLayout);
        passwordInputLayout = findViewById(R.id.passwordInputLayout);

        etLogin = findViewById(R.id.etLogin);
        etPassword = findViewById(R.id.etPassword);
        btnLogin = findViewById(R.id.btnLogin);
        tvRegister = findViewById(R.id.tvRegister);

        authApi = ApiClient.getClient(this).create(AuthApi.class);


        btnLogin.setOnClickListener(v -> login());
        tvRegister.setOnClickListener(v -> {
            startActivity(new Intent(this, RegisterActivity.class));
        });

    }

    private void login() {
        String login = etLogin.getText().toString().trim();
        String password = etPassword.getText().toString().trim();

        // Очищаем предыдущие ошибки
        loginInputLayout.setError(null);
        passwordInputLayout.setError(null);

        boolean hasErrors = false;

        // Валидация логина
        if (login.isEmpty()) {
            loginInputLayout.setError("Логин не может быть пустым");
            hasErrors = true;
        }

        // Валидация пароля
        if (password.isEmpty()) {
            passwordInputLayout.setError("Пароль не может быть пустым");
            hasErrors = true;
        }

        if (hasErrors) {
            return;
        }

        LoginRequest request = new LoginRequest(login, password);

        authApi.login(request).enqueue(new Callback<AuthResponse>() {
            @Override
            public void onResponse(@NonNull Call<AuthResponse> call, @NonNull Response<AuthResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                    saveToken(response.body().token);
                    goToProjects();
                } else {
                    String errorMessage = "Неверные учетные данные";
                    if (response.code() == 401) {
                        errorMessage = "Неверный логин или пароль";
                    } else if (response.code() == 400) {
                        errorMessage = "Некорректные данные";
                    }
                    Toast.makeText(LoginActivity.this,
                            errorMessage,
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<AuthResponse> call, @NonNull Throwable t) {
                Toast.makeText(LoginActivity.this,
                        "Ошибка сети: " + t.getMessage(),
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void saveToken(String token) {
        TokenManager.saveToken(this, token);
    }


    private void goToProjects() {
        startActivity(new Intent(this, ProjectListActivity.class));
        finish();
    }
}
