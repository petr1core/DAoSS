package com.example.flowcharteditorclient;

public class Project {
    public String id; // UUID from server
    public String name;
    public String description;
    public String ownerId;
    public String defaultLanguageId;
    public String visibility;
    public String requiredReviewersRules;
    // role is not part of Project entity, it's in ProjectMember
    // Keeping it for UI compatibility, but it should be fetched separately
    public String role;
}
