#include <cassert>
#include <cmath>
#include <iostream>

#include "Game.h"

namespace {
void step(Game &game, float seconds) {
    while (seconds > 0.0f) {
        const float amount = seconds > 0.2f ? 0.2f : seconds;
        game.update(amount);
        seconds -= amount;
    }
}
}

int main() {
    Game game(7);
    assert(game.state() == GameState::Title);
    assert(game.highScore() == 7);

    InputState start;
    start.pressed = true;
    start.down = true;
    start.x = 90.0f;
    start.y = 330.0f;
    game.handleInput(start);
    assert(game.state() == GameState::Playing);
    assert(game.health() == 100);
    assert(std::abs(game.playerStats().damage - 20.0f) < 0.001f);
    assert(std::abs(game.playerStats().fireInterval - 0.45f) < 0.001f);

    InputState move;
    move.down = true;
    move.deltaX = 1000.0f;
    move.deltaY = -1000.0f;
    game.handleInput(move);
    assert(std::abs(game.playerX() - 333.0f) < 0.001f);
    assert(std::abs(game.playerY() - 98.0f) < 0.001f);

    InputState moveDown;
    moveDown.down = true;
    moveDown.deltaY = 300.0f;
    game.handleInput(moveDown);
    step(game, 0.15f);
    assert(game.activePlayerBullets() > 0);
    assert((game.consumeEvents() & EventShoot) != 0);
    for (int i = 0; i < 80; ++i) step(game, 0.2f);
    assert(game.activePlayerBullets() <= 64);

    game.pause();
    assert(game.state() == GameState::Paused);
    step(game, 2.0f);
    assert(game.progress().survivalTime < 20.0f);
    InputState resume;
    resume.pressed = true;
    game.handleInput(resume);
    assert(game.state() == GameState::Playing);

    game.debugSetNormalCombatTime(119.9f);
    step(game, 0.2f);
    assert(game.state() == GameState::BossIntro);
    assert((game.consumeEvents() & EventBossWarning) != 0);
    step(game, 2.3f);
    assert(game.state() == GameState::BossFight);
    assert(game.boss().active);
    assert(std::abs(game.boss().maxHealth - 800.0f) < 0.1f);

    game.debugDamageBoss(800.0f);
    assert(game.state() == GameState::UpgradeSelect);
    assert(game.progress().bossesDefeated == 1);
    assert((game.consumeEvents() & EventBossDefeat) != 0);
    InputState chooseDamage;
    chooseDamage.pressed = true;
    chooseDamage.x = 60.0f;
    chooseDamage.y = 300.0f;
    game.handleInput(chooseDamage);
    assert(game.state() == GameState::Playing);
    assert(std::abs(game.playerStats().damage - 25.0f) < 0.01f);

    Game pickupGame(0);
    pickupGame.handleInput(start);
    pickupGame.debugApplyPickup(PickupType::Shield);
    assert(pickupGame.shieldTime() > 9.9f);
    assert(pickupGame.activePickupEffects() > 0);
    pickupGame.debugDamagePlayer(999.0f);
    assert(pickupGame.health() == 100);
    assert(pickupGame.shieldTime() < 0.01f);
    step(pickupGame, 0.3f);
    pickupGame.debugDamagePlayer(35.0f);
    assert(pickupGame.health() == 65);
    pickupGame.debugApplyPickup(PickupType::Heal);
    assert(pickupGame.health() == 90);
    pickupGame.debugApplyPickup(PickupType::AttackBoost);
    assert(pickupGame.attackBoostTime() > 7.9f);
    step(pickupGame, 0.25f);
    assert(pickupGame.activeBoostedPlayerBullets() > 0);
    pickupGame.debugApplyPickup(PickupType::PermanentDamage);
    assert(std::abs(pickupGame.playerStats().damage - 25.0f) < 0.01f);
    pickupGame.debugApplyPickup(PickupType::PermanentFireRate);
    assert(pickupGame.playerStats().fireInterval < 0.45f);
    pickupGame.debugApplyPickup(PickupType::PermanentMaxHealth);
    assert(std::abs(pickupGame.playerStats().maxHealth - 115.0f) < 0.01f);

    step(game, 1.0f);
    game.debugDamagePlayer(999.0f);
    assert(game.state() == GameState::GameOver);
    const int recordedHigh = game.highScore();
    assert(recordedHigh >= 7);
    assert((game.consumeEvents() & EventSaveHighScore) != 0 || recordedHigh == 7);

    InputState restart;
    restart.pressed = true;
    game.handleInput(restart);
    assert(game.state() == GameState::Playing);
    assert(game.health() == 100);
    assert(game.progress().bossesDefeated == 0);
    assert(game.progress().kills == 0);
    assert(game.score() == 0);
    assert(std::abs(game.playerStats().damage - 20.0f) < 0.001f);
    assert(std::abs(game.playerStats().fireInterval - 0.45f) < 0.001f);
    assert(game.highScore() == recordedHigh);

    SaveSnapshot slot;
    slot.occupied = true;
    slot.bossesDefeated = 3;
    slot.score = 12345;
    slot.scoreBonus = 9000;
    slot.kills = 80;
    slot.survivalTime = 340.0f;
    slot.normalCombatTime = 40.0f;
    slot.currentHealth = 125.0f;
    slot.playerStats.maxHealth = 140.0f;
    slot.playerStats.damage = 37.5f;
    slot.playerStats.fireInterval = 0.31f;
    Game loadedGame(0);
    loadedGame.setSaveSlot(1, slot);
    InputState loadSlot;
    loadSlot.pressed = true;
    loadSlot.x = 270.0f;
    loadSlot.y = 330.0f;
    loadedGame.handleInput(loadSlot);
    assert(loadedGame.state() == GameState::Playing);
    assert(loadedGame.selectedSlot() == 1);
    assert(loadedGame.progress().bossesDefeated == 3);
    assert(loadedGame.score() == 12345);
    assert(std::abs(loadedGame.playerStats().damage - 37.5f) < 0.01f);
    loadedGame.pause();
    assert(loadedGame.saveSnapshot().occupied);
    assert(loadedGame.saveSnapshot().bossesDefeated == 3);
    assert((loadedGame.consumeEvents() & EventSaveSlot) != 0);

    std::cout << "Infinite mode GameLogicTest passed" << std::endl;
    return 0;
}
