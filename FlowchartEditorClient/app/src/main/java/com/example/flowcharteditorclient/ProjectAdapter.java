package com.example.flowcharteditorclient;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

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

        // Логируем для отладки
        android.util.Log.d("ProjectAdapter", "Binding project: name=" + project.name + ", id=" + project.id);

        // Проверяем и устанавливаем название
        if (project.name != null && !project.name.isEmpty()) {
            holder.tvName.setText(project.name);
        } else {
            holder.tvName.setText("Без названия");
            android.util.Log.w("ProjectAdapter", "Project name is null or empty for project with id: " + project.id);
        }

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
            if (project.id == null || project.id.isEmpty()) {
                android.widget.Toast.makeText(
                        holder.itemView.getContext(),
                        "Ошибка: ID проекта не найден",
                        android.widget.Toast.LENGTH_SHORT).show();
                android.util.Log.e("ProjectAdapter", "Project ID is null or empty for project: " + project.name);
                return;
            }
            android.content.Intent intent = new android.content.Intent(
                    holder.itemView.getContext(),
                    ProjectDetailsActivity.class);
            intent.putExtra("PROJECT_ID", project.id.trim());
            holder.itemView.getContext().startActivity(intent);
        });

        // Добавляем обработчик долгого нажатия для контекстного меню (только
        // редактирование)
        holder.itemView.setOnLongClickListener(v -> {
            showContextMenu(holder.itemView.getContext(), project);
            return true;
        });
    }

    private void showContextMenu(android.content.Context context, Project project) {
        androidx.appcompat.app.AlertDialog.Builder builder = new androidx.appcompat.app.AlertDialog.Builder(context);
        builder.setTitle(project.name);

        String[] items = { "Редактировать" };
        builder.setItems(items, (dialog, which) -> {
            if (which == 0) {
                // Редактировать
                if (project.id == null || project.id.isEmpty()) {
                    android.widget.Toast
                            .makeText(context, "Ошибка: ID проекта не найден", android.widget.Toast.LENGTH_SHORT)
                            .show();
                    return;
                }
                android.content.Intent intent = new android.content.Intent(context, EditProjectActivity.class);
                intent.putExtra("PROJECT_ID", project.id.trim());
                context.startActivity(intent);
            }
        });
        builder.show();
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
