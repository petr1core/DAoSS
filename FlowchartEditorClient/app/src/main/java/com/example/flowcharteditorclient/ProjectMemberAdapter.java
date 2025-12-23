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
import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.RecyclerView;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class ProjectMemberAdapter extends RecyclerView.Adapter<ProjectMemberAdapter.ViewHolder> {

    private final List<ProjectMember> members;
    private final ProjectMembersApi membersApi;
    private final UsersApi usersApi;
    private final String projectId;
    private final android.content.Context context;
    private final Map<String, UserResponse> userCache = new HashMap<>();
    private final java.util.Set<String> loadingUsers = new java.util.HashSet<>();
    private OnMemberRemovedListener listener;
    private OnRoleUpdatedListener roleUpdatedListener;

    public interface OnMemberRemovedListener {
        void onMemberRemoved();
    }

    public interface OnRoleUpdatedListener {
        void onRoleUpdated();
    }

    public ProjectMemberAdapter(List<ProjectMember> members, ProjectMembersApi membersApi, String projectId,
            android.content.Context context) {
        this.members = members;
        this.membersApi = membersApi;
        this.usersApi = ApiClient.getClient(context).create(UsersApi.class);
        this.projectId = projectId;
        this.context = context;
    }

    public void setOnMemberRemovedListener(OnMemberRemovedListener listener) {
        this.listener = listener;
    }

    public void setOnRoleUpdatedListener(OnRoleUpdatedListener listener) {
        this.roleUpdatedListener = listener;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_member, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        ProjectMember member = members.get(position);

        // Логируем для отладки
        android.util.Log.d("ProjectMemberAdapter",
                "Binding member - UserId: '" + member.userId + "', Role: '" + member.role + "'");

        // Загружаем данные пользователя, если еще не загружены
        UserResponse userData = userCache.get(member.userId);
        if (userData != null) {
            updateUserDisplay(holder, userData, member);
        } else {
            // Показываем индикатор загрузки вместо UUID
            showLoadingState(holder);
            if (member.userId != null && !member.userId.isEmpty() && !loadingUsers.contains(member.userId)) {
                loadUserData(member.userId, holder, member);
            }
        }

        // Форматируем роль с заглавной буквы
        String role = member.role != null ? member.role : "";
        if (!role.isEmpty()) {
            role = role.substring(0, 1).toUpperCase() + role.substring(1).toLowerCase();
        }
        holder.tvRole.setText(role);

        // Скрываем кнопку удаления - удаление участников недоступно в Android клиенте
        holder.btnDelete.setVisibility(android.view.View.GONE);

        // Обработчик долгого нажатия для изменения роли
        holder.itemView.setOnLongClickListener(v -> {
            showChangeRoleDialog(member);
            return true;
        });
    }

    private void showLoadingState(ViewHolder holder) {
        holder.tvUserId.setText("Загрузка...");
        android.content.res.TypedArray typedArray = context
                .obtainStyledAttributes(new int[] { android.R.attr.textColorSecondary });
        int textColor = typedArray.getColor(0, 0xFF808080);
        typedArray.recycle();
        holder.tvUserId.setTextColor(textColor);
        holder.progressBar.setVisibility(View.VISIBLE);
        holder.btnCopyEmail.setVisibility(View.GONE);
    }

    private void updateUserDisplay(ViewHolder holder, UserResponse user, ProjectMember member) {
        // Скрываем индикатор загрузки
        holder.progressBar.setVisibility(View.GONE);
        android.content.res.TypedArray typedArray = context
                .obtainStyledAttributes(new int[] { android.R.attr.textColorPrimary });
        int textColor = typedArray.getColor(0, 0xFF000000);
        typedArray.recycle();
        holder.tvUserId.setTextColor(textColor);

        // Отображаем имя или логин, если есть, иначе email, иначе UUID
        String displayName = user.name != null && !user.name.isEmpty()
                ? user.name
                : (user.login != null && !user.login.isEmpty()
                        ? user.login
                        : (user.email != null && !user.email.isEmpty()
                                ? user.email
                                : member.userId));

        holder.tvUserId.setText(displayName);

        // Показываем иконку копирования email, если email есть
        if (user.email != null && !user.email.isEmpty()) {
            holder.btnCopyEmail.setVisibility(View.VISIBLE);
            holder.btnCopyEmail.setOnClickListener(v -> copyEmailToClipboard(user.email));
        } else {
            holder.btnCopyEmail.setVisibility(View.GONE);
        }
    }

    private void loadUserData(String userId, ViewHolder holder, ProjectMember member) {
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
                        if (position >= 0 && position < members.size() && members.get(position).userId.equals(userId)) {
                            updateUserDisplay(holder, user, members.get(position));
                        }
                    } else {
                        // Уведомляем об изменении, чтобы обновить все элементы
                        notifyDataSetChanged();
                    }
                } else {
                    // При ошибке показываем UUID
                    if (holder != null && holder.getAdapterPosition() != RecyclerView.NO_POSITION) {
                        int position = holder.getAdapterPosition();
                        if (position >= 0 && position < members.size() && members.get(position).userId.equals(userId)) {
                            holder.progressBar.setVisibility(View.GONE);
                            holder.tvUserId.setText(member.userId);
                            android.content.res.TypedArray typedArray = context
                                    .obtainStyledAttributes(new int[] { android.R.attr.textColorPrimary });
                            int textColor = typedArray.getColor(0, 0xFF000000);
                            typedArray.recycle();
                            holder.tvUserId.setTextColor(textColor);
                        }
                    }
                }
            }

            @Override
            public void onFailure(@NonNull Call<UserResponse> call, @NonNull Throwable t) {
                loadingUsers.remove(userId);
                android.util.Log.e("ProjectMemberAdapter", "Failed to load user data for " + userId, t);
                // При ошибке показываем UUID
                if (holder != null && holder.getAdapterPosition() != RecyclerView.NO_POSITION) {
                    int position = holder.getAdapterPosition();
                    if (position >= 0 && position < members.size() && members.get(position).userId.equals(userId)) {
                        holder.progressBar.setVisibility(View.GONE);
                        holder.tvUserId.setText(member.userId);
                        android.content.res.TypedArray typedArray = context
                                .obtainStyledAttributes(new int[] { android.R.attr.textColorPrimary });
                        int textColor = typedArray.getColor(0, 0xFF000000);
                        typedArray.recycle();
                        holder.tvUserId.setTextColor(textColor);
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

    private void showChangeRoleDialog(ProjectMember member) {
        android.widget.ArrayAdapter<CharSequence> adapter = android.widget.ArrayAdapter.createFromResource(
                context,
                R.array.member_roles,
                android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

        android.widget.Spinner spinner = new android.widget.Spinner(context);
        spinner.setAdapter(adapter);

        // Устанавливаем текущую роль
        String currentRole = member.role != null ? member.role : "";
        if (!currentRole.isEmpty()) {
            currentRole = currentRole.substring(0, 1).toUpperCase() + currentRole.substring(1).toLowerCase();
        }
        int position = 0;
        for (int i = 0; i < adapter.getCount(); i++) {
            if (adapter.getItem(i).toString().equalsIgnoreCase(currentRole)) {
                position = i;
                break;
            }
        }
        spinner.setSelection(position);

        new androidx.appcompat.app.AlertDialog.Builder(context)
                .setTitle("Изменить роль")
                .setMessage("Выберите новую роль для участника " + member.userId)
                .setView(spinner)
                .setPositiveButton("Сохранить", (dialog, which) -> {
                    String selectedRole = spinner.getSelectedItem().toString().toLowerCase();
                    updateMemberRole(member, selectedRole);
                })
                .setNegativeButton("Отмена", null)
                .show();
    }

    private void updateMemberRole(ProjectMember member, String newRole) {
        UpdateRoleRequest request = new UpdateRoleRequest(newRole);
        membersApi.updateMemberRole(projectId, member.userId, request).enqueue(new retrofit2.Callback<ProjectMember>() {
            @Override
            public void onResponse(@NonNull retrofit2.Call<ProjectMember> call,
                    @NonNull retrofit2.Response<ProjectMember> response) {
                if (response.isSuccessful()) {
                    Toast.makeText(context, "Роль обновлена", Toast.LENGTH_SHORT).show();
                    if (roleUpdatedListener != null) {
                        roleUpdatedListener.onRoleUpdated();
                    }
                } else {
                    Toast.makeText(context, "Не удалось обновить роль", Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull retrofit2.Call<ProjectMember> call, @NonNull Throwable t) {
                Toast.makeText(context, "Ошибка сети", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void showDeleteConfirmation(android.content.Context context, ProjectMember member) {
        new AlertDialog.Builder(context)
                .setTitle("Удалить участника?")
                .setMessage("Вы уверены, что хотите удалить участника " + member.userId + "?")
                .setPositiveButton("Удалить", (dialog, which) -> removeMember(member))
                .setNegativeButton("Отмена", null)
                .show();
    }

    private void removeMember(ProjectMember member) {
        android.util.Log.d("ProjectMemberAdapter", "Removing member: " + member.userId);
        membersApi.removeMember(projectId, member.userId).enqueue(new Callback<Void>() {
            @Override
            public void onResponse(@NonNull Call<Void> call, @NonNull Response<Void> response) {
                if (response.isSuccessful()) {
                    android.util.Log.d("ProjectMemberAdapter", "Member removed successfully");
                    Toast.makeText(context, "Участник удален", Toast.LENGTH_SHORT).show();
                    // Всегда вызываем listener для перезагрузки списка с сервера
                    if (listener != null) {
                        listener.onMemberRemoved();
                    }
                } else {
                    android.util.Log.e("ProjectMemberAdapter", "Failed to remove member: " + response.code());
                    Toast.makeText(context, "Не удалось удалить участника", Toast.LENGTH_SHORT).show();
                    // Перезагрузим список в любом случае
                    if (listener != null) {
                        listener.onMemberRemoved();
                    }
                }
            }

            @Override
            public void onFailure(@NonNull Call<Void> call, @NonNull Throwable t) {
                android.util.Log.e("ProjectMemberAdapter", "Network error removing member", t);
                Toast.makeText(context, "Ошибка сети", Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    public int getItemCount() {
        return members.size();
    }

    static class ViewHolder extends RecyclerView.ViewHolder {

        TextView tvUserId, tvRole;
        ImageView btnCopyEmail;
        android.widget.ProgressBar progressBar;
        android.view.View btnDelete;

        ViewHolder(@NonNull View itemView) {
            super(itemView);
            tvUserId = itemView.findViewById(R.id.tvUserId);
            tvRole = itemView.findViewById(R.id.tvRole);
            btnCopyEmail = itemView.findViewById(R.id.btnCopyEmail);
            progressBar = itemView.findViewById(R.id.progressBar);
            btnDelete = itemView.findViewById(R.id.btnDelete);
        }
    }
}
