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
        } else if (item.getItemId() == R.id.menu_my_id) {
            showMyId();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
    
    private void showMyId() {
        if (currentUserId != null && !currentUserId.isEmpty()) {
            new android.app.AlertDialog.Builder(this)
                    .setTitle("My User ID")
                    .setMessage(currentUserId)
                    .setPositiveButton("Copy", (dialog, which) -> {
                        android.content.ClipboardManager clipboard = (android.content.ClipboardManager) 
                                getSystemService(android.content.Context.CLIPBOARD_SERVICE);
                        android.content.ClipData clip = android.content.ClipData.newPlainText("User ID", currentUserId);
                        clipboard.setPrimaryClip(clip);
                        Toast.makeText(this, "User ID copied to clipboard", Toast.LENGTH_SHORT).show();
                    })
                    .setNegativeButton("Close", null)
                    .show();
        } else {
            Toast.makeText(this, "User ID not available", Toast.LENGTH_SHORT).show();
        }
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
                    ProjectAdapter adapter =
                            new ProjectAdapter(response.body());
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

