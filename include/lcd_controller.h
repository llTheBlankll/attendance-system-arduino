//
// Created by neltri on 1/20/24.
//

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class LCDController {

public:
    LCDController();

    void initializeLCD();

    void changeLcdText(const String &text);

private:
    LiquidCrystal_I2C lcd;
    String currentText;
};

#endif //FUNCTIONS_H
