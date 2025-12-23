package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;

public class RegisterRequest {

    @SerializedName("Email")
    public String email;
    
    @SerializedName("Password")
    public String password;
    
    @SerializedName("Name")
    public String name;
    
    @SerializedName("Login")
    public String login;

    public RegisterRequest(String email, String password, String name, String login) {
        this.email = email;
        this.password = password;
        this.name = name;
        this.login = login;
    }
}
