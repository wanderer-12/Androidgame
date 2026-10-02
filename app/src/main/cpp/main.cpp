#include <jni.h>

#include <game-activity/GameActivity.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <algorithm>
#include <chrono>
#include <memory>

#include "AndroidOut.h"
#include "Game.h"
#include "Renderer.h"

namespace {
class PlatformBridge {
public:
    explicit PlatformBridge(GameActivity *activity)
            : vm_(activity->vm), activity_(activity->javaGameActivity) {
        if (vm_->GetEnv(reinterpret_cast<void **>(&env_), JNI_VERSION_1_6) == JNI_EDETACHED) {
            vm_->AttachCurrentThread(&env_, nullptr);
            attached_ = true;
        }
        jclass activityClass = env_->GetObjectClass(activity_);
        playEvent_ = env_->GetMethodID(activityClass, "playGameEvent", "(I)V");
        loadHighScore_ = env_->GetMethodID(activityClass, "loadHighScore", "()I");
        saveHighScore_ = env_->GetMethodID(activityClass, "saveHighScore", "(I)V");
        loadGameSlot_ = env_->GetMethodID(activityClass, "loadGameSlot", "(I)[I");
        saveGameSlot_ = env_->GetMethodID(activityClass, "saveGameSlot", "(IIIIIIIIIIII)V");
        env_->DeleteLocalRef(activityClass);
    }

    ~PlatformBridge() {
        if (attached_) vm_->DetachCurrentThread();
    }

    int loadHighScore() const {
        return loadHighScore_ ? env_->CallIntMethod(activity_, loadHighScore_) : 0;
    }

    void saveHighScore(int score) const {
        if (saveHighScore_) env_->CallVoidMethod(activity_, saveHighScore_, score);
    }

    void playEvent(int event) const {
        if (playEvent_) env_->CallVoidMethod(activity_, playEvent_, event);
    }

    SaveSnapshot loadGameSlot(int slot) const {
        SaveSnapshot snapshot;
        if (!loadGameSlot_) return snapshot;
        auto *values = static_cast<jintArray>(env_->CallObjectMethod(activity_, loadGameSlot_, slot));
        if (!values) return snapshot;
        const jsize length = env_->GetArrayLength(values);
        if (length >= 12) {
            jint data[12]{};
            env_->GetIntArrayRegion(values, 0, 12, data);
            snapshot.occupied = data[0] != 0;
            snapshot.bossesDefeated = data[1];
            snapshot.score = data[2];
            snapshot.scoreBonus = data[3];
            snapshot.kills = data[4];
            snapshot.survivalTime = static_cast<float>(data[5]);
            snapshot.normalCombatTime = static_cast<float>(data[6]);
            snapshot.currentHealth = static_cast<float>(data[7]);
            snapshot.playerStats.maxHealth = static_cast<float>(data[8]);
            snapshot.playerStats.damage = static_cast<float>(data[9]) / 100.0f;
            snapshot.playerStats.fireInterval = static_cast<float>(data[10]) / 1000.0f;
            snapshot.lastPlayedEpoch = data[11];
        }
        env_->DeleteLocalRef(values);
        return snapshot;
    }

    void saveGameSlot(int slot, const SaveSnapshot &snapshot) const {
        if (!saveGameSlot_ || slot < 0) return;
        env_->CallVoidMethod(activity_, saveGameSlot_, slot, snapshot.bossesDefeated,
                             snapshot.score, snapshot.scoreBonus, snapshot.kills,
                             static_cast<jint>(snapshot.survivalTime),
                             static_cast<jint>(snapshot.normalCombatTime),
                             static_cast<jint>(snapshot.currentHealth),
                             static_cast<jint>(snapshot.playerStats.maxHealth),
                             static_cast<jint>(snapshot.playerStats.damage * 100.0f),
                             static_cast<jint>(snapshot.playerStats.fireInterval * 1000.0f),
                             snapshot.lastPlayedEpoch);
    }

private:
    JavaVM *vm_ = nullptr;
    jobject activity_ = nullptr;
    JNIEnv *env_ = nullptr;
    jmethodID playEvent_ = nullptr;
    jmethodID loadHighScore_ = nullptr;
    jmethodID saveHighScore_ = nullptr;
    jmethodID loadGameSlot_ = nullptr;
    jmethodID saveGameSlot_ = nullptr;
    bool attached_ = false;
};

class Engine {
public:
        explicit Engine(android_app *app)
            : app_(app), platform_(app->activity), game_(platform_.loadHighScore()),
              lastFrame_(Clock::now()) {
        for (int slot = 0; slot < 3; ++slot) game_.setSaveSlot(slot, platform_.loadGameSlot(slot));
    }

