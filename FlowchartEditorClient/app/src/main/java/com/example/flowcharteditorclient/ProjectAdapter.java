package com.example.flowcharteditorclient;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

public class ProjectAdapter extends RecyclerView.Adapter<ProjectAdapter.ViewHolder> {

    private final List<Project> projects;

    public ProjectAdapter(List<Project> projects) {
        this.projects = projects;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_project, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        Project project = projects.get(position);
        holder.tvName.setText(project.name);
        
        // Отображаем описание, если оно есть
        if (project.description != null && !project.description.isEmpty() && 
            !project.description.equals("No description")) {
            holder.tvDescription.setText(project.description);
            holder.tvDescription.setVisibility(android.view.View.VISIBLE);
        } else {
            holder.tvDescription.setVisibility(android.view.View.GONE);
        }
        
        // Скрываем роль, если она пустая или null
        if (project.role != null && !project.role.isEmpty()) {
            String role = project.role.substring(0, 1).toUpperCase() + 
                         project.role.substring(1).toLowerCase();
            holder.tvRole.setText(role);
            holder.tvRole.setVisibility(android.view.View.VISIBLE);
        } else {
            holder.tvRole.setVisibility(android.view.View.GONE);
        }
        
        // Добавляем обработчик клика на элемент
        holder.itemView.setOnClickListener(v -> {
            android.content.Intent intent = new android.content.Intent(
                    holder.itemView.getContext(), 
                    ProjectMembersActivity.class
            );
            intent.putExtra("PROJECT_ID", project.id);
            holder.itemView.getContext().startActivity(intent);
        });
    }

    @Override
    public int getItemCount() {
        return projects.size();
    }

    static class ViewHolder extends RecyclerView.ViewHolder {

        TextView tvName, tvDescription, tvRole;

        ViewHolder(@NonNull View itemView) {
            super(itemView);
            tvName = itemView.findViewById(R.id.tvProjectName);
            tvDescription = itemView.findViewById(R.id.tvProjectDescription);
            tvRole = itemView.findViewById(R.id.tvRole);
        }
    }
}
