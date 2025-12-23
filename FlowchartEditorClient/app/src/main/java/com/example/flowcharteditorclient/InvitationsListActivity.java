package com.example.flowcharteditorclient;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class InvitationsListActivity extends AppCompatActivity {

    private RecyclerView recyclerView;
    private InvitationsApi invitationsApi;
    private ProjectApi projectApi;
    private android.view.View emptyView;
    private InvitationAdapter adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_invitations_list);

        recyclerView = findViewById(R.id.recyclerInvitations);
        emptyView = findViewById(R.id.emptyView);

        androidx.appcompat.widget.Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setDisplayShowHomeEnabled(true);
        }

        recyclerView.setLayoutManager(new LinearLayoutManager(this));

        invitationsApi = ApiClient.getClient(this).create(InvitationsApi.class);
        projectApi = ApiClient.getClient(this).create(ProjectApi.class);
    }

    @Override
    protected void onResume() {
        super.onResume();
        loadInvitations();
    }

    private void loadInvitations() {
        invitationsApi.getUserInvitations().enqueue(new Callback<List<InvitationResponse>>() {
            @Override
            public void onResponse(@NonNull Call<List<InvitationResponse>> call,
                    @NonNull Response<List<InvitationResponse>> response) {

                if (response.isSuccessful() && response.body() != null) {
                    List<InvitationResponse> invitations = response.body();
                    updateUI(invitations);
                } else if (response.code() == 401) {
                    handleUnauthorized();
                } else {
                    Toast.makeText(InvitationsListActivity.this,
                            "Не удалось загрузить приглашения",
                            Toast.LENGTH_SHORT).show();
                    updateUI(java.util.Collections.emptyList());
                }
            }

            @Override
            public void onFailure(@NonNull Call<List<InvitationResponse>> call,
                    @NonNull Throwable t) {
                Toast.makeText(InvitationsListActivity.this,
                        "Ошибка сети",
                        Toast.LENGTH_SHORT).show();
                updateUI(java.util.Collections.emptyList());
            }
        });
    }

    private void updateUI(List<InvitationResponse> invitations) {
        if (invitations.isEmpty()) {
            recyclerView.setVisibility(android.view.View.GONE);
            emptyView.setVisibility(android.view.View.VISIBLE);
        } else {
            recyclerView.setVisibility(android.view.View.VISIBLE);
            emptyView.setVisibility(android.view.View.GONE);
            adapter = new InvitationAdapter(invitations, invitationsApi, projectApi, this);
            adapter.setOnInvitationActionListener(new InvitationAdapter.OnInvitationActionListener() {
                @Override
                public void onInvitationAccepted(String projectId) {
                    // Переходим к проекту после принятия приглашения
                    if (projectId == null || projectId.isEmpty()) {
                        Toast.makeText(InvitationsListActivity.this, "Ошибка: ID проекта не найден", Toast.LENGTH_SHORT)
                                .show();
                        return;
                    }
                    Intent intent = new Intent(InvitationsListActivity.this, ProjectMembersActivity.class);
                    intent.putExtra("PROJECT_ID", projectId.trim());
                    startActivity(intent);
                }

                @Override
                public void onInvitationRejected() {
                    // Обновляем список после отклонения
                    loadInvitations();
                }
            });
            recyclerView.setAdapter(adapter);
        }
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
