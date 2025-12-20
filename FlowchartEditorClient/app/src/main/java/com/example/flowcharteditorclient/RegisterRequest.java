package com.example.flowcharteditorclient;

public class RegisterRequest {

    public String email;
    public String password;
    public String name;
    public String login;

    public RegisterRequest(String email, String password, String name, String login) {
        this.email = email;
        this.password = password;
        this.name = name;
        this.login = login;
    }
}
