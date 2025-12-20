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

public class AddMemberActivity extends AppCompatActivity {

    private EditText etUserId;
    private Spinner spinnerRole;
    private MaterialButton btnAdd;
    private ProjectMembersApi membersApi;
    private String projectId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_add_member);

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

        etUserId = findViewById(R.id.etUserId);
        spinnerRole = findViewById(R.id.spinnerRole);
        btnAdd = findViewById(R.id.btnAdd);

        // Настраиваем Spinner с ролями (без owner)
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
                this,
                R.array.member_roles_add,
                android.R.layout.simple_spinner_item
        );
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerRole.setAdapter(adapter);

        membersApi = ApiClient.getClient(this).create(ProjectMembersApi.class);

        btnAdd.setOnClickListener(v -> addMember());
    }

    private void addMember() {
        String userId = etUserId.getText().toString().trim();

        if (userId.isEmpty()) {
            Toast.makeText(this, "User ID cannot be empty", Toast.LENGTH_SHORT).show();
            return;
        }

        // Получаем выбранную роль из Spinner (owner недоступен)
        String role = spinnerRole.getSelectedItem().toString().toLowerCase();

        CreateMemberRequest request = new CreateMemberRequest(userId, role);

        membersApi.addMember(projectId, request)
                .enqueue(new Callback<ProjectMember>() {
                    @Override
                    public void onResponse(@NonNull Call<ProjectMember> call,
                                           @NonNull Response<ProjectMember> response) {

                        if (response.isSuccessful()) {
                            Toast.makeText(AddMemberActivity.this,
                                    "Member added successfully",
                                    Toast.LENGTH_SHORT).show();
                            finish(); // возвращаемся к списку участников
                        } else if (response.code() == 401) {
                            handleUnauthorized();
                        } else {
                            Toast.makeText(AddMemberActivity.this,
                                    "Failed to add member",
                                    Toast.LENGTH_SHORT).show();
                        }
                    }

                    @Override
                    public void onFailure(@NonNull Call<ProjectMember> call,
                                          @NonNull Throwable t) {
                        Toast.makeText(AddMemberActivity.this,
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

