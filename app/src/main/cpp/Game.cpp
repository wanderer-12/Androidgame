#include "Game.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>

#ifndef CANYON_HEADLESS_TEST
#include "Renderer.h"
#endif

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kPlayerRadius = 20.0f;
constexpr float kNormalBossInterval = 60.0f;
constexpr float kBossIntroDuration = 2.2f;
constexpr float kWorldTop = 84.0f;
constexpr float kEnemyHealthMultiplier = 0.5f;
constexpr float kPermanentDropChance = 0.12f;
constexpr int kBaseEnemyLimit = 12;

template <typename Collection>
typename Collection::value_type *firstInactive(Collection &collection) {
    for (auto &item : collection) {
        if (!item.active) return &item;
    }
    return nullptr;
}

float scaleForStage(int stage, float amount) {
    return std::pow(1.0f + amount, static_cast<float>(stage));
}

float enemyHealthFor(EnemyType type, int stage) {
    const float baseHealth = type == EnemyType::Scout
                                     ? 25.0f
                                     : (type == EnemyType::Snake ? 45.0f
                                        : (type == EnemyType::Turret ? 90.0f : 20.0f));
    return baseHealth * kEnemyHealthMultiplier * scaleForStage(stage, 0.35f);
}

float bossCollisionRadius(BossType type) {
    if (type == BossType::Hunter) return 46.0f;
    if (type == BossType::Prism) return 44.0f;
    if (type == BossType::Tempest) return 48.0f;
    return 50.0f;
}
}

bool GameAssets::complete() const {
    return background && player && enemyScout && enemySnake && enemyTurret && enemyFlanker && bossCarrier &&
           bossHunter && bossPrism && bossTempest &&
           playerBullet && boostedPlayerBullet && enemyBullet && effectRing && shieldAura &&
           pickupShield && pickupHeal && pickupAttack && shield && warning && explosion &&
           upgradeDamage && upgradeFire && upgradeHealth && font;
}

Game::Game(int highScore) : highScore_(std::max(0, highScore)) {}

void Game::startRun(int slot) {
    state_ = GameState::Playing;
    pausedFrom_ = GameState::Playing;
    selectedSlot_ = slot;
    playerStats_ = {};
    progress_ = {};
    boss_ = {};
    playerX_ = 180.0f;
    playerY_ = 540.0f;
    playerTilt_ = 0.0f;
    currentHealth_ = playerStats_.maxHealth;
    invulnerable_ = 0.0f;
    shieldTime_ = 0.0f;
    attackBoostTime_ = 0.0f;
    randomState_ = 0x6d2b79f5u;
    fireTimer_ = 0.05f;
    enemyTimer_ = 0.8f;
    pickupTimer_ = randomFloat(8.0f, 14.0f);
    bossIntroTimer_ = 0.0f;
    comboTimer_ = 0.0f;
    shakeTime_ = 0.0f;
    upgradeAnimation_ = 0.0f;
    score_ = 0;
    pendingEvents_ = EventNone;
    for (auto &entity : enemies_) entity = {};
    for (auto &bullet : playerBullets_) bullet = {};
    for (auto &bullet : enemyBullets_) bullet = {};
    for (auto &pickup : pickups_) pickup = {};
    for (auto &explosion : explosions_) explosion = {};
    for (auto &effect : pickupEffects_) effect = {};
}

void Game::loadRun(int slot) {
    if (slot < 0 || slot >= static_cast<int>(saveSlots_.size()) || !saveSlots_[slot].occupied) {
        startRun(slot);
        return;
    }
    const SaveSnapshot snapshot = saveSlots_[slot];
    startRun(slot);
    playerStats_ = snapshot.playerStats;
    playerStats_.maxHealth = std::max(100.0f, playerStats_.maxHealth);
    playerStats_.damage = std::max(20.0f, playerStats_.damage);
    playerStats_.fireInterval = std::clamp(playerStats_.fireInterval, 0.12f, 0.45f);
    progress_.bossesDefeated = std::max(0, snapshot.bossesDefeated);
    progress_.kills = std::max(0, snapshot.kills);
    progress_.survivalTime = std::max(0.0f, snapshot.survivalTime);
    progress_.normalCombatTime = std::clamp(snapshot.normalCombatTime, 0.0f,
                                            kNormalBossInterval - 0.01f);
    progress_.scoreBonus = std::max(0, snapshot.scoreBonus);
    currentHealth_ = std::clamp(snapshot.currentHealth, 1.0f, playerStats_.maxHealth);
    score_ = snapshot.score;
    state_ = GameState::Playing;
    enemyTimer_ = 1.0f;
    pickupTimer_ = randomFloat(8.0f, 14.0f);
}

void Game::requestSaveSlot() {
    if (selectedSlot_ < 0 || selectedSlot_ >= static_cast<int>(saveSlots_.size()) ||
        state_ == GameState::Title || state_ == GameState::GameOver) {
        return;
    }
    SaveSnapshot snapshot;
    snapshot.occupied = true;
    snapshot.bossesDefeated = progress_.bossesDefeated;
    snapshot.score = score_;
    snapshot.scoreBonus = progress_.scoreBonus;
    snapshot.kills = progress_.kills;
    snapshot.survivalTime = progress_.survivalTime;
    snapshot.normalCombatTime = progress_.normalCombatTime;
    snapshot.currentHealth = currentHealth_;
    snapshot.lastPlayedEpoch = static_cast<int>(std::time(nullptr));
    snapshot.playerStats = playerStats_;
    saveSlots_[selectedSlot_] = snapshot;
    pendingEvents_ |= EventSaveSlot;
}

const SaveSnapshot &Game::saveSnapshot() const {
    static const SaveSnapshot empty{};
    if (selectedSlot_ < 0 || selectedSlot_ >= static_cast<int>(saveSlots_.size())) return empty;
    return saveSlots_[selectedSlot_];
}

const SaveSnapshot &Game::saveSlot(int slot) const {
    static const SaveSnapshot empty{};
    if (slot < 0 || slot >= static_cast<int>(saveSlots_.size())) return empty;
    return saveSlots_[slot];
}

void Game::setSaveSlot(int slot, const SaveSnapshot &snapshot) {
    if (slot >= 0 && slot < static_cast<int>(saveSlots_.size())) saveSlots_[slot] = snapshot;
}

void Game::handleInput(const InputState &input) {
    if (input.pressed) {
        if (state_ == GameState::Title) {
            if (input.y >= 255.0f && input.y <= 485.0f) {
                const int slot = std::clamp(static_cast<int>(input.x / 120.0f), 0, 2);
                if (saveSlots_[slot].occupied) loadRun(slot);
                else startRun(slot);
            }
            return;
        }
        if (state_ == GameState::GameOver) {
            if (input.y >= 475.0f) state_ = GameState::Title;
            else startRun(selectedSlot_ >= 0 ? selectedSlot_ : 0);
            return;
        }
        if (state_ == GameState::Paused) {
            state_ = pausedFrom_;
            return;
        }
        if (state_ == GameState::UpgradeSelect && input.y >= 250.0f && input.y <= 570.0f) {
            if (input.x < 120.0f) chooseUpgrade(UpgradeType::Damage);
            else if (input.x < 240.0f) chooseUpgrade(UpgradeType::FireRate);
            else chooseUpgrade(UpgradeType::MaxHealth);
            return;
        }
    }
    if ((state_ != GameState::Playing && state_ != GameState::BossFight) || !input.down) {
        return;
    }
    playerX_ = std::clamp(playerX_ + input.deltaX, 27.0f, 333.0f);
    playerY_ = std::clamp(playerY_ + input.deltaY, kWorldTop + 14.0f, 606.0f);
    const float maximumTilt = 8.0f * kPi / 180.0f;
    playerTilt_ = std::clamp(input.deltaX * 0.012f, -maximumTilt, maximumTilt);
}