    void onCommand(int32_t command) {
        switch (command) {
            case APP_CMD_INIT_WINDOW:
                if (app_->window) createRenderer();
                break;
            case APP_CMD_TERM_WINDOW:
                assets_ = {};
                renderer_.reset();
                break;
            case APP_CMD_PAUSE:
            case APP_CMD_STOP:
            case APP_CMD_LOST_FOCUS:
                game_.pause();
                dispatchEvents(game_.consumeEvents());
                break;
            case APP_CMD_RESUME:
            case APP_CMD_GAINED_FOCUS:
                lastFrame_ = Clock::now();
                break;
            default:
                break;
        }
    }

    bool hasRenderer() const { return renderer_ && renderer_->ready() && assets_.complete(); }

    void frame() {
        handleInput();
        game_.handleInput(input_);
        input_.clearTransient();

        const auto now = Clock::now();
        float frameSeconds = std::chrono::duration<float>(now - lastFrame_).count();
        lastFrame_ = now;
        frameSeconds = std::clamp(frameSeconds, 0.0f, 0.05f);
        accumulator_ = std::min(accumulator_ + frameSeconds, 0.2f);
        constexpr float step = 1.0f / 60.0f;
        while (accumulator_ >= step) {
            game_.update(step);
            accumulator_ -= step;
        }

        renderer_->beginFrame();
        game_.render(*renderer_, assets_);
        renderer_->endFrame();
        dispatchEvents(game_.consumeEvents());
    }

private:
    using Clock = std::chrono::steady_clock;

    void createRenderer() {
        renderer_ = std::make_unique<Renderer>(app_);
        if (!renderer_->ready()) return;
        assets_.background = renderer_->loadTexture("canyon_background.png", false);
        assets_.player = renderer_->loadTexture("player_ship.png", true);
        assets_.enemyScout = renderer_->loadTexture("enemy_scout.png", true);
        assets_.enemySnake = renderer_->loadTexture("enemy_snake.png", true);
        assets_.enemyTurret = renderer_->loadTexture("enemy_turret.png", true);
        assets_.enemyFlanker = renderer_->loadTexture("enemy_flanker.png", true);
        assets_.bossCarrier = renderer_->loadTexture("boss_carrier.png", true);
        assets_.bossHunter = renderer_->loadTexture("boss_hunter.png", true);
        assets_.bossPrism = renderer_->loadTexture("boss_prism.png", true);
        assets_.bossTempest = renderer_->loadTexture("boss_tempest.png", true);
        assets_.playerBullet = renderer_->loadTexture("bullet_player.png", true);
        assets_.boostedPlayerBullet = renderer_->loadTexture("bullet_player_boost.png", true);
        assets_.enemyBullet = renderer_->loadTexture("bullet_enemy.png", true);
        assets_.effectRing = renderer_->loadTexture("effect_ring.png", true);
        assets_.shieldAura = renderer_->loadTexture("shield_aura.png", true);
        assets_.pickupShield = renderer_->loadTexture("pickup_shield.png", true);
        assets_.pickupHeal = renderer_->loadTexture("pickup_heal.png", true);
        assets_.pickupAttack = renderer_->loadTexture("pickup_attack.png", true);
        assets_.shield = renderer_->loadTexture("shield.png", true);
        assets_.warning = renderer_->loadTexture("warning.png", true);
        assets_.explosion = renderer_->loadTexture("explosion_strip.png", true);
        assets_.upgradeDamage = renderer_->loadTexture("upgrade_damage.png", true);
        assets_.upgradeFire = renderer_->loadTexture("upgrade_fire.png", true);
        assets_.upgradeHealth = renderer_->loadTexture("upgrade_health.png", true);
        assets_.font = renderer_->loadTexture("font.png", true);
        if (!assets_.complete()) aout << "One or more game assets failed to load" << std::endl;
        lastFrame_ = Clock::now();
        accumulator_ = 0.0f;
    }

    void dispatchEvents(uint32_t events) {
        if (events & EventShoot) platform_.playEvent(0);
        if (events & EventImpact) platform_.playEvent(1);
        if (events & EventHit) platform_.playEvent(2);
        if (events & EventExplosion) platform_.playEvent(3);
        if (events & EventPickup) platform_.playEvent(4);
        if (events & EventBossWarning) platform_.playEvent(5);
        if (events & EventBossPhase) platform_.playEvent(6);
        if (events & EventBossDefeat) platform_.playEvent(7);
        if (events & EventUpgrade) platform_.playEvent(8);
        if (events & EventSaveSlot) {
            platform_.saveGameSlot(game_.selectedSlot(), game_.saveSnapshot());
        }
        if (events & EventSaveHighScore) platform_.saveHighScore(game_.highScore());
    }

