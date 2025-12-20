package com.example.flowcharteditorclient;

import android.os.Bundle;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.button.MaterialButton;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class RegisterActivity extends AppCompatActivity {

    private EditText etName, etLogin, etEmail, etPassword;
    private MaterialButton btnRegister;

    private AuthApi authApi;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_register);

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

        if (name.isEmpty() || login.isEmpty() || email.isEmpty() || password.isEmpty()) {
            Toast.makeText(this, "All fields are required", Toast.LENGTH_SHORT).show();
            return;
        }

        RegisterRequest request =
                new RegisterRequest(email, password, name, login);

        authApi.register(request).enqueue(new Callback<AuthResponse>() {
            @Override
            public void onResponse(@NonNull Call<AuthResponse> call,
                                   @NonNull Response<AuthResponse> response) {

                if (response.isSuccessful() && response.body() != null) {
                    // Сохраняем токен после успешной регистрации
                    TokenManager.saveToken(RegisterActivity.this, response.body().token);
                    Toast.makeText(RegisterActivity.this,
                            "Registration successful",
                            Toast.LENGTH_SHORT).show();
                    finish(); // возвращаемся на Login
                } else {
                    Toast.makeText(RegisterActivity.this,
                            "Registration failed",
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<AuthResponse> call,
                                  @NonNull Throwable t) {
                Toast.makeText(RegisterActivity.this,
                        "Network error",
                        Toast.LENGTH_SHORT).show();
            }
        });
    }
}

