package com.example.flowcharteditorclient;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Проверяем, есть ли сохраненный токен
        String token = TokenManager.getToken(this);
        
        if (token != null && !token.isEmpty()) {
            // Если токен есть, переходим на список проектов
            startActivity(new Intent(this, ProjectListActivity.class));
        } else {
            // Если токена нет, переходим на экран входа
            startActivity(new Intent(this, LoginActivity.class));
        }
        
        // Закрываем MainActivity, чтобы нельзя было вернуться назад
        finish();
    }
}