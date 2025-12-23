package com.example.flowcharteditorclient;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.floatingactionbutton.ExtendedFloatingActionButton;
import android.view.Menu;
import android.view.MenuItem;

import java.util.List;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class ProjectListActivity extends AppCompatActivity {

    private RecyclerView recyclerView;
    private ExtendedFloatingActionButton fabAdd;
    private ProjectApi projectApi;
    private AuthApi authApi;
    private String currentUserId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_project_list);

        recyclerView = findViewById(R.id.recyclerProjects);
        fabAdd = findViewById(R.id.fabAdd);

        recyclerView.setLayoutManager(new LinearLayoutManager(this));

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        projectApi = ApiClient.getClient(this).create(ProjectApi.class);
        authApi = ApiClient.getClient(this).create(AuthApi.class);

        fabAdd.setOnClickListener(v -> {
            Intent intent = new Intent(ProjectListActivity.this,
                    CreateProjectActivity.class);
            startActivity(intent);
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_project_list, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.menu_logout) {
            logout();
            return true;
        } else if (item.getItemId() == R.id.menu_invitations) {
            Intent intent = new Intent(ProjectListActivity.this, InvitationsListActivity.class);
            startActivity(intent);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void logout() {
        TokenManager.clear(this);
        Intent intent = new Intent(this, LoginActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(intent);
        finish();
    }

    @Override
    protected void onResume() {
        super.onResume();
        loadCurrentUserAndProjects();
    }

    private void loadCurrentUserAndProjects() {
        // Сначала получаем информацию о текущем пользователе
        authApi.getCurrentUser().enqueue(new Callback<UserInfo>() {
            @Override
            public void onResponse(@NonNull Call<UserInfo> call,
                    @NonNull Response<UserInfo> response) {
                if (response.isSuccessful() && response.body() != null) {
                    currentUserId = response.body().sub;
                    loadProjects();
                } else if (response.code() == 401) {
                    logout();
                } else {
                    Toast.makeText(ProjectListActivity.this,
                            "Failed to get user info",
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<UserInfo> call,
                    @NonNull Throwable t) {
                Toast.makeText(ProjectListActivity.this,
                        "Network error",
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void loadProjects() {
        if (currentUserId == null || currentUserId.isEmpty()) {
            Toast.makeText(this, "User ID not available", Toast.LENGTH_SHORT).show();
            return;
        }

        projectApi.getProjects(currentUserId).enqueue(new Callback<List<Project>>() {

            @Override
            public void onResponse(@NonNull Call<List<Project>> call,
                    @NonNull Response<List<Project>> response) {

                if (response.isSuccessful() && response.body() != null) {
                    List<Project> projects = response.body();
                    // Логируем для отладки
                    android.util.Log.d("ProjectListActivity", "Received " + projects.size() + " projects");
                    for (Project project : projects) {
                        android.util.Log.d("ProjectListActivity",
                                "Project - name: '" + project.name + "', id: " + project.id +
                                        ", description: '" + project.description + "'");
                        if (project.name == null || project.name.isEmpty()) {
                            android.util.Log.e("ProjectListActivity",
                                    "Project name is null or empty for project with id: " + project.id);
                        }
                        if (project.id == null || project.id.isEmpty()) {
                            android.util.Log.e("ProjectListActivity",
                                    "Project ID is null or empty for: " + project.name);
                        }
                    }
                    ProjectAdapter adapter = new ProjectAdapter(projects);
                    recyclerView.setAdapter(adapter);
                } else if (response.code() == 401) {
                    logout();
                } else {
                    Toast.makeText(ProjectListActivity.this,
                            "Failed to load projects",
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<List<Project>> call,
                    @NonNull Throwable t) {
                Toast.makeText(ProjectListActivity.this,
                        "Network error",
                        Toast.LENGTH_SHORT).show();
            }
        });
    }
}
