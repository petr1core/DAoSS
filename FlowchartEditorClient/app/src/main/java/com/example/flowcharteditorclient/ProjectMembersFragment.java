package com.example.flowcharteditorclient;

import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.floatingactionbutton.ExtendedFloatingActionButton;

import java.util.List;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

public class ProjectMembersFragment extends Fragment {

    private static final String ARG_PROJECT_ID = "project_id";
    private String projectId;
    private RecyclerView recyclerView;
    private ExtendedFloatingActionButton fabAdd;
    private ProjectMembersApi membersApi;
    private ProjectMemberAdapter adapter;
    private View emptyView;

    public static ProjectMembersFragment newInstance(String projectId) {
        ProjectMembersFragment fragment = new ProjectMembersFragment();
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
        View view = inflater.inflate(R.layout.fragment_project_members, container, false);
        
        recyclerView = view.findViewById(R.id.recyclerMembers);
        fabAdd = view.findViewById(R.id.fabAddMember);
        emptyView = view.findViewById(R.id.emptyView);

        recyclerView.setLayoutManager(new LinearLayoutManager(getContext()));

        membersApi = ApiClient.getClient(getContext()).create(ProjectMembersApi.class);

        fabAdd.setOnClickListener(v -> {
            Intent intent = new Intent(getContext(), AddMemberActivity.class);
            intent.putExtra("PROJECT_ID", projectId);
            startActivity(intent);
        });

        return view;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        loadMembers();
    }

    @Override
    public void onResume() {
        super.onResume();
        loadMembers();
    }

    private void loadMembers() {
        membersApi.getMembers(projectId).enqueue(new Callback<List<ProjectMember>>() {
            @Override
            public void onResponse(@NonNull Call<List<ProjectMember>> call,
                    @NonNull Response<List<ProjectMember>> response) {

                if (response.isSuccessful() && response.body() != null) {
                    List<ProjectMember> members = response.body();
                    updateUI(members);
                } else {
                    if (response.code() == 401) {
                        handleUnauthorized();
                    } else {
                        Toast.makeText(getContext(),
                                "Не удалось загрузить участников",
                                Toast.LENGTH_SHORT).show();
                        updateUI(java.util.Collections.emptyList());
                    }
                }
            }

            @Override
            public void onFailure(@NonNull Call<List<ProjectMember>> call,
                    @NonNull Throwable t) {
                Toast.makeText(getContext(),
                        "Ошибка сети",
                        Toast.LENGTH_SHORT).show();
                updateUI(java.util.Collections.emptyList());
            }
        });
    }

    private void updateUI(List<ProjectMember> members) {
        if (members.isEmpty()) {
            recyclerView.setVisibility(View.GONE);
            emptyView.setVisibility(View.VISIBLE);
            if (adapter != null) {
                recyclerView.setAdapter(null);
                adapter = null;
            }
        } else {
            recyclerView.setVisibility(View.VISIBLE);
            emptyView.setVisibility(View.GONE);
            adapter = new ProjectMemberAdapter(members, membersApi, projectId, getContext());
            adapter.setOnMemberRemovedListener(() -> loadMembers());
            adapter.setOnRoleUpdatedListener(() -> loadMembers());
            recyclerView.setAdapter(adapter);
        }
    }

    private void handleUnauthorized() {
        TokenManager.clear(getContext());
        Intent intent = new Intent(getContext(), LoginActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(intent);
        if (getActivity() != null) {
            getActivity().finish();
        }
    }
}

