package com.example.flowcharteditorclient;

import java.util.List;

import retrofit2.Call;
import retrofit2.http.Body;
import retrofit2.http.DELETE;
import retrofit2.http.GET;
import retrofit2.http.POST;
import retrofit2.http.Path;

public interface InvitationsApi {
    @GET("/api/users/by-email/{email}")
    Call<UserResponse> getUserByEmail(@Path("email") String email);
    
    @POST("/api/projects/{projectId}/invitations")
    Call<InvitationResponse> sendInvitation(@Path("projectId") String projectId, @Body SendInvitationRequest request);
    
    @GET("/api/invitations")
    Call<List<InvitationResponse>> getUserInvitations();
    
    @POST("/api/invitations/{invitationId}/accept")
    Call<InvitationResponse> acceptInvitation(@Path("invitationId") String invitationId);
    
    @POST("/api/invitations/{invitationId}/reject")
    Call<InvitationResponse> rejectInvitation(@Path("invitationId") String invitationId);
    
    @GET("/api/projects/{projectId}/invitations")
    Call<List<InvitationResponse>> getProjectInvitations(@Path("projectId") String projectId);
    
    @DELETE("/api/projects/{projectId}/invitations/{invitationId}")
    Call<Void> cancelInvitation(@Path("projectId") String projectId, @Path("invitationId") String invitationId);
}

