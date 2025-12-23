package com.example.flowcharteditorclient;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.floatingactionbutton.ExtendedFloatingActionButton;

import java.util.List;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class ProjectMembersActivity extends AppCompatActivity {

    private RecyclerView recyclerView;
    private ExtendedFloatingActionButton fabAdd;
    private ProjectMembersApi membersApi;
    private String projectId;
    private ProjectMemberAdapter adapter;
    private android.view.View emptyView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_project_members);

        projectId = getIntent().getStringExtra("PROJECT_ID");
        if (projectId == null || projectId.isEmpty()) {
            Toast.makeText(this, "Project ID not provided", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        recyclerView = findViewById(R.id.recyclerMembers);
        fabAdd = findViewById(R.id.fabAddMember);
        emptyView = findViewById(R.id.emptyView);

        androidx.appcompat.widget.Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setDisplayShowHomeEnabled(true);
        }

        recyclerView.setLayoutManager(new LinearLayoutManager(this));

        membersApi = ApiClient.getClient(this).create(ProjectMembersApi.class);

        fabAdd.setOnClickListener(v -> {
            Intent intent = new Intent(ProjectMembersActivity.this,
                    AddMemberActivity.class);
            intent.putExtra("PROJECT_ID", projectId);
            startActivity(intent);
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        loadMembers();
    }

    private void loadMembers() {
        membersApi.getMembers(projectId).enqueue(new Callback<List<ProjectMember>>() {
            @Override
            public void onResponse(@NonNull Call<List<ProjectMember>> call,
                                   @NonNull Response<List<ProjectMember>> response) {

                if (response.isSuccessful() && response.body() != null) {
                    List<ProjectMember> members = response.body();
                    updateUI(members);
                } else if (response.code() == 401) {
                    handleUnauthorized();
                } else {
                    Toast.makeText(ProjectMembersActivity.this,
                            "Failed to load members",
                            Toast.LENGTH_SHORT).show();
                    // Показываем empty view при ошибке загрузки
                    updateUI(java.util.Collections.emptyList());
                }
            }

            @Override
            public void onFailure(@NonNull Call<List<ProjectMember>> call,
                                  @NonNull Throwable t) {
                Toast.makeText(ProjectMembersActivity.this,
                        "Network error",
                        Toast.LENGTH_SHORT).show();
                // Показываем empty view при ошибке сети
                updateUI(java.util.Collections.emptyList());
            }
        });
    }
    
    private void updateUI(List<ProjectMember> members) {
        // Если в списке только владелец (роль owner или не задана) — считаем, что участников нет
        if (members.size() == 1) {
            ProjectMember only = members.get(0);
            if (only.role == null || only.role.isEmpty() || only.role.equalsIgnoreCase("owner")) {
                members = java.util.Collections.emptyList();
            }
        }

        if (members.isEmpty()) {
            recyclerView.setVisibility(android.view.View.GONE);
            emptyView.setVisibility(android.view.View.VISIBLE);
        } else {
            recyclerView.setVisibility(android.view.View.VISIBLE);
            emptyView.setVisibility(android.view.View.GONE);
            adapter = new ProjectMemberAdapter(members, membersApi, projectId);
            adapter.setOnMemberRemovedListener(() -> {
                // Обновляем список после удаления
                loadMembers();
            });
            recyclerView.setAdapter(adapter);
        }
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

