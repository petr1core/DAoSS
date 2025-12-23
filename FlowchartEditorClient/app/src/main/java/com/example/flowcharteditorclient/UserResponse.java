package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;

public class UserResponse {
    @SerializedName(value = "Id", alternate = {"id"})
    public String id;
    
    @SerializedName(value = "Name", alternate = {"name"})
    public String name;
    
    @SerializedName(value = "Email", alternate = {"email"})
    public String email;
    
    @SerializedName(value = "Login", alternate = {"login"})
    public String login;
}

