package com.example.flowcharteditorclient;

import retrofit2.Call;
import retrofit2.http.GET;
import retrofit2.http.Path;

public interface UsersApi {
    @GET("/api/users/{id}")
    Call<UserResponse> getUser(@Path("id") String id);
}

