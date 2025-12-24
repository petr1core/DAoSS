package com.example.flowcharteditorclient;

import android.content.Intent;
import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.button.MaterialButton;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class EditProjectActivity extends AppCompatActivity {

    private EditText etProjectName;
    private EditText etProjectDescription;
    private Spinner spinnerVisibility;
    private MaterialButton btnSave;
    private ProjectApi projectApi;
    private AuthApi authApi;
    private String projectId;
    private String currentUserId;
    private Project currentProject;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_edit_project);

        projectId = getIntent().getStringExtra("PROJECT_ID");
        if (projectId == null || projectId.isEmpty()) {
            Toast.makeText(this, "Project ID not provided", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        androidx.appcompat.widget.Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setDisplayShowHomeEnabled(true);
        }

        etProjectName = findViewById(R.id.etProjectName);
        etProjectDescription = findViewById(R.id.etProjectDescription);
        spinnerVisibility = findViewById(R.id.spinnerVisibility);
        btnSave = findViewById(R.id.btnSave);

        // Настраиваем Spinner с видимостью
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
                this,
                R.array.visibility_options,
                android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerVisibility.setAdapter(adapter);

        projectApi = ApiClient.getClient(this).create(ProjectApi.class);
        authApi = ApiClient.getClient(this).create(AuthApi.class);

        // Получаем информацию о текущем пользователе и проекте
        loadCurrentUser();
        loadProject();
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
                }
            }

            @Override
            public void onFailure(@NonNull Call<UserInfo> call,
                                  @NonNull Throwable t) {
                Toast.makeText(EditProjectActivity.this,
                        "Network error",
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void loadProject() {
        projectApi.getProjectById(projectId).enqueue(new Callback<Project>() {
            @Override
            public void onResponse(@NonNull Call<Project> call,
                                   @NonNull Response<Project> response) {
                if (response.isSuccessful() && response.body() != null) {
                    currentProject = response.body();
                    populateFields();
                } else if (response.code() == 401) {
                    handleUnauthorized();
                } else {
                    Toast.makeText(EditProjectActivity.this,
                            "Не удалось загрузить проект",
                            Toast.LENGTH_SHORT).show();
                    finish();
                }
            }

            @Override
            public void onFailure(@NonNull Call<Project> call,
                                  @NonNull Throwable t) {
                Toast.makeText(EditProjectActivity.this,
                        "Ошибка сети",
                        Toast.LENGTH_SHORT).show();
                finish();
            }
        });
    }

    private void populateFields() {
        if (currentProject == null) return;

        etProjectName.setText(currentProject.name);
        
        if (currentProject.description != null && !currentProject.description.isEmpty() && 
            !currentProject.description.equals("No description")) {
            etProjectDescription.setText(currentProject.description);
        }

        // Устанавливаем видимость
        String visibility = currentProject.visibility != null ? currentProject.visibility : "private";
        for (int i = 0; i < spinnerVisibility.getCount(); i++) {
            if (spinnerVisibility.getItemAtPosition(i).toString().equalsIgnoreCase(visibility)) {
                spinnerVisibility.setSelection(i);
                break;
            }
        }

        btnSave.setOnClickListener(v -> updateProject());
    }

    private void updateProject() {
        String name = etProjectName.getText().toString().trim();
        String description = etProjectDescription.getText().toString().trim();
        String visibility = spinnerVisibility.getSelectedItem().toString().toLowerCase();

        if (name.isEmpty()) {
            Toast.makeText(this,
                    "Название проекта не может быть пустым",
                    Toast.LENGTH_SHORT).show();
            return;
        }

        if (description.isEmpty()) {
            description = "No description";
        }

        if (currentUserId == null || currentUserId.isEmpty()) {
            Toast.makeText(this,
                    "User ID not available. Please wait...",
                    Toast.LENGTH_SHORT).show();
            return;
        }

        CreateProjectRequest request = new CreateProjectRequest(name, description, currentUserId);
        request.visibility = visibility;

        projectApi.updateProject(projectId, request)
                .enqueue(new Callback<Void>() {
                    @Override
                    public void onResponse(@NonNull Call<Void> call,
                                           @NonNull Response<Void> response) {

                        if (response.isSuccessful()) {
                            Toast.makeText(EditProjectActivity.this,
                                    "Проект обновлен",
                                    Toast.LENGTH_SHORT).show();
                            finish();
                        } else if (response.code() == 401) {
                            handleUnauthorized();
                        } else {
                            Toast.makeText(EditProjectActivity.this,
                                    "Не удалось обновить проект",
                                    Toast.LENGTH_SHORT).show();
                        }
                    }

                    @Override
                    public void onFailure(@NonNull Call<Void> call,
                                          @NonNull Throwable t) {
                        Toast.makeText(EditProjectActivity.this,
                                "Ошибка сети",
                                Toast.LENGTH_SHORT).show();
                    }
                });
    }

    private void handleUnauthorized() {
        TokenManager.clear(this);
        Intent intent = new Intent(this, LoginActivity.class);
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



