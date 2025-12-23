package com.example.flowcharteditorclient;

import java.util.List;

import retrofit2.Call;
import retrofit2.http.DELETE;
import retrofit2.http.GET;
import retrofit2.http.Body;
import retrofit2.http.POST;
import retrofit2.http.PUT;
import retrofit2.http.Path;
import retrofit2.http.Query;

public interface ProjectApi {

    @GET("/api/projects")
    Call<List<Project>> getProjects(@Query("ownerId") String ownerId);

    @POST("/api/projects")
    Call<Project> createProject(@Body CreateProjectRequest request);
    
    @GET("/api/projects/{id}")
    Call<Project> getProjectById(@Path("id") String id);
    
    @PUT("/api/projects/{id}")
    Call<Void> updateProject(@Path("id") String id, @Body CreateProjectRequest request);
    
    @DELETE("/api/projects/{id}")
    Call<Void> deleteProject(@Path("id") String id);
}