void Game::pause() {
    if (state_ == GameState::Playing || state_ == GameState::BossIntro ||
        state_ == GameState::BossFight) {
        pausedFrom_ = state_;
        state_ = GameState::Paused;
        requestSaveSlot();
    }
}

float Game::randomFloat(float minimum, float maximum) {
    randomState_ = randomState_ * 1664525u + 1013904223u;
    const float unit = static_cast<float>((randomState_ >> 8) & 0x00ffffffu) /
                       static_cast<float>(0x01000000u);
    return minimum + (maximum - minimum) * unit;
}

bool Game::collides(float ax, float ay, float ar, float bx, float by, float br) {
    const float dx = ax - bx;
    const float dy = ay - by;
    const float radius = ar + br;
    return dx * dx + dy * dy <= radius * radius;
}

void Game::beginBossIntro() {
    state_ = GameState::BossIntro;
    bossIntroTimer_ = kBossIntroDuration;
    boss_.active = true;
    boss_.type = static_cast<BossType>(progress_.bossesDefeated % 4);
    boss_.x = 180.0f;
    boss_.y = -56.0f;
    const float direction = progress_.bossesDefeated % 2 == 0 ? 1.0f : -1.0f;
    boss_.vx = (boss_.type == BossType::Hunter ? 76.0f
                : (boss_.type == BossType::Tempest ? 58.0f : 42.0f)) * direction;
    boss_.maxHealth = 800.0f * kEnemyHealthMultiplier *
                      scaleForStage(progress_.bossesDefeated, 0.4f);
    boss_.health = boss_.maxHealth;
    boss_.phase = 1;
    boss_.age = 0.0f;
    boss_.shootTimer = 1.0f;
    boss_.burstShots = 0;
    clearCombatObjects();
    pendingEvents_ |= EventBossWarning;
}

void Game::beginBossFight() {
    state_ = GameState::BossFight;
    boss_.y = 118.0f;
    boss_.shootTimer = 0.9f;
    boss_.burstTimer = 0.0f;
    boss_.burstShots = 0;
}

void Game::clearCombatObjects() {
    for (auto &enemy : enemies_) enemy.active = false;
    for (auto &bullet : enemyBullets_) bullet.active = false;
    for (auto &pickup : pickups_) pickup.active = false;
}

void Game::spawnEnemy() {
    Enemy *enemy = firstInactive(enemies_);
    if (!enemy) return;
    const int stage = progress_.bossesDefeated;
    const float roll = randomFloat(0.0f, 1.0f);
    if (roll < 0.36f) enemy->type = EnemyType::Scout;
    else if (roll < (stage > 0 ? 0.68f : 0.76f)) enemy->type = EnemyType::Snake;
    else if (roll < (stage > 0 ? 0.90f : 0.94f)) enemy->type = EnemyType::Turret;
    else enemy->type = EnemyType::Flanker;

    const float damageScale = scaleForStage(stage, 0.15f);
    const float speedScale = scaleForStage(stage, 0.08f);
    enemy->active = true;
    enemy->x = randomFloat(30.0f, 330.0f);
    enemy->originX = enemy->x;
    enemy->y = -42.0f;
    enemy->age = 0.0f;
    enemy->hitFlash = 0.0f;
    if (enemy->type == EnemyType::Scout) {
        enemy->health = enemyHealthFor(enemy->type, stage);
        enemy->vy = 170.0f * speedScale;
        enemy->vx = randomFloat(-20.0f, 20.0f) * speedScale;
        enemy->radius = 16.0f;
        enemy->contactDamage = 20.0f * damageScale;
        enemy->shootTimer = 999.0f;
    } else if (enemy->type == EnemyType::Snake) {
        enemy->health = enemyHealthFor(enemy->type, stage);
        enemy->vy = 92.0f * speedScale;
        enemy->vx = 0.0f;
        enemy->radius = 19.0f;
        enemy->contactDamage = 20.0f * damageScale;
        enemy->shootTimer = randomFloat(1.2f, 3.5f);
    } else if (enemy->type == EnemyType::Turret) {
        enemy->health = enemyHealthFor(enemy->type, stage);
        enemy->vy = 54.0f * speedScale;
        enemy->vx = randomFloat(-12.0f, 12.0f) * speedScale;
        enemy->radius = 23.0f;
        enemy->contactDamage = 20.0f * damageScale;
        enemy->shootTimer = randomFloat(0.6f, 2.2f);
    } else {
        enemy->health = enemyHealthFor(enemy->type, stage);
        enemy->x = randomFloat(0.0f, 1.0f) < 0.5f ? -28.0f : 388.0f;
        enemy->originX = enemy->x;
        enemy->y = randomFloat(120.0f, 280.0f);
        enemy->vy = 112.0f * speedScale;
        enemy->vx = enemy->x < 0.0f ? 112.0f * speedScale : -112.0f * speedScale;
        enemy->radius = 14.0f;
        enemy->contactDamage = 24.0f * damageScale;
        enemy->shootTimer = randomFloat(1.4f, 2.8f);
    }
}

void Game::spawnPickup(bool allowAdvanced, float x, float y) {
    Pickup *pickup = firstInactive(pickups_);
    if (!pickup) return;
    const float healWeight = currentHealth_ < playerStats_.maxHealth * 0.4f ? 0.52f : 0.22f;
    const float roll = randomFloat(0.0f, 1.0f);
    if (allowAdvanced && roll < kPermanentDropChance) {
        const float advancedRoll = randomFloat(0.0f, 1.0f);
        pickup->type = advancedRoll < 0.34f
                           ? PickupType::PermanentDamage
                           : (advancedRoll < 0.67f ? PickupType::PermanentFireRate
                                                   : PickupType::PermanentMaxHealth);
    } else {
        const float normalRoll = allowAdvanced ? randomFloat(0.0f, 1.0f) : roll;
        pickup->type = normalRoll < 0.28f ? PickupType::Shield
                                          : (normalRoll < 0.28f + healWeight
                                                 ? PickupType::Heal
                                                 : PickupType::AttackBoost);
    }
    pickup->active = true;
    pickup->x = x < 0.0f ? randomFloat(24.0f, 336.0f) : x;
    pickup->y = y;
    pickup->vy = 64.0f;
    pickup->age = 0.0f;
}

void Game::spawnPermanentPickup(float x, float y) {
    Pickup *pickup = firstInactive(pickups_);
    if (!pickup) return;
    const float roll = randomFloat(0.0f, 1.0f);
    pickup->type = roll < 0.34f ? PickupType::PermanentDamage
                   : (roll < 0.67f ? PickupType::PermanentFireRate
                                   : PickupType::PermanentMaxHealth);
    pickup->active = true;
    pickup->x = std::clamp(x, 24.0f, 336.0f);
    pickup->y = std::clamp(y, 120.0f, 570.0f);
    pickup->vy = 42.0f;
    pickup->age = 0.0f;
}

