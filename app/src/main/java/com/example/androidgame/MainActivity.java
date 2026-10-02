package com.example.androidgame;

import android.content.SharedPreferences;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.Window;

import com.google.androidgamesdk.GameActivity;

public class MainActivity extends GameActivity {
    private static final String PREFERENCES_NAME = "canyon_breakout";
    private static final String HIGH_SCORE_KEY = "high_score";
    private static final int SAVE_VALUE_COUNT = 12;

    private SoundPool soundPool;
    private int[] soundIds;
    private SharedPreferences preferences;

    static {
        System.loadLibrary("androidgame");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreate(savedInstanceState);
        preferences = getSharedPreferences(PREFERENCES_NAME, MODE_PRIVATE);
        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder()
                .setMaxStreams(12)
                .setAudioAttributes(attributes)
                .build();
        soundIds = new int[]{
                soundPool.load(this, R.raw.shoot, 1),
                soundPool.load(this, R.raw.impact, 1),
                soundPool.load(this, R.raw.hit, 1),
                soundPool.load(this, R.raw.explode, 1),
                soundPool.load(this, R.raw.collect, 1),
                soundPool.load(this, R.raw.boss_warning, 1),
                soundPool.load(this, R.raw.boss_phase, 1),
                soundPool.load(this, R.raw.boss_defeat, 1),
                soundPool.load(this, R.raw.upgrade, 1)
        };
        hideSystemUi();
    }

    @SuppressWarnings("unused")
    public void playGameEvent(int event) {
        if (soundPool != null && event >= 0 && event < soundIds.length) {
            soundPool.play(soundIds[event], 1.0f, 1.0f, 1, 0, 1.0f);
        }
    }

    @SuppressWarnings("unused")
    public int loadHighScore() {
        return preferences == null ? 0 : preferences.getInt(HIGH_SCORE_KEY, 0);
    }

    @SuppressWarnings("unused")
    public void saveHighScore(int score) {
        if (preferences != null) preferences.edit().putInt(HIGH_SCORE_KEY, score).apply();
    }

    @SuppressWarnings("unused")
    public int[] loadGameSlot(int slot) {
        int[] values = new int[SAVE_VALUE_COUNT];
        if (preferences == null || slot < 0 || slot >= 3) return values;
        String prefix = "slot_" + slot + "_";
        values[0] = preferences.getBoolean(prefix + "occupied", false) ? 1 : 0;
        values[1] = preferences.getInt(prefix + "bosses", 0);
        values[2] = preferences.getInt(prefix + "score", 0);
        values[3] = preferences.getInt(prefix + "score_bonus", 0);
        values[4] = preferences.getInt(prefix + "kills", 0);
        values[5] = preferences.getInt(prefix + "survival", 0);
        values[6] = preferences.getInt(prefix + "normal_time", 0);
        values[7] = preferences.getInt(prefix + "health", 100);
        values[8] = preferences.getInt(prefix + "max_health", 100);
        values[9] = preferences.getInt(prefix + "damage", 2000);
        values[10] = preferences.getInt(prefix + "fire_interval", 450);
        values[11] = preferences.getInt(prefix + "last_played", 0);
        return values;
    }

    @SuppressWarnings("unused")
    public void saveGameSlot(int slot, int bosses, int score, int scoreBonus, int kills,
                             int survival, int normalTime, int health, int maxHealth,
                             int damage, int fireInterval, int lastPlayed) {
        if (preferences == null || slot < 0 || slot >= 3) return;
        String prefix = "slot_" + slot + "_";
        preferences.edit()
                .putBoolean(prefix + "occupied", true)
                .putInt(prefix + "bosses", Math.max(0, bosses))
                .putInt(prefix + "score", Math.max(0, score))
                .putInt(prefix + "score_bonus", Math.max(0, scoreBonus))
                .putInt(prefix + "kills", Math.max(0, kills))
                .putInt(prefix + "survival", Math.max(0, survival))
                .putInt(prefix + "normal_time", Math.max(0, normalTime))
                .putInt(prefix + "health", Math.max(1, health))
                .putInt(prefix + "max_health", Math.max(100, maxHealth))
                .putInt(prefix + "damage", Math.max(2000, damage))
                .putInt(prefix + "fire_interval", Math.max(120, fireInterval))
                .putInt(prefix + "last_played", Math.max(0, lastPlayed))
                .apply();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemUi();
    }

    @SuppressWarnings("deprecation")
    private void hideSystemUi() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.systemBars());
                controller.setSystemBarsBehavior(
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        }
    }

    @Override
    protected void onDestroy() {
        if (soundPool != null) {
            soundPool.release();
            soundPool = null;
        }
        super.onDestroy();
    }
}
