package com.example.flowcharteditorclient;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class ProjectInvitationAdapter extends RecyclerView.Adapter<ProjectInvitationAdapter.ViewHolder> {

    private final List<InvitationResponse> invitations;
    private final InvitationsApi invitationsApi;
    private final UsersApi usersApi;
    private final String projectId;
    private final android.content.Context context;
    private final Map<String, UserResponse> userCache = new HashMap<>();
    private final java.util.Set<String> loadingUsers = new java.util.HashSet<>();
    private OnInvitationCanceledListener listener;

    public interface OnInvitationCanceledListener {
        void onInvitationCanceled();
    }

    public ProjectInvitationAdapter(List<InvitationResponse> invitations, InvitationsApi invitationsApi,
            String projectId, android.content.Context context) {
        this.invitations = invitations;
        this.invitationsApi = invitationsApi;
        this.usersApi = ApiClient.getClient(context).create(UsersApi.class);
        this.projectId = projectId;
        this.context = context;
    }

    public void setOnInvitationCanceledListener(OnInvitationCanceledListener listener) {
        this.listener = listener;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_project_invitation, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        InvitationResponse invitation = invitations.get(position);

        // Логируем для отладки
        android.util.Log.d("ProjectInvitationAdapter",
                "Binding invitation - InvitedUserId: '" + invitation.invitedUserId +
                        "', Role: '" + invitation.role + "', Status: '" + invitation.status + "'");

        // Загружаем данные пользователя, если еще не загружены
        if (invitation.invitedUserId != null && !invitation.invitedUserId.isEmpty()) {
            UserResponse userData = userCache.get(invitation.invitedUserId);
            if (userData != null) {
                updateUserDisplay(holder, userData);
            } else {
                // Показываем индикатор загрузки вместо UUID
                showLoadingState(holder);
                if (!loadingUsers.contains(invitation.invitedUserId)) {
                    loadUserData(invitation.invitedUserId, holder);
                }
            }
        } else {
            holder.tvInvitedUserId.setText("Неизвестный пользователь");
            holder.btnCopyEmail.setVisibility(View.GONE);
            holder.progressBar.setVisibility(View.GONE);
            android.util.Log.w("ProjectInvitationAdapter", "InvitedUserId is null or empty");
        }

        // Форматируем роль
        String role = invitation.role != null ? invitation.role : "";
        if (!role.isEmpty()) {
            role = role.substring(0, 1).toUpperCase() + role.substring(1).toLowerCase();
        }
        holder.tvRole.setText(role);

        // Отображаем статус
        String status = invitation.status != null ? invitation.status : "";
        if (!status.isEmpty()) {
            status = status.substring(0, 1).toUpperCase() + status.substring(1).toLowerCase();
        }
        holder.tvStatus.setText(status);

        // Показываем кнопку отмены только для pending приглашений
        if ("pending".equalsIgnoreCase(invitation.status)) {
            holder.btnCancel.setVisibility(View.VISIBLE);
            holder.btnCancel.setOnClickListener(v -> cancelInvitation(invitation, holder.getAdapterPosition()));
        } else {
            holder.btnCancel.setVisibility(View.GONE);
        }
    }

    private void showLoadingState(ViewHolder holder) {
        holder.tvInvitedUserId.setText("Загрузка...");
        android.content.res.TypedArray typedArray = context
                .obtainStyledAttributes(new int[] { android.R.attr.textColorSecondary });
        int textColor = typedArray.getColor(0, 0xFF808080);
        typedArray.recycle();
        holder.tvInvitedUserId.setTextColor(textColor);
        holder.progressBar.setVisibility(View.VISIBLE);
        holder.btnCopyEmail.setVisibility(View.GONE);
    }

    private void updateUserDisplay(ViewHolder holder, UserResponse user) {
        // Скрываем индикатор загрузки
        holder.progressBar.setVisibility(View.GONE);
        android.content.res.TypedArray typedArray = context
                .obtainStyledAttributes(new int[] { android.R.attr.textColorPrimary });
        int textColor = typedArray.getColor(0, 0xFF000000);
        typedArray.recycle();
        holder.tvInvitedUserId.setTextColor(textColor);

        // Отображаем email пользователя
        String displayText = user.email != null && !user.email.isEmpty()
                ? user.email
                : (user.name != null && !user.name.isEmpty()
                        ? user.name
                        : (user.login != null && !user.login.isEmpty()
                                ? user.login
                                : "Неизвестный пользователь"));

        holder.tvInvitedUserId.setText(displayText);

        // Показываем иконку копирования email, если email есть
        if (user.email != null && !user.email.isEmpty()) {
            holder.btnCopyEmail.setVisibility(View.VISIBLE);
            holder.btnCopyEmail.setOnClickListener(v -> copyEmailToClipboard(user.email));
        } else {
            holder.btnCopyEmail.setVisibility(View.GONE);
        }
    }

    private void loadUserData(String userId, ViewHolder holder) {
        if (userId == null || userId.isEmpty() || userCache.containsKey(userId) || loadingUsers.contains(userId)) {
            return;
        }

        loadingUsers.add(userId);

        usersApi.getUser(userId).enqueue(new Callback<UserResponse>() {
            @Override
            public void onResponse(@NonNull Call<UserResponse> call, @NonNull Response<UserResponse> response) {
                loadingUsers.remove(userId);
                if (response.isSuccessful() && response.body() != null) {
                    UserResponse user = response.body();
                    userCache.put(userId, user);
                    // Обновляем отображение, если holder еще актуален
                    if (holder != null && holder.getAdapterPosition() != RecyclerView.NO_POSITION) {
                        int position = holder.getAdapterPosition();
                        if (position >= 0 && position < invitations.size()
                                && invitations.get(position).invitedUserId.equals(userId)) {
                            updateUserDisplay(holder, user);
                        }
                    } else {
                        // Уведомляем об изменении, чтобы обновить все элементы
                        notifyDataSetChanged();
                    }
                } else {
                    // При ошибке показываем UUID
                    if (holder != null && holder.getAdapterPosition() != RecyclerView.NO_POSITION) {
                        int position = holder.getAdapterPosition();
                        if (position >= 0 && position < invitations.size()
                                && invitations.get(position).invitedUserId.equals(userId)) {
                            holder.progressBar.setVisibility(View.GONE);
                            holder.tvInvitedUserId.setText(userId);
                            android.content.res.TypedArray typedArray = context
                                    .obtainStyledAttributes(new int[] { android.R.attr.textColorPrimary });
                            int textColor = typedArray.getColor(0, 0xFF000000);
                            typedArray.recycle();
                            holder.tvInvitedUserId.setTextColor(textColor);
                        }
                    }
                }
            }

            @Override
            public void onFailure(@NonNull Call<UserResponse> call, @NonNull Throwable t) {
                loadingUsers.remove(userId);
                android.util.Log.e("ProjectInvitationAdapter", "Failed to load user data for " + userId, t);
                // При ошибке показываем UUID
                if (holder != null && holder.getAdapterPosition() != RecyclerView.NO_POSITION) {
                    int position = holder.getAdapterPosition();
                    if (position >= 0 && position < invitations.size()
                            && invitations.get(position).invitedUserId.equals(userId)) {
                        holder.progressBar.setVisibility(View.GONE);
                        holder.tvInvitedUserId.setText(userId);
                        android.content.res.TypedArray typedArray = context
                                .obtainStyledAttributes(new int[] { android.R.attr.textColorPrimary });
                        int textColor = typedArray.getColor(0, 0xFF000000);
                        typedArray.recycle();
                        holder.tvInvitedUserId.setTextColor(textColor);
                    }
                }
            }
        });
    }

    private void copyEmailToClipboard(String email) {
        ClipboardManager clipboard = (ClipboardManager) context
                .getSystemService(android.content.Context.CLIPBOARD_SERVICE);
        ClipData clip = ClipData.newPlainText("Email", email);
        clipboard.setPrimaryClip(clip);
        Toast.makeText(context, "Email скопирован: " + email, Toast.LENGTH_SHORT).show();
    }

    private void cancelInvitation(InvitationResponse invitation, int position) {
        invitationsApi.cancelInvitation(projectId, invitation.id).enqueue(new Callback<Void>() {
            @Override
            public void onResponse(@NonNull Call<Void> call, @NonNull Response<Void> response) {
                if (response.isSuccessful()) {
                    Toast.makeText(
                            context,
                            "Приглашение отменено",
                            Toast.LENGTH_SHORT).show();

                    // Удаляем из списка
                    invitations.remove(position);
                    notifyItemRemoved(position);

                    if (listener != null) {
                        listener.onInvitationCanceled();
                    }
                } else {
                    Toast.makeText(
                            context,
                            "Не удалось отменить приглашение",
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<Void> call, @NonNull Throwable t) {
                Toast.makeText(
                        context,
                        "Ошибка сети",
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public int getItemCount() {
        return invitations.size();
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        TextView tvInvitedUserId, tvRole, tvStatus;
        ImageView btnCopyEmail;
        android.widget.ProgressBar progressBar;
        android.view.View btnCancel;

        ViewHolder(@NonNull View itemView) {
            super(itemView);
            tvInvitedUserId = itemView.findViewById(R.id.tvInvitedUserId);
            tvRole = itemView.findViewById(R.id.tvRole);
            tvStatus = itemView.findViewById(R.id.tvStatus);
            btnCopyEmail = itemView.findViewById(R.id.btnCopyEmail);
            progressBar = itemView.findViewById(R.id.progressBar);
            btnCancel = itemView.findViewById(R.id.btnCancel);
        }
    }
}
