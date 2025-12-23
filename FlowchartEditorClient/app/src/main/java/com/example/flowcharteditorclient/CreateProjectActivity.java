package com.example.flowcharteditorclient;

import android.content.Intent;
import android.os.Bundle;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.button.MaterialButton;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class CreateProjectActivity extends AppCompatActivity {

    private EditText etProjectName;
    private EditText etProjectDescription;
    private MaterialButton btnCreate;
    private ProjectApi projectApi;
    private AuthApi authApi;
    private String currentUserId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_create_project);

        androidx.appcompat.widget.Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setDisplayShowHomeEnabled(true);
        }

        etProjectName = findViewById(R.id.etProjectName);
        etProjectDescription = findViewById(R.id.etProjectDescription);
        btnCreate = findViewById(R.id.btnCreate);

        projectApi = ApiClient.getClient(this).create(ProjectApi.class);
        authApi = ApiClient.getClient(this).create(AuthApi.class);

        // Получаем информацию о текущем пользователе
        loadCurrentUser();

        btnCreate.setOnClickListener(v -> createProject());
    }

    private void loadCurrentUser() {
        authApi.getCurrentUser().enqueue(new Callback<UserInfo>() {
            @Override
            public void onResponse(@NonNull Call<UserInfo> call,
                                   @NonNull Response<UserInfo> response) {
                if (response.isSuccessful() && response.body() != null) {
                    currentUserId = response.body().sub;
                } else if (response.code() == 401) {
                    handleUnauthorized();
                } else {
                    Toast.makeText(CreateProjectActivity.this,
                            "Failed to get user info",
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<UserInfo> call,
                                  @NonNull Throwable t) {
                Toast.makeText(CreateProjectActivity.this,
                        "Network error",
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void createProject() {
        String name = etProjectName.getText().toString().trim();
        String description = etProjectDescription.getText().toString().trim();

        if (name.isEmpty()) {
            Toast.makeText(this,
                    "Project name cannot be empty",
                    Toast.LENGTH_SHORT).show();
            return;
        }

        if (description.isEmpty()) {
            description = "No description"; // Минимальное описание
        }

        if (currentUserId == null || currentUserId.isEmpty()) {
            Toast.makeText(this,
                    "User ID not available. Please wait...",
                    Toast.LENGTH_SHORT).show();
            return;
        }

        CreateProjectRequest request =
                new CreateProjectRequest(name, description, currentUserId);

        projectApi.createProject(request)
                .enqueue(new Callback<Project>() {
                    @Override
                    public void onResponse(@NonNull Call<Project> call,
                                           @NonNull Response<Project> response) {

                        if (response.isSuccessful()) {
                            Toast.makeText(CreateProjectActivity.this,
                                    "Project created",
                                    Toast.LENGTH_SHORT).show();
                            finish(); // возвращаемся к списку
                        } else if (response.code() == 401) {
                            handleUnauthorized();
                        } else {
                            Toast.makeText(CreateProjectActivity.this,
                                    "Creation failed",
                                    Toast.LENGTH_SHORT).show();
                        }
                    }

                    @Override
                    public void onFailure(@NonNull Call<Project> call,
                                          @NonNull Throwable t) {
                        Toast.makeText(CreateProjectActivity.this,
                                "Network error",
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
