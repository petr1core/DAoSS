package com.example.flowcharteditorclient;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class ProjectOverviewFragment extends Fragment {

    private static final String ARG_PROJECT_ID = "project_id";
    private String projectId;
    private TextView tvProjectName;
    private TextView tvProjectDescription;
    private TextView tvOwner;
    private ImageView btnCopyOwnerEmail;
    private ProgressBar progressBarOwner;
    private TextView tvVisibility;
    private TextView tvCreatedAt;
    private UsersApi usersApi;
    private UserResponse ownerData;

    public static ProjectOverviewFragment newInstance(String projectId) {
        ProjectOverviewFragment fragment = new ProjectOverviewFragment();
        Bundle args = new Bundle();
        args.putString(ARG_PROJECT_ID, projectId);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            projectId = getArguments().getString(ARG_PROJECT_ID);
        }
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_project_overview, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        
        tvProjectName = view.findViewById(R.id.tvProjectName);
        tvProjectDescription = view.findViewById(R.id.tvProjectDescription);
        tvOwner = view.findViewById(R.id.tvOwner);
        btnCopyOwnerEmail = view.findViewById(R.id.btnCopyOwnerEmail);
        progressBarOwner = view.findViewById(R.id.progressBarOwner);
        tvVisibility = view.findViewById(R.id.tvVisibility);
        tvCreatedAt = view.findViewById(R.id.tvCreatedAt);
        
        usersApi = ApiClient.getClient(getContext()).create(UsersApi.class);
        
        if (btnCopyOwnerEmail != null) {
            btnCopyOwnerEmail.setOnClickListener(v -> {
                if (ownerData != null && ownerData.email != null && !ownerData.email.isEmpty()) {
                    copyEmailToClipboard(ownerData.email);
                }
            });
        }
    }

    public void updateProject(Project project) {
        if (project == null || getView() == null) return;

        if (project.name != null && !project.name.isEmpty()) {
            tvProjectName.setText(project.name);
        } else {
            tvProjectName.setText("Без названия");
        }

        if (project.description != null && !project.description.isEmpty() &&
                !project.description.equals("No description")) {
            tvProjectDescription.setText(project.description);
            tvProjectDescription.setVisibility(View.VISIBLE);
        } else {
            tvProjectDescription.setVisibility(View.GONE);
        }

        if (project.ownerId != null && !project.ownerId.isEmpty()) {
            // Если данные владельца уже загружены и это тот же владелец, просто обновляем отображение
            if (ownerData != null && ownerData.id != null && project.ownerId.equals(ownerData.id)) {
                updateOwnerDisplay();
            } else {
                // Сбрасываем данные владельца, если владелец изменился
                ownerData = null;
                // Показываем индикатор загрузки и загружаем данные владельца
                showOwnerLoadingState();
                loadOwnerData(project.ownerId);
            }
        } else {
            tvOwner.setText("Владелец: Неизвестно");
            if (progressBarOwner != null) {
                progressBarOwner.setVisibility(View.GONE);
            }
            if (btnCopyOwnerEmail != null) {
                btnCopyOwnerEmail.setVisibility(View.GONE);
            }
        }

        String visibilityText = "Видимость: ";
        if (project.visibility != null) {
            if ("private".equalsIgnoreCase(project.visibility)) {
                visibilityText += "Приватный";
            } else if ("public".equalsIgnoreCase(project.visibility)) {
                visibilityText += "Публичный";
            } else {
                visibilityText += project.visibility;
            }
        } else {
            visibilityText += "Неизвестно";
        }
        tvVisibility.setText(visibilityText);

        if (project.createdAt != null) {
            try {
                SimpleDateFormat inputFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss", Locale.getDefault());
                Date date = inputFormat.parse(project.createdAt);
                if (date != null) {
                    SimpleDateFormat outputFormat = new SimpleDateFormat("dd.MM.yyyy HH:mm", Locale.getDefault());
                    tvCreatedAt.setText("Создан: " + outputFormat.format(date));
                } else {
                    tvCreatedAt.setText("Создан: " + project.createdAt);
                }
            } catch (Exception e) {
                tvCreatedAt.setText("Создан: " + project.createdAt);
            }
        } else {
            tvCreatedAt.setText("Создан: Неизвестно");
        }
    }

    private void showOwnerLoadingState() {
        if (tvOwner != null) {
            tvOwner.setText("Владелец: Загрузка...");
            android.content.res.TypedArray typedArray = getContext().obtainStyledAttributes(new int[]{android.R.attr.textColorSecondary});
            int textColor = typedArray.getColor(0, 0xFF808080);
            typedArray.recycle();
            tvOwner.setTextColor(textColor);
        }
        if (progressBarOwner != null) {
            progressBarOwner.setVisibility(View.VISIBLE);
        }
        if (btnCopyOwnerEmail != null) {
            btnCopyOwnerEmail.setVisibility(View.GONE);
        }
    }

    private void loadOwnerData(String ownerId) {
        if (ownerId == null || ownerId.isEmpty() || getContext() == null) {
            return;
        }

        usersApi.getUser(ownerId).enqueue(new Callback<UserResponse>() {
            @Override
            public void onResponse(@NonNull Call<UserResponse> call, @NonNull Response<UserResponse> response) {
                if (response.isSuccessful() && response.body() != null && getView() != null) {
                    ownerData = response.body();
                    updateOwnerDisplay();
                } else {
                    // При ошибке показываем UUID
                    if (getView() != null && tvOwner != null) {
                        if (progressBarOwner != null) {
                            progressBarOwner.setVisibility(View.GONE);
                        }
                        tvOwner.setText("Владелец: " + ownerId);
                        android.content.res.TypedArray typedArray = getContext().obtainStyledAttributes(new int[]{android.R.attr.textColorPrimary});
                        int textColor = typedArray.getColor(0, 0xFF000000);
                        typedArray.recycle();
                        tvOwner.setTextColor(textColor);
                    }
                }
            }

            @Override
            public void onFailure(@NonNull Call<UserResponse> call, @NonNull Throwable t) {
                android.util.Log.e("ProjectOverviewFragment", "Failed to load owner data for " + ownerId, t);
                // При ошибке показываем UUID
                if (getView() != null && tvOwner != null) {
                    if (progressBarOwner != null) {
                        progressBarOwner.setVisibility(View.GONE);
                    }
                    tvOwner.setText("Владелец: " + ownerId);
                    android.content.res.TypedArray typedArray = getContext().obtainStyledAttributes(new int[]{android.R.attr.textColorPrimary});
                    int textColor = typedArray.getColor(0, 0xFF000000);
                    typedArray.recycle();
                    tvOwner.setTextColor(textColor);
                }
            }
        });
    }

    private void updateOwnerDisplay() {
        if (ownerData == null || getView() == null || tvOwner == null) {
            return;
        }

        // Скрываем индикатор загрузки
        if (progressBarOwner != null) {
            progressBarOwner.setVisibility(View.GONE);
        }

        // Устанавливаем правильный цвет текста
        android.content.res.TypedArray typedArray = getContext().obtainStyledAttributes(new int[]{android.R.attr.textColorPrimary});
        int textColor = typedArray.getColor(0, 0xFF000000);
        typedArray.recycle();
        tvOwner.setTextColor(textColor);

        // Отображаем имя или логин, если есть, иначе email, иначе UUID
        String displayName = ownerData.name != null && !ownerData.name.isEmpty()
                ? ownerData.name
                : (ownerData.login != null && !ownerData.login.isEmpty()
                        ? ownerData.login
                        : (ownerData.email != null && !ownerData.email.isEmpty()
                                ? ownerData.email
                                : "Неизвестно"));

        tvOwner.setText("Владелец: " + displayName);

        // Показываем иконку копирования email, если email есть
        if (btnCopyOwnerEmail != null) {
            if (ownerData.email != null && !ownerData.email.isEmpty()) {
                btnCopyOwnerEmail.setVisibility(View.VISIBLE);
            } else {
                btnCopyOwnerEmail.setVisibility(View.GONE);
            }
        }
    }

    private void copyEmailToClipboard(String email) {
        if (getContext() == null) return;
        ClipboardManager clipboard = (ClipboardManager) getContext().getSystemService(android.content.Context.CLIPBOARD_SERVICE);
        ClipData clip = ClipData.newPlainText("Email", email);
        clipboard.setPrimaryClip(clip);
        Toast.makeText(getContext(), "Email скопирован: " + email, Toast.LENGTH_SHORT).show();
    }
}

