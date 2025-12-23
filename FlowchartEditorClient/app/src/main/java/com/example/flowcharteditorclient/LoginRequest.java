package com.example.flowcharteditorclient;

import com.google.gson.annotations.SerializedName;

public class LoginRequest {
    @SerializedName("Login")
    public String login;
    
    @SerializedName("Password")
    public String password;

    public LoginRequest(String login, String password) {
        this.login = login;
        this.password = password;
    }

}
