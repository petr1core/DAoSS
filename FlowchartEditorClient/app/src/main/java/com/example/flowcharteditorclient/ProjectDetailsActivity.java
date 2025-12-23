package com.example.flowcharteditorclient;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager2.adapter.FragmentStateAdapter;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class ProjectDetailsActivity extends AppCompatActivity {

    private String projectId;
    private ProjectApi projectApi;
    private AuthApi authApi;
    private String currentUserId;
    private Project currentProject;
    private ViewPager2 viewPager;
    private TabLayout tabLayout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_project_details);

        projectId = getIntent().getStringExtra("PROJECT_ID");
        if (projectId != null) {
            projectId = projectId.trim();
        }
        if (projectId == null || projectId.isEmpty()) {
            android.util.Log.e("ProjectDetailsActivity",
                    "PROJECT_ID is null or empty. Intent extras: " + getIntent().getExtras());
            Toast.makeText(this, "Project ID not provided", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }
        android.util.Log.d("ProjectDetailsActivity", "Received PROJECT_ID: " + projectId);

        androidx.appcompat.widget.Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setDisplayShowHomeEnabled(true);
        }

        viewPager = findViewById(R.id.viewPager);
        tabLayout = findViewById(R.id.tabLayout);

        projectApi = ApiClient.getClient(this).create(ProjectApi.class);
        authApi = ApiClient.getClient(this).create(AuthApi.class);

        // Настраиваем ViewPager с фрагментами
        ProjectDetailsPagerAdapter adapter = new ProjectDetailsPagerAdapter(this, projectId);
        viewPager.setAdapter(adapter);

        // Связываем TabLayout с ViewPager
        new TabLayoutMediator(tabLayout, viewPager, (tab, position) -> {
            switch (position) {
                case 0:
                    tab.setText("Обзор");
                    break;
                case 1:
                    tab.setText("Участники");
                    break;
                case 2:
                    tab.setText("Приглашения");
                    break;
            }
        }).attach();

        // Загружаем информацию о проекте и пользователе
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
                Toast.makeText(ProjectDetailsActivity.this,
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
                    if (getSupportActionBar() != null) {
                        getSupportActionBar().setTitle(currentProject.name);
                    }
                    // Обновляем фрагмент обзора
                    updateOverviewFragment();
                } else if (response.code() == 401) {
                    handleUnauthorized();
                } else {
                    Toast.makeText(ProjectDetailsActivity.this,
                            "Не удалось загрузить проект",
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<Project> call,
                    @NonNull Throwable t) {
                Toast.makeText(ProjectDetailsActivity.this,
                        "Ошибка сети",
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void updateOverviewFragment() {
        if (currentProject == null) return;
        
        // Находим фрагмент обзора и обновляем его
        // ViewPager2 использует теги вида "f" + position для фрагментов
        Fragment fragment = getSupportFragmentManager()
                .findFragmentByTag("f" + 0); // Обзор всегда на позиции 0
        if (fragment instanceof ProjectOverviewFragment) {
            ((ProjectOverviewFragment) fragment).updateProject(currentProject);
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

    // Адаптер для ViewPager
    private static class ProjectDetailsPagerAdapter extends FragmentStateAdapter {
        private final String projectId;

        public ProjectDetailsPagerAdapter(FragmentActivity activity, String projectId) {
            super(activity);
            this.projectId = projectId;
        }

        @NonNull
        @Override
        public Fragment createFragment(int position) {
            switch (position) {
                case 0:
                    return ProjectOverviewFragment.newInstance(projectId);
                case 1:
                    return ProjectMembersFragment.newInstance(projectId);
                case 2:
                    return ProjectInvitationsFragment.newInstance(projectId);
                default:
                    return ProjectOverviewFragment.newInstance(projectId);
            }
        }

        @Override
        public int getItemCount() {
            return 3; // Обзор, Участники, Приглашения
        }
    }
}