void Game::spawnPlayerBullet() {
    Bullet *bullet = firstInactive(playerBullets_);
    if (!bullet) return;
    bullet->active = true;
    bullet->boosted = attackBoostTime_ > 0.0f;
    bullet->x = playerX_;
    bullet->y = playerY_ - 35.0f;
    bullet->vx = 0.0f;
    bullet->vy = -430.0f;
    bullet->radius = 4.0f;
    bullet->damage = playerStats_.damage * (bullet->boosted ? 2.0f : 1.0f);
    bullet->age = 0.0f;
    pendingEvents_ |= EventShoot;
}

void Game::spawnEnemyBullet(float x, float y, float angle, float speed, float damage) {
    Bullet *bullet = firstInactive(enemyBullets_);
    if (!bullet) return;
    bullet->active = true;
    bullet->boosted = false;
    bullet->x = x;
    bullet->y = y;
    bullet->vx = std::cos(angle) * speed;
    bullet->vy = std::sin(angle) * speed;
    bullet->radius = 6.0f;
    bullet->damage = damage;
    bullet->age = 0.0f;
}

void Game::spawnAimedEnemyBullet(float x, float y, float speed, float damage, float angleOffset) {
    const float angle = std::atan2(playerY_ - y, playerX_ - x) + angleOffset;
    spawnEnemyBullet(x, y, angle, speed, damage);
}

void Game::spawnExplosion(float x, float y, float scale) {
    Explosion *explosion = firstInactive(explosions_);
    if (!explosion) return;
    explosion->active = true;
    explosion->x = x;
    explosion->y = y;
    explosion->age = 0.0f;
    explosion->scale = scale;
    pendingEvents_ |= EventExplosion;
}

void Game::spawnPickupEffect(PickupType type, float x, float y) {
    PickupEffect *effect = firstInactive(pickupEffects_);
    if (!effect) return;
    effect->active = true;
    effect->type = type;
    effect->x = x;
    effect->y = y;
    effect->age = 0.0f;
}

void Game::applyPickup(PickupType type) {
    if (type == PickupType::Shield) {
        shieldTime_ = 10.0f;
    } else if (type == PickupType::Heal) {
        currentHealth_ = std::min(playerStats_.maxHealth, currentHealth_ + 25.0f);
    } else if (type == PickupType::AttackBoost) {
        attackBoostTime_ = 8.0f;
    } else if (type == PickupType::PermanentDamage) {
        playerStats_.damage += 5.0f;
        upgradeAnimation_ = 1.2f;
        lastUpgrade_ = UpgradeType::Damage;
    } else if (type == PickupType::PermanentFireRate) {
        playerStats_.fireInterval = std::max(0.12f, playerStats_.fireInterval * 0.9f);
        upgradeAnimation_ = 1.2f;
        lastUpgrade_ = UpgradeType::FireRate;
    } else if (type == PickupType::PermanentMaxHealth) {
        playerStats_.maxHealth += 100.0f;
        currentHealth_ = std::min(playerStats_.maxHealth, currentHealth_ + 100.0f);
        upgradeAnimation_ = 1.2f;
        lastUpgrade_ = UpgradeType::MaxHealth;
    }
    spawnPickupEffect(type, playerX_, playerY_);
    pendingEvents_ |= EventPickup;
}

void Game::damagePlayer(float damage, float x, float y) {
    if (state_ != GameState::Playing && state_ != GameState::BossFight) return;
    if (invulnerable_ > 0.0f) return;
    if (shieldTime_ > 0.0f) {
        shieldTime_ = 0.0f;
        invulnerable_ = 0.25f;
    } else {
        currentHealth_ -= damage;
        invulnerable_ = 1.0f;
    }
    shakeTime_ = 0.25f;
    spawnExplosion(x, y, 0.7f);
    pendingEvents_ |= EventHit;
    if (currentHealth_ <= 0.0f) finishRun();
}

void Game::killEnemy(Enemy &enemy) {
    if (!enemy.active) return;
    enemy.active = false;
    ++progress_.kills;
    progress_.combo = comboTimer_ > 0.0f ? progress_.combo + 1 : 1;
    comboTimer_ = 2.5f;
    const int base = enemy.type == EnemyType::Scout ? 50
                     : (enemy.type == EnemyType::Snake ? 90
                        : (enemy.type == EnemyType::Turret ? 160 : 120));
    const int multiplier = 10 + std::min(progress_.combo - 1, 9);
    progress_.scoreBonus += base * multiplier / 10;
    spawnExplosion(enemy.x, enemy.y, enemy.type == EnemyType::Turret ? 1.2f : 0.8f);
    const float dropChance = enemy.type == EnemyType::Turret ? 0.22f
                              : (enemy.type == EnemyType::Snake ? 0.14f
                                 : (enemy.type == EnemyType::Flanker ? 0.12f : 0.08f));
    if (randomFloat(0.0f, 1.0f) < dropChance) spawnPickup(false, enemy.x, enemy.y);
}

void Game::damageBoss(float damage) {
    if (!boss_.active || state_ != GameState::BossFight) return;
    boss_.health -= damage;
    boss_.hitFlash = 0.08f;
    if (boss_.health <= 0.0f) defeatBoss();
}

void Game::defeatBoss() {
    if (!boss_.active) return;
    spawnExplosion(boss_.x, boss_.y, 2.4f);
    boss_.active = false;
    progress_.scoreBonus += 2500 * (progress_.bossesDefeated + 1);
    ++progress_.bossesDefeated;
    progress_.normalCombatTime = 0.0f;
    updateScore();
    clearCombatObjects();
    spawnPermanentPickup(boss_.x, boss_.y + 48.0f);
    state_ = GameState::UpgradeSelect;
    pendingEvents_ |= EventBossDefeat;
}

void Game::chooseUpgrade(UpgradeType type) {
    if (state_ != GameState::UpgradeSelect) return;
    lastUpgrade_ = type;
    if (type == UpgradeType::Damage) {
        playerStats_.damage *= 1.25f;
    } else if (type == UpgradeType::FireRate) {
        playerStats_.fireInterval = std::max(0.12f, playerStats_.fireInterval * 0.85f);
    } else {
        playerStats_.maxHealth += 100.0f;
        currentHealth_ = std::min(playerStats_.maxHealth, currentHealth_ + 100.0f);
    }
    upgradeAnimation_ = 1.2f;
    progress_.normalCombatTime = 0.0f;
    enemyTimer_ = 1.0f;
    pickupTimer_ = randomFloat(8.0f, 14.0f);
    state_ = GameState::Playing;
    pendingEvents_ |= EventUpgrade;
    requestSaveSlot();
}

void Game::finishRun() {
    state_ = GameState::GameOver;
    currentHealth_ = std::max(0.0f, currentHealth_);
    updateScore();
    if (score_ > highScore_) {
        highScore_ = score_;
        pendingEvents_ |= EventSaveHighScore;
    }
    pendingEvents_ |= EventExplosion;
}

void Game::updateTimers(float deltaSeconds) {
    invulnerable_ = std::max(0.0f, invulnerable_ - deltaSeconds);
    shieldTime_ = std::max(0.0f, shieldTime_ - deltaSeconds);
    attackBoostTime_ = std::max(0.0f, attackBoostTime_ - deltaSeconds);
    comboTimer_ = std::max(0.0f, comboTimer_ - deltaSeconds);
    upgradeAnimation_ = std::max(0.0f, upgradeAnimation_ - deltaSeconds);
    shakeTime_ = std::max(0.0f, shakeTime_ - deltaSeconds);
    if (comboTimer_ <= 0.0f) progress_.combo = 0;
}

