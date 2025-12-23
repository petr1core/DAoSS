package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;
import java.util.Date;

public class InvitationResponse {
    @SerializedName(value = "Id", alternate = {"id"})
    public String id;
    
    @SerializedName(value = "ProjectId", alternate = {"projectId"})
    public String projectId;
    
    @SerializedName(value = "InvitedUserId", alternate = {"invitedUserId"})
    public String invitedUserId;
    
    @SerializedName(value = "InvitedByUserId", alternate = {"invitedByUserId"})
    public String invitedByUserId;
    
    @SerializedName(value = "Role", alternate = {"role"})
    public String role;
    
    @SerializedName(value = "Status", alternate = {"status"})
    public String status;
    
    @SerializedName(value = "ExpiresAt", alternate = {"expiresAt"})
    public Date expiresAt;
    
    @SerializedName(value = "CreatedAt", alternate = {"createdAt"})
    public Date createdAt;
}

