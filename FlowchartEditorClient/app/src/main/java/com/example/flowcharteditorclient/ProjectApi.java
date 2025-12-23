package com.example.flowcharteditorclient;

import java.util.List;

import retrofit2.Call;
import retrofit2.http.GET;
import retrofit2.http.Body;
import retrofit2.http.POST;
import retrofit2.http.Query;

public interface ProjectApi {

    @GET("/api/projects")
    Call<List<Project>> getProjects(@Query("ownerId") String ownerId);

    @POST("/api/projects")
    Call<Project> createProject(@Body CreateProjectRequest request);
}

