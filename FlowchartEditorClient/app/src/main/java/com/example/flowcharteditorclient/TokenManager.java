package com.example.flowcharteditorclient;

import android.content.Context;
import android.content.SharedPreferences;

public class TokenManager {

    private static final String PREFS_NAME = "auth";
    private static final String KEY_TOKEN = "jwt";

    public static void saveToken(Context context, String token) {
        SharedPreferences prefs =
                context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        prefs.edit().putString(KEY_TOKEN, token).apply();
    }

    public static String getToken(Context context) {
        SharedPreferences prefs =
                context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        return prefs.getString(KEY_TOKEN, null);
    }

    public static void clear(Context context) {
        SharedPreferences prefs =
                context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        prefs.edit().clear().apply();
    }
}
