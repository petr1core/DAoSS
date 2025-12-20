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

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class LoginActivity extends AppCompatActivity {
    private EditText etLogin, etPassword;
    private MaterialButton btnLogin;
    private TextView tvRegister;

    private AuthApi authApi;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

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
        String login = etLogin.getText().toString();
        String password = etPassword.getText().toString();

        LoginRequest request = new LoginRequest(login, password);

        authApi.login(request).enqueue(new Callback<AuthResponse>() {
            @Override
            public void onResponse(@NonNull Call<AuthResponse> call, @NonNull Response<AuthResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                    saveToken(response.body().token);
                    goToProjects();
                } else {
                    Toast.makeText(LoginActivity.this,
                            "Invalid credentials",
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<AuthResponse> call, @NonNull Throwable t) {
                Toast.makeText(LoginActivity.this,
                        "Network error",
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
