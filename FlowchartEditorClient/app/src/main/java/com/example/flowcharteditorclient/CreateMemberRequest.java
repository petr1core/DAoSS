package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;

public class CreateMemberRequest {
    @SerializedName("UserId")
    public String userId;
    
    @SerializedName("Role")
    public String role;

    public CreateMemberRequest(String userId, String role) {
        this.userId = userId;
        this.role = role;
    }
}

