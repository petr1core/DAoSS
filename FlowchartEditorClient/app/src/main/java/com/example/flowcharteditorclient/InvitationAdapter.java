package com.example.flowcharteditorclient;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class InvitationAdapter extends RecyclerView.Adapter<InvitationAdapter.ViewHolder> {

    private final List<InvitationResponse> invitations;
    private final InvitationsApi invitationsApi;
    private final ProjectApi projectApi;
    private final android.content.Context context;
    private OnInvitationActionListener listener;

    public interface OnInvitationActionListener {
        void onInvitationAccepted(String projectId);

        void onInvitationRejected();
    }

    public InvitationAdapter(List<InvitationResponse> invitations, InvitationsApi invitationsApi, ProjectApi projectApi,
            android.content.Context context) {
        this.invitations = invitations;
        this.invitationsApi = invitationsApi;
        this.projectApi = projectApi;
        this.context = context;
    }

    public void setOnInvitationActionListener(OnInvitationActionListener listener) {
        this.listener = listener;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_invitation, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        InvitationResponse invitation = invitations.get(position);

        // Загружаем информацию о проекте
        loadProjectInfo(holder, invitation.projectId);

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

        // Показываем кнопки действий только для pending приглашений
        if ("pending".equalsIgnoreCase(invitation.status)) {
            holder.actionButtons.setVisibility(View.VISIBLE);

            holder.btnAccept.setOnClickListener(v -> acceptInvitation(invitation, holder.getAdapterPosition()));
            holder.btnReject.setOnClickListener(v -> rejectInvitation(invitation, holder.getAdapterPosition()));
        } else {
            holder.actionButtons.setVisibility(View.GONE);
        }
    }

    private void loadProjectInfo(@NonNull ViewHolder holder, String projectId) {
        projectApi.getProjectById(projectId).enqueue(new Callback<Project>() {
            @Override
            public void onResponse(@NonNull Call<Project> call, @NonNull Response<Project> response) {
                if (response.isSuccessful() && response.body() != null) {
                    Project project = response.body();
                    holder.tvProjectName.setText(project.name);

                    if (project.description != null && !project.description.isEmpty() &&
                            !project.description.equals("No description")) {
                        holder.tvProjectDescription.setText(project.description);
                        holder.tvProjectDescription.setVisibility(View.VISIBLE);
                    } else {
                        holder.tvProjectDescription.setVisibility(View.GONE);
                    }
                } else {
                    holder.tvProjectName.setText("Загрузка...");
                    holder.tvProjectDescription.setVisibility(View.GONE);
                }
            }

            @Override
            public void onFailure(@NonNull Call<Project> call, @NonNull Throwable t) {
                holder.tvProjectName.setText("Не удалось загрузить проект");
                holder.tvProjectDescription.setVisibility(View.GONE);
            }
        });
    }

    private void acceptInvitation(InvitationResponse invitation, int position) {
        invitationsApi.acceptInvitation(invitation.id).enqueue(new Callback<InvitationResponse>() {
            @Override
            public void onResponse(@NonNull Call<InvitationResponse> call,
                    @NonNull Response<InvitationResponse> response) {
                if (response.isSuccessful()) {
                    Toast.makeText(
                            context,
                            "Приглашение принято",
                            Toast.LENGTH_SHORT).show();

                    // Обновляем статус в списке
                    invitation.status = "accepted";
                    notifyItemChanged(position);

                    if (listener != null) {
                        listener.onInvitationAccepted(invitation.projectId);
                    }
                } else {
                    Toast.makeText(
                            context,
                            "Не удалось принять приглашение",
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<InvitationResponse> call, @NonNull Throwable t) {
                Toast.makeText(
                        context,
                        "Ошибка сети",
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void rejectInvitation(InvitationResponse invitation, int position) {
        invitationsApi.rejectInvitation(invitation.id).enqueue(new Callback<InvitationResponse>() {
            @Override
            public void onResponse(@NonNull Call<InvitationResponse> call,
                    @NonNull Response<InvitationResponse> response) {
                if (response.isSuccessful()) {
                    Toast.makeText(
                            context,
                            "Приглашение отклонено",
                            Toast.LENGTH_SHORT).show();

                    // Удаляем из списка
                    invitations.remove(position);
                    notifyItemRemoved(position);

                    if (listener != null) {
                        listener.onInvitationRejected();
                    }
                } else {
                    Toast.makeText(
                            context,
                            "Не удалось отклонить приглашение",
                            Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(@NonNull Call<InvitationResponse> call, @NonNull Throwable t) {
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
        TextView tvProjectName, tvProjectDescription, tvRole, tvStatus;
        View actionButtons;
        com.google.android.material.button.MaterialButton btnAccept, btnReject;

        ViewHolder(@NonNull View itemView) {
            super(itemView);
            tvProjectName = itemView.findViewById(R.id.tvProjectName);
            tvProjectDescription = itemView.findViewById(R.id.tvProjectDescription);
            tvRole = itemView.findViewById(R.id.tvRole);
            tvStatus = itemView.findViewById(R.id.tvStatus);
            actionButtons = itemView.findViewById(R.id.actionButtons);
            btnAccept = itemView.findViewById(R.id.btnAccept);
            btnReject = itemView.findViewById(R.id.btnReject);
        }
    }
}
