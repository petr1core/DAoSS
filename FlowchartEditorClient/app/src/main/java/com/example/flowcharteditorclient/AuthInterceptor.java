package com.example.flowcharteditorclient;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;

import okhttp3.Interceptor;
import okhttp3.Request;
import okhttp3.Response;

import java.io.IOException;

public class AuthInterceptor implements Interceptor {

    private final Context context;

    public AuthInterceptor(Context context) {
        this.context = context;
    }

    @NonNull
    @Override
    public Response intercept(@NonNull Chain chain) throws IOException {

        Request originalRequest = chain.request();

        String token = TokenManager.getToken(context);

        if (token != null) {
            Request newRequest = originalRequest.newBuilder()
                    .addHeader("Authorization", "Bearer " + token)
                    .build();
            Response response = chain.proceed(newRequest);
            
            // Если получили 401 Unauthorized, очищаем токен
            // Activity будут проверять код ответа в своих callback и вызывать logout()
            if (response.code() == 401) {
                TokenManager.clear(context);
            }
            
            return response;
        }

        return chain.proceed(originalRequest);
    }
}

