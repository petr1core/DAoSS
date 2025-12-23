package com.example.flowcharteditorclient;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class ProjectMemberAdapter extends RecyclerView.Adapter<ProjectMemberAdapter.ViewHolder> {

    private final List<ProjectMember> members;
    private final ProjectMembersApi membersApi;
    private final String projectId;
    private OnMemberRemovedListener listener;

    public interface OnMemberRemovedListener {
        void onMemberRemoved();
    }

    public ProjectMemberAdapter(List<ProjectMember> members, ProjectMembersApi membersApi, String projectId) {
        this.members = members;
        this.membersApi = membersApi;
        this.projectId = projectId;
    }

    public void setOnMemberRemovedListener(OnMemberRemovedListener listener) {
        this.listener = listener;
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
        // Показываем только UUID без префикса
        holder.tvUserId.setText(member.userId);
        
        // Форматируем роль с заглавной буквы
        String role = member.role != null ? member.role : "";
        if (!role.isEmpty()) {
            role = role.substring(0, 1).toUpperCase() + role.substring(1).toLowerCase();
        }
        holder.tvRole.setText(role);
        
        // Обработчик клика на кнопку удаления
        holder.btnDelete.setOnClickListener(v -> {
            showDeleteConfirmation(holder.itemView.getContext(), member);
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
        membersApi.removeMember(projectId, member.userId).enqueue(new Callback<Void>() {
            @Override
            public void onResponse(@NonNull Call<Void> call, @NonNull Response<Void> response) {
                if (response.isSuccessful()) {
                    int position = members.indexOf(member);
                    if (position != -1) {
                        members.remove(position);
                        notifyItemRemoved(position);
                        if (listener != null) {
                            listener.onMemberRemoved();
                        }
                    }
                } else {
                    // Ошибка будет обработана через AuthInterceptor (401) или через listener
                    if (listener != null) {
                        listener.onMemberRemoved(); // Перезагрузим список
                    }
                }
            }

            @Override
            public void onFailure(@NonNull Call<Void> call, @NonNull Throwable t) {
                // Ошибка будет обработана через AuthInterceptor
            }
        });
    }

    @Override
    public int getItemCount() {
        return members.size();
    }

    static class ViewHolder extends RecyclerView.ViewHolder {

        TextView tvUserId, tvRole;
        android.view.View btnDelete;

        ViewHolder(@NonNull View itemView) {
            super(itemView);
            tvUserId = itemView.findViewById(R.id.tvUserId);
            tvRole = itemView.findViewById(R.id.tvRole);
            btnDelete = itemView.findViewById(R.id.btnDelete);
        }
    }
}

