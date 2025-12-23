package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;

public class SendInvitationRequest {
    @SerializedName(value = "InvitedUserId", alternate = {"invitedUserId"})
    public String invitedUserId;
    
    @SerializedName(value = "Role", alternate = {"role"})
    public String role;
    
    public SendInvitationRequest(String invitedUserId, String role) {
        if (invitedUserId == null) {
            android.util.Log.e("SendInvitationRequest", "invitedUserId is null!");
            throw new IllegalArgumentException("InvitedUserId cannot be null");
        }
        if (role == null) {
            android.util.Log.e("SendInvitationRequest", "role is null!");
            throw new IllegalArgumentException("Role cannot be null");
        }
        this.invitedUserId = invitedUserId.trim();
        this.role = role.trim();
        // Логируем для отладки
        android.util.Log.d("SendInvitationRequest", "Created request - InvitedUserId: '" + this.invitedUserId + "', Role: '" + this.role + "'");
    }
}

