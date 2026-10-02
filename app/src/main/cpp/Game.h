#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

class Renderer;
class TextureAsset;

enum class GameState {
    Title,
    Playing,
    Paused,
    BossIntro,
    BossFight,
    UpgradeSelect,
    GameOver
};

enum class EnemyType { Scout, Snake, Turret, Flanker };
enum class BossType { Carrier, Hunter, Prism, Tempest };
enum class PickupType {
    Shield,
    Heal,
    AttackBoost,
    PermanentDamage,
    PermanentFireRate,
    PermanentMaxHealth
};
enum class UpgradeType { Damage, FireRate, MaxHealth };

enum GameEventBits : uint32_t {
    EventNone = 0,
    EventShoot = 1u << 0,
    EventHit = 1u << 1,
    EventExplosion = 1u << 2,
    EventPickup = 1u << 3,
    EventBossWarning = 1u << 4,
    EventBossDefeat = 1u << 5,
    EventUpgrade = 1u << 6,
    EventSaveHighScore = 1u << 7,
    EventSaveSlot = 1u << 8,
    EventImpact = 1u << 9,
    EventBossPhase = 1u << 10
};

struct InputState {
    bool pressed = false;
    bool released = false;
    bool down = false;
    float x = 0.0f;
    float y = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;

    void clearTransient() {
        pressed = false;
        released = false;
        deltaX = 0.0f;
        deltaY = 0.0f;
    }
};

struct PlayerStats {
    float maxHealth = 100.0f;
    float damage = 20.0f;
    float fireInterval = 0.45f;
};

struct RunProgress {
    float survivalTime = 0.0f;
    float normalCombatTime = 0.0f;
    int kills = 0;
    int bossesDefeated = 0;
    int combo = 0;
    int scoreBonus = 0;
};

struct Bullet {
    bool active = false;
    bool boosted = false;
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float radius = 0.0f;
    float damage = 0.0f;
    float age = 0.0f;
};

struct Boss {
    bool active = false;
    BossType type = BossType::Carrier;
    float x = 180.0f;
    float y = -60.0f;
    float vx = 0.0f;
    float health = 0.0f;
    float maxHealth = 0.0f;
    float shootTimer = 0.0f;
    float burstTimer = 0.0f;
    float hitFlash = 0.0f;
    float phaseFlash = 0.0f;
    float age = 0.0f;
    int burstShots = 0;
    int phase = 1;
};

struct SaveSnapshot {
    bool occupied = false;
    int bossesDefeated = 0;
    int score = 0;
    int scoreBonus = 0;
    int kills = 0;
    float survivalTime = 0.0f;
    float normalCombatTime = 0.0f;
    float currentHealth = 100.0f;
    int lastPlayedEpoch = 0;
    PlayerStats playerStats{};
};

struct GameAssets {
    std::shared_ptr<TextureAsset> background;
    std::shared_ptr<TextureAsset> player;
    std::shared_ptr<TextureAsset> enemyScout;
    std::shared_ptr<TextureAsset> enemySnake;
    std::shared_ptr<TextureAsset> enemyTurret;
    std::shared_ptr<TextureAsset> enemyFlanker;
    std::shared_ptr<TextureAsset> bossCarrier;
    std::shared_ptr<TextureAsset> bossHunter;
    std::shared_ptr<TextureAsset> bossPrism;
    std::shared_ptr<TextureAsset> bossTempest;
    std::shared_ptr<TextureAsset> playerBullet;
    std::shared_ptr<TextureAsset> boostedPlayerBullet;
    std::shared_ptr<TextureAsset> enemyBullet;
    std::shared_ptr<TextureAsset> effectRing;
    std::shared_ptr<TextureAsset> shieldAura;
    std::shared_ptr<TextureAsset> pickupShield;
    std::shared_ptr<TextureAsset> pickupHeal;
    std::shared_ptr<TextureAsset> pickupAttack;
    std::shared_ptr<TextureAsset> shield;
    std::shared_ptr<TextureAsset> warning;
    std::shared_ptr<TextureAsset> explosion;
    std::shared_ptr<TextureAsset> upgradeDamage;
    std::shared_ptr<TextureAsset> upgradeFire;
    std::shared_ptr<TextureAsset> upgradeHealth;
    std::shared_ptr<TextureAsset> font;

    bool complete() const;
};

class Game {
public:
    explicit Game(int highScore);

    void handleInput(const InputState &input);
    void update(float deltaSeconds);
    void render(Renderer &renderer, const GameAssets &assets) const;
    void pause();

