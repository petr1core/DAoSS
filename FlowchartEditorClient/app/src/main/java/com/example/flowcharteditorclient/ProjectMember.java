package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;

public class ProjectMember {
    @SerializedName("ProjectId")
    public String projectId;
    
    @SerializedName("UserId")
    public String userId;
    
    @SerializedName("Role")
    public String role;
    
    @SerializedName("CreatedAt")
    public String createdAt;
}

