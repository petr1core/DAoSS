package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;

public class UpdateRoleRequest {
    @SerializedName("Role")
    public String role;
    
    public UpdateRoleRequest(String role) {
        this.role = role;
    }
}