    uint32_t consumeEvents();
    int highScore() const { return highScore_; }
    int score() const { return score_; }
    int health() const { return static_cast<int>(currentHealth_ + 0.5f); }
    float currentHealth() const { return currentHealth_; }
    float playerX() const { return playerX_; }
    float playerY() const { return playerY_; }
    float shieldTime() const { return shieldTime_; }
    float attackBoostTime() const { return attackBoostTime_; }
    GameState state() const { return state_; }
    const PlayerStats &playerStats() const { return playerStats_; }
    const RunProgress &progress() const { return progress_; }
    const Boss &boss() const { return boss_; }
    const SaveSnapshot &saveSnapshot() const;
    const SaveSnapshot &saveSlot(int slot) const;
    void setSaveSlot(int slot, const SaveSnapshot &snapshot);
    int selectedSlot() const { return selectedSlot_; }
    std::size_t activePlayerBullets() const;
    std::size_t activeBoostedPlayerBullets() const;
    std::size_t activeEnemyBullets() const;
    std::size_t activePickupEffects() const;

#ifdef CANYON_HEADLESS_TEST
    void debugSetNormalCombatTime(float seconds) { progress_.normalCombatTime = seconds; }
    float debugEnemyHealth(EnemyType type, int stage = 0) const;
    void debugDamageBoss(float damage);
    void debugApplyPickup(PickupType type);
    std::size_t debugActiveEnemies() const;
    float debugPermanentDropChance() const;
    void debugDamagePlayer(float damage);
#endif

private:
    struct Enemy {
        bool active = false;
        EnemyType type = EnemyType::Scout;
        float x = 0.0f;
        float y = 0.0f;
        float originX = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float radius = 0.0f;
        float health = 0.0f;
        float contactDamage = 0.0f;
        float age = 0.0f;
        float shootTimer = 0.0f;
        float hitFlash = 0.0f;
    };

    struct Pickup {
        bool active = false;
        PickupType type = PickupType::Shield;
        float x = 0.0f;
        float y = 0.0f;
        float vy = 0.0f;
        float age = 0.0f;
    };

    struct Explosion {
        bool active = false;
        float x = 0.0f;
        float y = 0.0f;
        float age = 0.0f;
        float scale = 1.0f;
    };

    struct PickupEffect {
        bool active = false;
        PickupType type = PickupType::Shield;
        float x = 0.0f;
        float y = 0.0f;
        float age = 0.0f;
    };

    void startRun(int slot = -1);
    void loadRun(int slot);
    void requestSaveSlot();
    void finishRun();
    void beginBossIntro();
    void beginBossFight();
    void defeatBoss();
    void chooseUpgrade(UpgradeType type);
    void clearCombatObjects();

    void spawnEnemy();
    void spawnPickup(bool allowAdvanced = true, float x = -1.0f, float y = -24.0f);
    void spawnPermanentPickup(float x, float y);
    void spawnPlayerBullet();
    void spawnEnemyBullet(float x, float y, float angle, float speed, float damage);
    void spawnAimedEnemyBullet(float x, float y, float speed, float damage, float angleOffset = 0.0f);
    void spawnExplosion(float x, float y, float scale = 1.0f);
    void spawnPickupEffect(PickupType type, float x, float y);
    void applyPickup(PickupType type);
    void damagePlayer(float damage, float x, float y);
    void damageBoss(float damage);
    void killEnemy(Enemy &enemy);

    void updateTimers(float deltaSeconds);
    void updatePlayerFire(float deltaSeconds);
    void updateEnemies(float deltaSeconds);
    void updateBullets(float deltaSeconds);
    void updatePickups(float deltaSeconds);
    void updateBoss(float deltaSeconds);
    void updateEffects(float deltaSeconds);
    void updateScore();

    float randomFloat(float minimum, float maximum);
    static bool collides(float ax, float ay, float ar, float bx, float by, float br);

    GameState state_ = GameState::Title;
    GameState pausedFrom_ = GameState::Playing;
    PlayerStats playerStats_{};
    RunProgress progress_{};
    Boss boss_{};

    float backgroundTravel_ = 0.0f;
    float playerX_ = 180.0f;
    float playerY_ = 540.0f;
    float playerTilt_ = 0.0f;
    float currentHealth_ = 100.0f;
    float invulnerable_ = 0.0f;
    float shieldTime_ = 0.0f;
    float attackBoostTime_ = 0.0f;
    float fireTimer_ = 0.0f;
    float enemyTimer_ = 0.0f;
    float pickupTimer_ = 0.0f;
    float bossIntroTimer_ = 0.0f;
    float comboTimer_ = 0.0f;
    float shakeTime_ = 0.0f;
    float upgradeAnimation_ = 0.0f;
    UpgradeType lastUpgrade_ = UpgradeType::Damage;

    int score_ = 0;
    int highScore_ = 0;
    int selectedSlot_ = -1;
    uint32_t randomState_ = 0x6d2b79f5u;
    uint32_t pendingEvents_ = EventNone;

    std::array<Enemy, 32> enemies_{};
    std::array<Bullet, 64> playerBullets_{};
    std::array<Bullet, 96> enemyBullets_{};
    std::array<Pickup, 16> pickups_{};
    std::array<Explosion, 40> explosions_{};
    std::array<PickupEffect, 16> pickupEffects_{};
    std::array<SaveSnapshot, 3> saveSlots_{};
};
