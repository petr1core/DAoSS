package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;

public class Project {
    @SerializedName(value = "Id", alternate = { "id" })
    public String id; // UUID from server

    @SerializedName(value = "Name", alternate = { "name" })
    public String name;

    @SerializedName(value = "Description", alternate = { "description" })
    public String description;

    @SerializedName(value = "OwnerId", alternate = { "ownerId" })
    public String ownerId;

    @SerializedName(value = "DefaultLanguageId", alternate = { "defaultLanguageId" })
    public String defaultLanguageId;

    @SerializedName(value = "Visibility", alternate = { "visibility" })
    public String visibility;

    @SerializedName(value = "RequiredReviewersRules", alternate = { "requiredReviewersRules" })
    public String requiredReviewersRules;

    @SerializedName(value = "CreatedAt", alternate = { "createdAt" })
    public String createdAt;

    @SerializedName(value = "UpdatedAt", alternate = { "updatedAt" })
    public String updatedAt;

    // role is not part of Project entity, it's in ProjectMember
    // Keeping it for UI compatibility, but it should be fetched separately
    public String role;
}
