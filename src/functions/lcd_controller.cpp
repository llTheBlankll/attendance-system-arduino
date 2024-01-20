//
// Created by neltri on 1/20/24.
//

#include <lcd_controller.h>
#include <LiquidCrystal_I2C.h>

void LCDController::changeLcdText(const String &text) {
    currentText = text;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(currentText);
}

void LCDController::initializeLCD() {
    lcd.clear();
    lcd.init();
    lcd.backlight();
}

LCDController::LCDController() : lcd(0x27, 16, 2), currentText("Need restart") {
    // Constructor body (if needed)
}