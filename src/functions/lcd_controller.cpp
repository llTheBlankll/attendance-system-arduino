//
// Created by neltri on 1/20/24.
//

#include <functions.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

String currentText = "Need restart";

void changeLcdText(const String &text, String currentText) {
    currentText = text;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(currentText);
}