void Game::updatePlayerFire(float deltaSeconds) {
    fireTimer_ -= deltaSeconds;
    const float interval = playerStats_.fireInterval * (attackBoostTime_ > 0.0f ? 0.5f : 1.0f);
    while (fireTimer_ <= 0.0f) {
        spawnPlayerBullet();
        fireTimer_ += interval;
    }
}

void Game::updateEnemies(float deltaSeconds) {
    const float enemyDamageScale = scaleForStage(progress_.bossesDefeated, 0.15f);
    for (auto &enemy : enemies_) {
        if (!enemy.active) continue;
        enemy.age += deltaSeconds;
        enemy.hitFlash = std::max(0.0f, enemy.hitFlash - deltaSeconds);
        if (enemy.type == EnemyType::Flanker) {
            enemy.x += enemy.vx * deltaSeconds;
            enemy.y += enemy.vy * deltaSeconds;
            enemy.shootTimer -= deltaSeconds;
            if (enemy.shootTimer <= 0.0f) {
                spawnAimedEnemyBullet(enemy.x, enemy.y + 10.0f, 168.0f, 8.0f * enemyDamageScale);
                enemy.shootTimer += 2.4f;
            }
            if (enemy.x < -48.0f || enemy.x > 408.0f) enemy.active = false;
        } else if (enemy.type == EnemyType::Snake) {
            enemy.x = enemy.originX + std::sin(enemy.age * 3.0f) * 54.0f;
        } else {
            enemy.x += enemy.vx * deltaSeconds;
            if (enemy.x < 20.0f || enemy.x > 340.0f) enemy.vx = -enemy.vx;
        }
        if (enemy.type != EnemyType::Flanker) enemy.y += enemy.vy * deltaSeconds;
        if (enemy.type != EnemyType::Flanker) enemy.shootTimer -= deltaSeconds;
        if (enemy.type == EnemyType::Snake && enemy.shootTimer <= 0.0f) {
            spawnAimedEnemyBullet(enemy.x, enemy.y + 18.0f, 162.0f, 10.0f * enemyDamageScale);
            enemy.shootTimer += 3.5f;
        } else if (enemy.type == EnemyType::Turret && enemy.shootTimer <= 0.0f) {
            for (int i = -1; i <= 1; ++i) {
                spawnAimedEnemyBullet(enemy.x, enemy.y + 18.0f, 146.0f, 10.0f * enemyDamageScale,
                                      static_cast<float>(i) * 0.18f);
            }
            enemy.shootTimer += 2.2f;
        }
        if (enemy.y > 690.0f) enemy.active = false;
        if (enemy.active && collides(playerX_, playerY_, kPlayerRadius, enemy.x, enemy.y,
                                     enemy.radius)) {
            const float damage = enemy.contactDamage;
            enemy.active = false;
            damagePlayer(damage, playerX_, playerY_);
        }
    }
}

void Game::updateBullets(float deltaSeconds) {
    for (auto &bullet : playerBullets_) {
        if (!bullet.active) continue;
        bullet.age += deltaSeconds;
        bullet.x += bullet.vx * deltaSeconds;
        bullet.y += bullet.vy * deltaSeconds;
        if (bullet.y < -30.0f) {
            bullet.active = false;
            continue;
        }
        if (state_ == GameState::BossFight && boss_.active &&
            collides(bullet.x, bullet.y, bullet.radius, boss_.x, boss_.y,
                     bossCollisionRadius(boss_.type))) {
            const float damage = bullet.damage;
            bullet.active = false;
            pendingEvents_ |= EventImpact;
            damageBoss(damage);
            continue;
        }
        for (auto &enemy : enemies_) {
            if (!enemy.active) continue;
            if (collides(bullet.x, bullet.y, bullet.radius, enemy.x, enemy.y, enemy.radius)) {
                enemy.health -= bullet.damage;
                enemy.hitFlash = 0.08f;
                bullet.active = false;
                pendingEvents_ |= EventImpact;
                if (enemy.health <= 0.0f) killEnemy(enemy);
                break;
            }
        }
    }
    for (auto &bullet : enemyBullets_) {
        if (!bullet.active) continue;
        bullet.age += deltaSeconds;
        bullet.x += bullet.vx * deltaSeconds;
        bullet.y += bullet.vy * deltaSeconds;
        if (bullet.x < -40.0f || bullet.x > 400.0f || bullet.y < -50.0f || bullet.y > 690.0f) {
            bullet.active = false;
            continue;
        }
        if (collides(playerX_, playerY_, kPlayerRadius, bullet.x, bullet.y, bullet.radius)) {
            const float damage = bullet.damage;
            bullet.active = false;
            damagePlayer(damage, playerX_, playerY_);
        }
    }
}

void Game::updatePickups(float deltaSeconds) {
    for (auto &pickup : pickups_) {
        if (!pickup.active) continue;
        pickup.age += deltaSeconds;
        pickup.y += pickup.vy * deltaSeconds;
        if (pickup.y > 680.0f) pickup.active = false;
        if (pickup.active && collides(playerX_, playerY_, kPlayerRadius, pickup.x, pickup.y, 14.0f)) {
            const PickupType type = pickup.type;
            pickup.active = false;
            applyPickup(type);
        }
    }
}

