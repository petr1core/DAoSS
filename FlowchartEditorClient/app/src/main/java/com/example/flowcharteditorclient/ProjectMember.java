package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;

public class ProjectMember {
    @SerializedName(value = "ProjectId", alternate = {"projectId"})
    public String projectId;
    
    @SerializedName(value = "UserId", alternate = {"userId"})
    public String userId;
    
    @SerializedName(value = "Role", alternate = {"role"})
    public String role;
    
    @SerializedName(value = "CreatedAt", alternate = {"createdAt"})
    public String createdAt;
}

