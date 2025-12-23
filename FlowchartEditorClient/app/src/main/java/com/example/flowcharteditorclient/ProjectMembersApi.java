package com.example.flowcharteditorclient;

import java.util.List;

import retrofit2.Call;
import retrofit2.http.Body;
import retrofit2.http.DELETE;
import retrofit2.http.GET;
import retrofit2.http.POST;
import retrofit2.http.Path;

public interface ProjectMembersApi {

    @GET("/api/projects/{projectId}/members")
    Call<List<ProjectMember>> getMembers(@Path("projectId") String projectId);

    @POST("/api/projects/{projectId}/members")
    Call<ProjectMember> addMember(@Path("projectId") String projectId, @Body CreateMemberRequest request);

    @DELETE("/api/projects/{projectId}/members/{userId}")
    Call<Void> removeMember(@Path("projectId") String projectId, @Path("userId") String userId);
}