void Game::updateBoss(float deltaSeconds) {
    if (!boss_.active) return;
    boss_.hitFlash = std::max(0.0f, boss_.hitFlash - deltaSeconds);
    boss_.phaseFlash = std::max(0.0f, boss_.phaseFlash - deltaSeconds);
    boss_.age += deltaSeconds;

    const float ratio = boss_.health / std::max(1.0f, boss_.maxHealth);
    const int phase = ratio > 0.66f ? 1 : (ratio > 0.33f ? 2 : 3);
    if (phase != boss_.phase) {
        boss_.phase = phase;
        boss_.phaseFlash = 0.45f;
        pendingEvents_ |= EventBossPhase;
    }
    const float damage = 19.0f * scaleForStage(std::min(progress_.bossesDefeated, 4), 0.11f);
    if (boss_.type == BossType::Carrier) {
        boss_.x += boss_.vx * deltaSeconds;
        if (boss_.x < 78.0f || boss_.x > 282.0f) boss_.vx = -boss_.vx;
        if (boss_.phase == 1) {
            boss_.shootTimer -= deltaSeconds;
            if (boss_.shootTimer <= 0.0f) {
                spawnAimedEnemyBullet(boss_.x, boss_.y + 34.0f, 188.0f, damage);
                boss_.shootTimer += 0.95f;
            }
        } else if (boss_.phase == 2) {
            boss_.shootTimer -= deltaSeconds;
            if (boss_.shootTimer <= 0.0f) {
                for (int i = -2; i <= 2; ++i) {
                    spawnAimedEnemyBullet(boss_.x, boss_.y + 34.0f, 176.0f, damage,
                                          static_cast<float>(i) * 0.13f);
                }
                boss_.shootTimer += 1.12f;
            }
        } else {
            if (boss_.burstShots > 0) {
                boss_.burstTimer -= deltaSeconds;
                if (boss_.burstTimer <= 0.0f) {
                    spawnAimedEnemyBullet(boss_.x, boss_.y + 34.0f, 194.0f, damage);
                    --boss_.burstShots;
                    boss_.burstTimer += 0.13f;
                }
            } else {
                boss_.shootTimer -= deltaSeconds;
                if (boss_.shootTimer <= 0.0f) {
                    boss_.burstShots = 7;
                    boss_.burstTimer = 0.0f;
                    boss_.shootTimer += 0.9f;
                }
            }
        }
    } else if (boss_.type == BossType::Hunter) {
        boss_.x += boss_.vx * deltaSeconds;
        if (boss_.x < 66.0f || boss_.x > 294.0f) boss_.vx = -boss_.vx;
        boss_.y = 116.0f + std::sin(boss_.age * 2.1f) * 12.0f;
        boss_.shootTimer -= deltaSeconds;
        if (boss_.shootTimer <= 0.0f) {
            if (boss_.phase == 1) {
                spawnAimedEnemyBullet(boss_.x - 24.0f, boss_.y + 30.0f, 210.0f, damage, -0.10f);
                spawnAimedEnemyBullet(boss_.x + 24.0f, boss_.y + 30.0f, 210.0f, damage, 0.10f);
                boss_.shootTimer += 0.92f;
            } else if (boss_.phase == 2) {
                for (int i = -1; i <= 1; ++i) {
                    spawnAimedEnemyBullet(boss_.x, boss_.y + 34.0f, 218.0f, damage,
                                          static_cast<float>(i) * 0.22f);
                }
                boss_.shootTimer += 0.70f;
            } else {
                spawnAimedEnemyBullet(boss_.x - 22.0f, boss_.y + 32.0f, 232.0f, damage, -0.16f);
                spawnAimedEnemyBullet(boss_.x + 22.0f, boss_.y + 32.0f, 232.0f, damage, 0.16f);
                spawnEnemyBullet(boss_.x, boss_.y + 24.0f, kPi * 0.25f, 188.0f, damage);
                spawnEnemyBullet(boss_.x, boss_.y + 24.0f, kPi * 0.75f, 188.0f, damage);
                boss_.shootTimer += 0.56f;
            }
        }
    } else if (boss_.type == BossType::Prism) {
        boss_.x = 180.0f + std::sin(boss_.age * 0.85f) * 102.0f;
        boss_.y = 116.0f + std::cos(boss_.age * 1.7f) * 14.0f;
        boss_.shootTimer -= deltaSeconds;
        if (boss_.shootTimer <= 0.0f) {
            const int projectileCount = 4 + boss_.phase * 2;
            const float angleOffset = boss_.age * 0.9f;
            for (int i = 0; i < projectileCount; ++i) {
                const float angle = angleOffset + 2.0f * kPi * static_cast<float>(i) /
                                                    static_cast<float>(projectileCount);
                spawnEnemyBullet(boss_.x, boss_.y + 10.0f, angle,
                                 138.0f + boss_.phase * 14.0f, damage * 0.72f);
            }
            if (boss_.phase == 3) {
                spawnAimedEnemyBullet(boss_.x, boss_.y + 30.0f, 214.0f, damage);
            }
            boss_.shootTimer += boss_.phase == 1 ? 1.35f : (boss_.phase == 2 ? 1.05f : 0.82f);
        }
    } else {
        boss_.x = 180.0f + std::sin(boss_.age * 1.25f) * 118.0f;
        boss_.y = 118.0f + std::sin(boss_.age * 2.4f) * 20.0f;
        boss_.shootTimer -= deltaSeconds;
        if (boss_.shootTimer <= 0.0f) {
            const int count = boss_.phase == 3 ? 10 : (boss_.phase == 2 ? 8 : 6);
            for (int i = 0; i < count; ++i) {
                const float angle = boss_.age * 1.1f + 2.0f * kPi * i / count;
                spawnEnemyBullet(boss_.x, boss_.y + 12.0f, angle, 145.0f + boss_.phase * 12.0f,
                                 damage * 0.75f);
            }
            if (boss_.phase >= 2) spawnAimedEnemyBullet(boss_.x, boss_.y + 28.0f, 220.0f, damage);
            boss_.shootTimer += boss_.phase == 1 ? 1.3f : (boss_.phase == 2 ? 0.95f : 0.7f);
        }
    }
    if (collides(playerX_, playerY_, kPlayerRadius, boss_.x, boss_.y,
                 bossCollisionRadius(boss_.type))) {
        damagePlayer(22.0f * scaleForStage(std::min(progress_.bossesDefeated, 4), 0.08f), playerX_, playerY_);
    }
}

void Game::updateEffects(float deltaSeconds) {
    for (auto &explosion : explosions_) {
        if (!explosion.active) continue;
        explosion.age += deltaSeconds;
        if (explosion.age >= 0.5f) explosion.active = false;
    }
    for (auto &effect : pickupEffects_) {
        if (!effect.active) continue;
        effect.age += deltaSeconds;
        if (effect.age >= 0.8f) effect.active = false;
    }
}

void Game::updateScore() {
    score_ = static_cast<int>(progress_.survivalTime * 10.0f) + progress_.scoreBonus;
}

void Game::update(float deltaSeconds) {
    deltaSeconds = std::clamp(deltaSeconds, 0.0f, 0.25f);
    backgroundTravel_ += (state_ == GameState::Title ? 18.0f : 58.0f) * deltaSeconds;
    updateEffects(deltaSeconds);
    if (state_ == GameState::Title || state_ == GameState::Paused ||
        state_ == GameState::UpgradeSelect || state_ == GameState::GameOver) {
        return;
    }

    progress_.survivalTime += deltaSeconds;
    updateTimers(deltaSeconds);
    playerTilt_ += (0.0f - playerTilt_) * std::min(1.0f, deltaSeconds * 6.0f);

    if (state_ == GameState::BossIntro) {
        bossIntroTimer_ -= deltaSeconds;
        const float progress = 1.0f - std::max(0.0f, bossIntroTimer_) / kBossIntroDuration;
        boss_.y = -56.0f + 174.0f * std::min(1.0f, progress * 1.3f);
        if (bossIntroTimer_ <= 0.0f) beginBossFight();
        updateScore();
        return;
    }

    if (state_ == GameState::Playing) {
        progress_.normalCombatTime += deltaSeconds;
        if (progress_.normalCombatTime >= kNormalBossInterval) {
            progress_.normalCombatTime = kNormalBossInterval;
            beginBossIntro();
            updateScore();
            return;
        }
        enemyTimer_ -= deltaSeconds;
        pickupTimer_ -= deltaSeconds;
        int activeEnemies = 0;
        for (const auto &enemy : enemies_) activeEnemies += enemy.active ? 1 : 0;
        const int enemyLimit = kBaseEnemyLimit + std::min(progress_.bossesDefeated / 2, 2);
        if (enemyTimer_ <= 0.0f && activeEnemies < enemyLimit) {
            spawnEnemy();
            const float stage = static_cast<float>(progress_.bossesDefeated);
            enemyTimer_ += std::max(0.48f, 1.42f - stage * 0.08f) + randomFloat(0.0f, 0.28f);
        }
        if (pickupTimer_ <= 0.0f) {
            spawnPickup();
            pickupTimer_ = randomFloat(8.0f, 14.0f);
        }
    }

    updatePlayerFire(deltaSeconds);
    updateEnemies(deltaSeconds);
    updateBullets(deltaSeconds);
    updatePickups(deltaSeconds);
    if (state_ == GameState::BossFight) updateBoss(deltaSeconds);
    updateScore();
}

std::size_t Game::activePlayerBullets() const {
    std::size_t count = 0;
    for (const auto &bullet : playerBullets_) count += bullet.active ? 1u : 0u;
    return count;
}

std::size_t Game::activeBoostedPlayerBullets() const {
    std::size_t count = 0;
    for (const auto &bullet : playerBullets_) count += bullet.active && bullet.boosted ? 1u : 0u;
    return count;
}

