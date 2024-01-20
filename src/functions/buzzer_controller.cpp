//
// Created by neltri on 1/20/24.
//

#include <neotimer.h>
#include "buzzer_controller.h"
#include <Arduino.h>

Neotimer buzzTimer(1000);


void BuzzerController::buzzTimerPoll() {
    if (buzzTimer.done()) {
        noTone(BUZZER_PIN);
        buzzTimer.reset();
    }
}

void buzz(const int time) {
    buzzTimer.set(time);
    buzzTimer.start();

    if (buzzTimer.done()) {
        noTone(BUZZER_PIN);
    }

    if (buzzTimer.waiting()) {
        tone(BUZZER_PIN, 1000);
    }
}

void BuzzerController() {
    pinMode(BUZZER_PIN, OUTPUT);
}
