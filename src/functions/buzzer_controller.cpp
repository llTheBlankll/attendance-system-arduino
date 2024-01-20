//
// Created by neltri on 1/20/24.
//

#include <buzzer_controller.h>

Neotimer buzzTimer(1000);

void BuzzerController::buzzTimerPoll() {
    if (buzzTimer.done()) {
        noTone(BUZZER_PIN);
        buzzTimer.reset();
    }
}

void BuzzerController::buzz(const int time) {
    buzzTimer.set(time);
    buzzTimer.start();

    if (buzzTimer.done()) {
        noTone(BUZZER_PIN);
    }

    if (buzzTimer.waiting()) {
        tone(BUZZER_PIN, 1000);
    }
}

BuzzerController::BuzzerController() : buzzTimer(1000) {
}