std::size_t Game::activeEnemyBullets() const {
    std::size_t count = 0;
    for (const auto &bullet : enemyBullets_) count += bullet.active ? 1u : 0u;
    return count;
}

std::size_t Game::activePickupEffects() const {
    std::size_t count = 0;
    for (const auto &effect : pickupEffects_) count += effect.active ? 1u : 0u;
    return count;
}

#ifdef CANYON_HEADLESS_TEST
float Game::debugEnemyHealth(EnemyType type, int stage) const {
    return enemyHealthFor(type, stage);
}

std::size_t Game::debugActiveEnemies() const {
    std::size_t count = 0;
    for (const auto &enemy : enemies_) count += enemy.active ? 1u : 0u;
    return count;
}

float Game::debugPermanentDropChance() const {
    return kPermanentDropChance;
}

void Game::debugDamageBoss(float damage) {
    if (!boss_.active) beginBossIntro();
    boss_.active = true;
    state_ = GameState::BossFight;
    boss_.y = 118.0f;
    damageBoss(damage);
}

void Game::debugApplyPickup(PickupType type) {
    applyPickup(type);
}

void Game::debugDamagePlayer(float damage) {
    damagePlayer(damage, playerX_, playerY_);
}
#endif

#ifndef CANYON_HEADLESS_TEST
void Game::render(Renderer &renderer, const GameAssets &assets) const {
    const float backgroundHeight = kCanvasWidth * assets.background->height() /
                                   static_cast<float>(assets.background->width());
    const int baseIndex = static_cast<int>(std::floor(backgroundTravel_ / backgroundHeight));
    const float localOffset = std::fmod(backgroundTravel_, backgroundHeight);
    for (int slot = -2; slot <= 2; ++slot) {
        const int imageIndex = baseIndex + slot;
        const float y = localOffset + slot * backgroundHeight;
        const bool flipped = (std::abs(imageIndex) % 2) == 1;
        renderer.drawSprite(*assets.background, kCanvasWidth * 0.5f, y + backgroundHeight * 0.5f,
                            kCanvasWidth, backgroundHeight, 0.0f, {}, 0.0f,
                            flipped ? 1.0f : 0.0f, 1.0f, flipped ? 0.0f : 1.0f);
    }
    const float shakeX = shakeTime_ > 0.0f ? std::sin(progress_.survivalTime * 80.0f) * 2.2f : 0.0f;
    const float shakeY = shakeTime_ > 0.0f ? std::cos(progress_.survivalTime * 73.0f) * 1.5f : 0.0f;
    renderer.drawRect(180.0f, 320.0f, 360.0f, 640.0f, {0.0f, 0.05f, 0.03f, 0.12f});

    for (const auto &pickup : pickups_) {
        if (!pickup.active) continue;
        const float pulse = 1.0f + 0.08f * std::sin(pickup.age * 7.0f);
        const TextureAsset *texture = nullptr;
        if (pickup.type == PickupType::Shield) texture = assets.pickupShield.get();
        else if (pickup.type == PickupType::Heal) texture = assets.pickupHeal.get();
        else if (pickup.type == PickupType::AttackBoost) texture = assets.pickupAttack.get();
        else if (pickup.type == PickupType::PermanentDamage) texture = assets.upgradeDamage.get();
        else if (pickup.type == PickupType::PermanentFireRate) texture = assets.upgradeFire.get();
        else texture = assets.upgradeHealth.get();
        renderer.drawSprite(*texture, pickup.x + shakeX, pickup.y + shakeY,
                            (pickup.type >= PickupType::PermanentDamage ? 32.0f : 28.0f) * pulse,
                            (pickup.type >= PickupType::PermanentDamage ? 32.0f : 28.0f) * pulse);
    }
    for (const auto &effect : pickupEffects_) {
        if (!effect.active) continue;
        const float progress = std::min(1.0f, effect.age / 0.8f);
        const float alpha = 1.0f - progress;
        Color color{0.35f, 0.9f, 1.0f, alpha};
        const TextureAsset *icon = assets.pickupShield.get();
        if (effect.type == PickupType::Heal) {
            color = {0.3f, 1.0f, 0.48f, alpha};
            icon = assets.pickupHeal.get();
        } else if (effect.type == PickupType::AttackBoost) {
            color = {1.0f, 0.2f, 0.08f, alpha};
            icon = assets.pickupAttack.get();
        } else if (effect.type == PickupType::PermanentDamage) {
            color = {1.0f, 0.68f, 0.12f, alpha};
            icon = assets.upgradeDamage.get();
        } else if (effect.type == PickupType::PermanentFireRate) {
            color = {1.0f, 0.68f, 0.12f, alpha};
            icon = assets.upgradeFire.get();
        } else if (effect.type == PickupType::PermanentMaxHealth) {
            color = {1.0f, 0.68f, 0.12f, alpha};
            icon = assets.upgradeHealth.get();
        }
        const float ringSize = 28.0f + progress * 92.0f;
        renderer.drawSprite(*assets.effectRing, effect.x + shakeX, effect.y + shakeY,
                            ringSize, ringSize, progress * 1.2f, color);
        renderer.drawSprite(*icon, effect.x + shakeX,
                            effect.y + shakeY - 8.0f - progress * 28.0f,
                            24.0f, 24.0f, 0.0f, {1.0f, 1.0f, 1.0f, alpha});
    }
    for (const auto &enemy : enemies_) {
        if (!enemy.active) continue;
        const auto &texture = enemy.type == EnemyType::Scout
                                     ? *assets.enemyScout
                                     : (enemy.type == EnemyType::Snake ? *assets.enemySnake
                                        : (enemy.type == EnemyType::Turret ? *assets.enemyTurret
                                                                            : *assets.enemyFlanker));
        const float size = enemy.type == EnemyType::Turret ? 58.0f
                           : (enemy.type == EnemyType::Flanker ? 44.0f : 50.0f);
        renderer.drawSprite(texture, enemy.x + shakeX, enemy.y + shakeY, size, size,
                            0.0f, enemy.hitFlash > 0.0f ? Color{1.0f, 1.0f, 1.0f, 1.0f}
                                                         : Color{});
        if (enemy.hitFlash > 0.0f) {
            renderer.drawRect(enemy.x + shakeX, enemy.y + shakeY, size * 0.75f, size * 0.75f,
                              {1.0f, 1.0f, 1.0f, 0.42f});
        }
    }
    if (boss_.active) {
        const TextureAsset *bossTexture = assets.bossCarrier.get();
        float bossWidth = 140.0f;
        float bossHeight = 110.0f;
        if (boss_.type == BossType::Hunter) {
            bossTexture = assets.bossHunter.get();
            bossWidth = 122.0f;
            bossHeight = 120.0f;
        } else if (boss_.type == BossType::Prism) {
            bossTexture = assets.bossPrism.get();
            bossWidth = 116.0f;
            bossHeight = 116.0f;
        } else if (boss_.type == BossType::Tempest) {
            bossTexture = assets.bossTempest.get();
            bossWidth = 132.0f;
            bossHeight = 118.0f;
        }
        renderer.drawSprite(*bossTexture, boss_.x + shakeX, boss_.y + shakeY,
                            bossWidth, bossHeight,
                            0.0f, boss_.hitFlash > 0.0f ? Color{1.0f, 1.0f, 1.0f, 1.0f}
                                                         : Color{});
    }
    for (const auto &bullet : playerBullets_) {
        if (!bullet.active) continue;
        if (bullet.boosted) {
            renderer.drawRect(bullet.x + shakeX, bullet.y + shakeY + 12.0f,
                              5.0f, 22.0f, {1.0f, 0.1f, 0.04f, 0.32f});
            renderer.drawSprite(*assets.boostedPlayerBullet, bullet.x + shakeX,
                                bullet.y + shakeY, 10.0f, 22.0f);
        } else {
            renderer.drawSprite(*assets.playerBullet, bullet.x + shakeX,
                                bullet.y + shakeY, 8.0f, 18.0f);
        }
    }
    for (const auto &bullet : enemyBullets_) {
        if (bullet.active) renderer.drawSprite(*assets.enemyBullet, bullet.x + shakeX,
                                                bullet.y + shakeY, 13.0f, 13.0f);
    }
    for (const auto &explosion : explosions_) {
        if (!explosion.active) continue;
        const int frame = std::min(5, static_cast<int>(explosion.age / 0.5f * 6.0f));
        renderer.drawSprite(*assets.explosion, explosion.x + shakeX, explosion.y + shakeY,
                            54.0f * explosion.scale, 54.0f * explosion.scale, 0.0f, {},
                            frame / 6.0f, 0.0f, (frame + 1) / 6.0f, 1.0f);
    }

    if (shieldTime_ > 0.0f && state_ != GameState::GameOver) {
        const float pulse = 1.0f + std::sin(progress_.survivalTime * 7.0f) * 0.045f;
        renderer.drawSprite(*assets.shieldAura, playerX_ + shakeX, playerY_ + shakeY,
                            84.0f * pulse, 96.0f * pulse,
                            std::sin(progress_.survivalTime * 2.0f) * 0.025f,
                            {0.24f, 0.88f, 1.0f, 0.72f});
    }
    if (attackBoostTime_ > 0.0f && state_ != GameState::GameOver) {
        const float pulse = 72.0f + std::sin(progress_.survivalTime * 10.0f) * 6.0f;
        renderer.drawSprite(*assets.effectRing, playerX_ + shakeX, playerY_ + shakeY,
                            pulse, pulse, progress_.survivalTime * 0.8f,
                            {1.0f, 0.16f, 0.05f, 0.42f});
    }
    const bool visible = invulnerable_ <= 0.0f || static_cast<int>(invulnerable_ * 14.0f) % 2 == 0;
    if (visible && state_ != GameState::GameOver) {
        renderer.drawSprite(*assets.player, playerX_ + shakeX, playerY_ + shakeY, 61.0f, 70.0f,
                            playerTilt_);
    }

    renderer.drawRect(180.0f, 30.0f, 360.0f, 60.0f, {0.01f, 0.04f, 0.035f, 0.84f});
    renderer.drawText(*assets.font, "HP", 7.0f, 7.0f, 1.0f, {0.82f, 1.0f, 0.88f, 1.0f});
    renderer.drawRect(61.0f, 14.0f, 102.0f, 9.0f, {0.16f, 0.08f, 0.08f, 1.0f});
    const float healthRatio = std::min(1.0f, currentHealth_ / playerStats_.maxHealth);
    renderer.drawRect(10.0f + 51.0f * healthRatio, 14.0f, 102.0f * healthRatio,
                      9.0f, currentHealth_ < playerStats_.maxHealth * 0.35f
                                  ? Color{0.95f, 0.2f, 0.16f, 1.0f}
                                  : Color{0.22f, 0.88f, 0.44f, 1.0f});
    char hpBuffer[32];
    std::snprintf(hpBuffer, sizeof(hpBuffer), "%d/%d", health(),
                  static_cast<int>(playerStats_.maxHealth + 0.5f));
    renderer.drawText(*assets.font, hpBuffer, 112.0f, 7.0f, 0.9f, {}, TextAlign::Center);
    char scoreBuffer[32];
    std::snprintf(scoreBuffer, sizeof(scoreBuffer), "%06d", score_);
    renderer.drawText(*assets.font, scoreBuffer, 352.0f, 7.0f, 1.25f,
                      {1.0f, 0.87f, 0.35f, 1.0f}, TextAlign::Right);
    char statsBuffer[64];
    std::snprintf(statsBuffer, sizeof(statsBuffer), "DMG %.0f  FIRE %.2f", playerStats_.damage,
                  playerStats_.fireInterval * (attackBoostTime_ > 0.0f ? 0.5f : 1.0f));
    renderer.drawText(*assets.font, statsBuffer, 7.0f, 29.0f, 0.82f,
                      {0.65f, 0.9f, 0.98f, 1.0f});
    char bossBuffer[32];
    std::snprintf(bossBuffer, sizeof(bossBuffer), "BOSS %d", progress_.bossesDefeated + 1);
    renderer.drawText(*assets.font, bossBuffer, 250.0f, 29.0f, 0.82f,
                      {1.0f, 0.7f, 0.38f, 1.0f});
    if (shieldTime_ > 0.0f) {
        renderer.drawSprite(*assets.shield, 18.0f, 53.0f, 16.0f, 16.0f);
        char timer[16]; std::snprintf(timer, sizeof(timer), "%.1f", shieldTime_);
        renderer.drawText(*assets.font, timer, 28.0f, 49.0f, 0.78f, {});
    }
    if (attackBoostTime_ > 0.0f) {
        renderer.drawSprite(*assets.pickupAttack, 78.0f, 53.0f, 16.0f, 16.0f);
        char timer[16]; std::snprintf(timer, sizeof(timer), "%.1f", attackBoostTime_);
        renderer.drawText(*assets.font, timer, 88.0f, 49.0f, 0.78f, {});
    }
    if (boss_.active && (state_ == GameState::BossIntro || state_ == GameState::BossFight)) {
        const float bossHealthRatio = boss_.health / std::max(1.0f, boss_.maxHealth);
        renderer.drawRect(198.0f, 77.0f, 264.0f, 8.0f, {0.18f, 0.04f, 0.06f, 1.0f});
        renderer.drawRect(66.0f + 132.0f * bossHealthRatio,
                          77.0f, 264.0f * bossHealthRatio, 8.0f,
                          {0.88f, 0.16f, 0.24f, 1.0f});
        const char *bossName = boss_.type == BossType::Carrier
                                       ? "CARRIER"
                                       : (boss_.type == BossType::Hunter
                                              ? "HUNTER"
                                              : (boss_.type == BossType::Prism ? "PRISM" : "TEMPEST"));
        renderer.drawText(*assets.font, bossName, 7.0f, 71.0f, 0.78f,
                          {1.0f, 0.52f, 0.4f, 1.0f});
    }

    if (state_ == GameState::BossIntro) {
        renderer.drawRect(180.0f, 305.0f, 360.0f, 80.0f, {0.38f, 0.02f, 0.03f, 0.72f});
        renderer.drawText(*assets.font, "WARNING", 180.0f, 282.0f, 3.0f,
                          {1.0f, 0.76f, 0.25f, 1.0f}, TextAlign::Center);
        renderer.drawText(*assets.font, "BOSS INBOUND", 180.0f, 330.0f, 1.5f,
                          {1.0f, 0.9f, 0.72f, 1.0f}, TextAlign::Center);
        return;
    }
    if (state_ == GameState::Playing || state_ == GameState::BossFight) {
        if (upgradeAnimation_ > 0.0f) {
            renderer.drawText(*assets.font, "UPGRADE", 180.0f, 112.0f, 1.35f,
                              {0.45f, 1.0f, 0.58f, upgradeAnimation_}, TextAlign::Center);
        }
        return;
    }

    renderer.drawRect(180.0f, 320.0f, 360.0f, 640.0f, {0.0f, 0.025f, 0.02f, 0.76f});
    if (state_ == GameState::Title) {
        renderer.drawText(*assets.font, "CANYON", 180.0f, 112.0f, 3.5f,
                          {1.0f, 0.76f, 0.25f, 1.0f}, TextAlign::Center);
        renderer.drawText(*assets.font, "BREAKOUT", 180.0f, 148.0f, 3.5f,
                          {0.88f, 1.0f, 0.92f, 1.0f}, TextAlign::Center);
        renderer.drawText(*assets.font, "INFINITE RUN", 180.0f, 204.0f, 1.35f,
                          {0.48f, 0.9f, 1.0f, 1.0f}, TextAlign::Center);
        const float centers[] = {60.0f, 180.0f, 300.0f};
        for (int i = 0; i < 3; ++i) {
            const SaveSnapshot &slot = saveSlots_[i];
            renderer.drawRect(centers[i], 345.0f, 104.0f, 170.0f,
                              {0.05f, 0.12f, 0.14f, 0.98f});
            renderer.drawRect(centers[i], 263.0f, 100.0f, 3.0f,
                              {0.28f, 0.82f, 0.8f, 1.0f});
            char line[40];
            std::snprintf(line, sizeof(line), "SLOT %d", i + 1);
            renderer.drawText(*assets.font, line, centers[i], 288.0f, 0.82f,
                              {0.85f, 1.0f, 0.96f, 1.0f}, TextAlign::Center);
            if (slot.occupied) {
                std::snprintf(line, sizeof(line), "BOSSES %d", slot.bossesDefeated);
                renderer.drawText(*assets.font, line, centers[i], 324.0f, 0.68f,
                                  {1.0f, 0.76f, 0.3f, 1.0f}, TextAlign::Center);
                std::snprintf(line, sizeof(line), "SCORE %06d", slot.score);
                renderer.drawText(*assets.font, line, centers[i], 350.0f, 0.58f, {}, TextAlign::Center);
                if (slot.lastPlayedEpoch > 0) {
                    const std::time_t played = static_cast<std::time_t>(slot.lastPlayedEpoch);
                    const std::tm *local = std::localtime(&played);
                    if (local) {
                        std::snprintf(line, sizeof(line), "LAST %02d:%02d", local->tm_hour, local->tm_min);
                        renderer.drawText(*assets.font, line, centers[i], 375.0f, 0.58f, {}, TextAlign::Center);
                    }
                }
                renderer.drawText(*assets.font, "TAP LOAD", centers[i], 405.0f, 0.68f,
                                  {0.48f, 0.9f, 1.0f, 1.0f}, TextAlign::Center);
            } else {
                renderer.drawText(*assets.font, "EMPTY", centers[i], 337.0f, 0.82f,
                                  {0.58f, 0.7f, 0.72f, 1.0f}, TextAlign::Center);
                renderer.drawText(*assets.font, "TAP NEW", centers[i], 405.0f, 0.68f,
                                  {0.48f, 0.9f, 1.0f, 1.0f}, TextAlign::Center);
            }
        }
        renderer.drawText(*assets.font, "SELECT A SLOT", 180.0f, 470.0f, 1.15f,
                          {0.45f, 0.95f, 1.0f, 1.0f}, TextAlign::Center);
    } else if (state_ == GameState::Paused) {
        renderer.drawText(*assets.font, "PAUSED", 180.0f, 280.0f, 4.0f,
                          {1.0f, 0.9f, 0.55f, 1.0f}, TextAlign::Center);
        renderer.drawText(*assets.font, "TAP TO CONTINUE", 180.0f, 350.0f, 1.5f,
                          {0.75f, 1.0f, 1.0f, 1.0f}, TextAlign::Center);
    } else if (state_ == GameState::UpgradeSelect) {
        renderer.drawText(*assets.font, "CHOOSE UPGRADE", 180.0f, 116.0f, 2.1f,
                          {1.0f, 0.82f, 0.35f, 1.0f}, TextAlign::Center);
        const float centers[] = {60.0f, 180.0f, 300.0f};
        const TextureAsset *icons[] = {assets.upgradeDamage.get(), assets.upgradeFire.get(),
                                       assets.upgradeHealth.get()};
        const char *labels[] = {"DAMAGE", "FIRE RATE", "MAX HP"};
        const char *values[] = {"+25%", "+15%", "+100 HP"};
        for (int i = 0; i < 3; ++i) {
            renderer.drawRect(centers[i], 310.0f, 106.0f, 190.0f,
                              {0.05f, 0.12f, 0.14f, 0.98f});
            renderer.drawRect(centers[i], 218.0f, 104.0f, 3.0f,
                              {0.28f, 0.82f, 0.8f, 1.0f});
            renderer.drawSprite(*icons[i], centers[i], 265.0f, 34.0f, 34.0f);
            renderer.drawText(*assets.font, labels[i], centers[i], 330.0f, 0.9f,
                              {0.85f, 1.0f, 0.96f, 1.0f}, TextAlign::Center);
            renderer.drawText(*assets.font, values[i], centers[i], 360.0f, 1.1f,
                              {1.0f, 0.76f, 0.3f, 1.0f}, TextAlign::Center);
            renderer.drawText(*assets.font, "TAP", centers[i], 420.0f, 0.82f,
                              {0.48f, 0.9f, 1.0f, 1.0f}, TextAlign::Center);
        }
    } else {
        renderer.drawText(*assets.font, "RUN OVER", 180.0f, 185.0f, 3.2f,
                          {1.0f, 0.44f, 0.28f, 1.0f}, TextAlign::Center);
        char line[48];
        std::snprintf(line, sizeof(line), "TIME %02d:%02d", static_cast<int>(progress_.survivalTime) / 60,
                      static_cast<int>(progress_.survivalTime) % 60);
        renderer.drawText(*assets.font, line, 180.0f, 245.0f, 1.25f, {}, TextAlign::Center);
        std::snprintf(line, sizeof(line), "KILLS %d", progress_.kills);
        renderer.drawText(*assets.font, line, 180.0f, 272.0f, 1.25f, {}, TextAlign::Center);
        std::snprintf(line, sizeof(line), "BOSSES %d", progress_.bossesDefeated);
        renderer.drawText(*assets.font, line, 180.0f, 299.0f, 1.25f, {}, TextAlign::Center);
        std::snprintf(line, sizeof(line), "SCORE %06d", score_);
        renderer.drawText(*assets.font, line, 180.0f, 342.0f, 1.65f,
                          {1.0f, 0.84f, 0.35f, 1.0f}, TextAlign::Center);
        std::snprintf(line, sizeof(line), "BEST %06d", highScore_);
        renderer.drawText(*assets.font, line, 180.0f, 376.0f, 1.25f, {}, TextAlign::Center);
        renderer.drawText(*assets.font, "TAP TO RESTART", 180.0f, 440.0f, 1.5f,
                          {0.5f, 0.95f, 1.0f, 1.0f}, TextAlign::Center);
        renderer.drawRect(180.0f, 510.0f, 150.0f, 34.0f, {0.05f, 0.14f, 0.16f, 0.95f});
        renderer.drawText(*assets.font, "SAVE SLOTS", 180.0f, 502.0f, 1.1f,
                          {0.65f, 1.0f, 0.95f, 1.0f}, TextAlign::Center);
    }
}
#endif

uint32_t Game::consumeEvents() {
    const uint32_t events = pendingEvents_;
    pendingEvents_ = EventNone;
    return events;
}
