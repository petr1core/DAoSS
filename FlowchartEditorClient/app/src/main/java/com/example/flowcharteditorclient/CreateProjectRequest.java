package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;
import java.util.UUID;

public class CreateProjectRequest {
    @SerializedName("Name")
    public String name;
    
    @SerializedName("Description")
    public String description;
    
    @SerializedName("OwnerId")
    public String ownerId;
    
    @SerializedName("DefaultLanguageId")
    public String defaultLanguageId;
    
    @SerializedName("Visibility")
    public String visibility;
    
    @SerializedName("RequiredReviewersRules")
    public String requiredReviewersRules;

    public CreateProjectRequest(String name, String description, String ownerId) {
        this.name = name;
        this.description = description;
        this.ownerId = ownerId;
        this.visibility = "private";
        // Используем пустой GUID вместо пустой строки
        this.defaultLanguageId = "00000000-0000-0000-0000-000000000000";
        this.requiredReviewersRules = null; // Optional
    }
}
