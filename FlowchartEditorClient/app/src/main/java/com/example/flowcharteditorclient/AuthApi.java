package com.example.flowcharteditorclient;
import retrofit2.Call;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.POST;
public interface AuthApi {
    @POST("/api/auth/login")
    Call<AuthResponse> login(@Body LoginRequest request);
    
    @POST("/api/auth/register")
    Call<AuthResponse> register(@Body RegisterRequest request);
    
    @GET("/api/auth/me")
    Call<UserInfo> getCurrentUser();
}