    void handleInput() {
        android_input_buffer *buffer = android_app_swap_input_buffers(app_);
        if (!buffer) return;
        for (uint64_t i = 0; i < buffer->motionEventsCount; ++i) {
            GameActivityMotionEvent &event = buffer->motionEvents[i];
            const int action = event.action & AMOTION_EVENT_ACTION_MASK;
            const int pointerIndex = (event.action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                                     AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            if ((action == AMOTION_EVENT_ACTION_DOWN ||
                 action == AMOTION_EVENT_ACTION_POINTER_DOWN) && activePointerId_ < 0) {
                const auto &pointer = event.pointers[pointerIndex];
                activePointerId_ = pointer.id;
                lastScreenX_ = GameActivityPointerAxes_getX(&pointer);
                lastScreenY_ = GameActivityPointerAxes_getY(&pointer);
                input_.pressed = true;
                input_.down = true;
                renderer_->screenToCanvas(lastScreenX_, lastScreenY_, input_.x, input_.y);
            } else if (action == AMOTION_EVENT_ACTION_MOVE && activePointerId_ >= 0) {
                for (uint32_t p = 0; p < event.pointerCount; ++p) {
                    const auto &pointer = event.pointers[p];
                    if (pointer.id != activePointerId_) continue;
                    const float screenX = GameActivityPointerAxes_getX(&pointer);
                    const float screenY = GameActivityPointerAxes_getY(&pointer);
                    const auto delta = renderer_->screenDeltaToCanvas(
                            screenX - lastScreenX_, screenY - lastScreenY_);
                    input_.deltaX += delta.first;
                    input_.deltaY += delta.second;
                    lastScreenX_ = screenX;
                    lastScreenY_ = screenY;
                    renderer_->screenToCanvas(screenX, screenY, input_.x, input_.y);
                    break;
                }
            } else if (action == AMOTION_EVENT_ACTION_CANCEL) {
                activePointerId_ = -1;
                input_.released = true;
                input_.down = false;
            } else if (action == AMOTION_EVENT_ACTION_UP ||
                       action == AMOTION_EVENT_ACTION_POINTER_UP) {
                const auto &pointer = event.pointers[pointerIndex];
                if (pointer.id == activePointerId_) {
                    activePointerId_ = -1;
                    input_.released = true;
                    input_.down = false;
                }
            }
        }
        android_app_clear_motion_events(buffer);

        for (uint64_t i = 0; i < buffer->keyEventsCount; ++i) {
            const GameActivityKeyEvent &event = buffer->keyEvents[i];
            if (event.action == AKEY_EVENT_ACTION_DOWN && event.keyCode == AKEYCODE_BACK) {
                game_.pause();
            }
        }
        android_app_clear_key_events(buffer);
    }

    android_app *app_;
    PlatformBridge platform_;
    Game game_;
    std::unique_ptr<Renderer> renderer_;
    GameAssets assets_;
    InputState input_;
    int32_t activePointerId_ = -1;
    float lastScreenX_ = 0.0f;
    float lastScreenY_ = 0.0f;
    Clock::time_point lastFrame_;
    float accumulator_ = 0.0f;
};

void handleCommand(android_app *app, int32_t command) {
    static_cast<Engine *>(app->userData)->onCommand(command);
}

bool motionEventFilter(const GameActivityMotionEvent *event) {
    return (event->source & AINPUT_SOURCE_CLASS_MASK) == AINPUT_SOURCE_CLASS_POINTER;
}
}

extern "C" void android_main(android_app *app) {
    Engine engine(app);
    app->userData = &engine;
    app->onAppCmd = handleCommand;
    android_app_set_motion_event_filter(app, motionEventFilter);

    while (!app->destroyRequested) {
        int events = 0;
        android_poll_source *source = nullptr;
        int result = 0;
        do {
            result = ALooper_pollOnce(engine.hasRenderer() ? 0 : -1, nullptr, &events,
                                      reinterpret_cast<void **>(&source));
            if (result >= 0 && source) source->process(app, source);
        } while (result >= 0 && !app->destroyRequested);
        if (engine.hasRenderer()) engine.frame();
    }
}
