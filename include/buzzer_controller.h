//
// Created by neltri on 1/20/24.
//

#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H
#define BUZZER_PIN 15
#include <neotimer.h>

class BuzzerController {
public:
    BuzzerController();

    void buzzTimerPoll();

    void buzz(int time);

private:
    Neotimer buzzTimer;
};

#endif //BUZZER_CONTROLLER_H
