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
    private com.google.android.material.button.MaterialButton btnViewInvitations;
    private ProjectMembersApi membersApi;
    private String projectId;
    private ProjectMemberAdapter adapter;
    private android.view.View emptyView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_project_members);

        projectId = getIntent().getStringExtra("PROJECT_ID");
        if (projectId != null) {
            projectId = projectId.trim();
        }
        if (projectId == null || projectId.isEmpty()) {
            android.util.Log.e("ProjectMembersActivity",
                    "PROJECT_ID is null or empty. Intent extras: " + getIntent().getExtras());
            Toast.makeText(this, "Project ID not provided", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }
        android.util.Log.d("ProjectMembersActivity", "Received PROJECT_ID: " + projectId);

        recyclerView = findViewById(R.id.recyclerMembers);
        fabAdd = findViewById(R.id.fabAddMember);
        btnViewInvitations = findViewById(R.id.btnViewInvitations);
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

        btnViewInvitations.setOnClickListener(v -> {
            Intent intent = new Intent(ProjectMembersActivity.this,
                    ProjectInvitationsActivity.class);
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
                    // Логируем для отладки
                    android.util.Log.d("ProjectMembersActivity", "Received " + members.size() + " members");
                    for (int i = 0; i < members.size(); i++) {
                        ProjectMember member = members.get(i);
                        android.util.Log.d("ProjectMembersActivity",
                                "Member[" + i + "] - UserId: '" + member.userId +
                                        "', Role: '" + member.role +
                                        "', ProjectId: '" + member.projectId + "'");
                        if (member.userId == null) {
                            android.util.Log.e("ProjectMembersActivity", "Member[" + i + "] has null userId!");
                        }
                    }
                    updateUI(members);
                } else {
                    android.util.Log.e("ProjectMembersActivity", "Failed to load members: " + response.code());
                    if (response.errorBody() != null) {
                        try {
                            String errorBody = response.errorBody().string();
                            android.util.Log.e("ProjectMembersActivity", "Error body: " + errorBody);
                        } catch (Exception e) {
                            android.util.Log.e("ProjectMembersActivity", "Error reading error body", e);
                        }
                    }
                    if (response.code() == 401) {
                        handleUnauthorized();
                    } else {
                        Toast.makeText(ProjectMembersActivity.this,
                                "Failed to load members",
                                Toast.LENGTH_SHORT).show();
                        // Показываем empty view при ошибке загрузки
                        updateUI(java.util.Collections.emptyList());
                    }
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
        android.util.Log.d("ProjectMembersActivity", "updateUI called with " + members.size() + " members");

        // Фильтруем участников - исключаем только owner, но показываем всех остальных
        List<ProjectMember> filteredMembers = new java.util.ArrayList<>();
        for (ProjectMember member : members) {
            // Показываем всех участников, включая owner (но можно скрыть, если нужно)
            // Исключаем только если роль явно owner и это единственный участник
            if (member.userId != null && !member.userId.isEmpty()) {
                filteredMembers.add(member);
                android.util.Log.d("ProjectMembersActivity",
                        "Added member: " + member.userId + ", role: " + member.role);
            } else {
                android.util.Log.w("ProjectMembersActivity", "Skipping member with null/empty userId");
            }
        }

        if (filteredMembers.isEmpty()) {
            android.util.Log.d("ProjectMembersActivity", "No members to display, showing empty view");
            recyclerView.setVisibility(android.view.View.GONE);
            emptyView.setVisibility(android.view.View.VISIBLE);
            // Очищаем адаптер
            if (adapter != null) {
                recyclerView.setAdapter(null);
                adapter = null;
            }
        } else {
            android.util.Log.d("ProjectMembersActivity", "Displaying " + filteredMembers.size() + " members");
            recyclerView.setVisibility(android.view.View.VISIBLE);
            emptyView.setVisibility(android.view.View.GONE);
            adapter = new ProjectMemberAdapter(filteredMembers, membersApi, projectId, this);
            adapter.setOnMemberRemovedListener(() -> {
                // Обновляем список после удаления
                android.util.Log.d("ProjectMembersActivity", "Member removed, reloading list");
                loadMembers();
            });
            adapter.setOnRoleUpdatedListener(() -> {
                // Обновляем список после изменения роли
                android.util.Log.d("ProjectMembersActivity", "Role updated, reloading list");
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
