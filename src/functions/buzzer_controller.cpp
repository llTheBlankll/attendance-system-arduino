//
// Created by neltri on 1/20/24.
//

#include <Arduino.h>
#include <buzzer_controller.h>
#define BUZZER_PIN 15

void BuzzerController::buzz(const int time) {
  tone(BUZZER_PIN, 1000, time);
}

BuzzerController::BuzzerController() = default;
