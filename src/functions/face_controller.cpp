//
// Created by neltri on 1/20/24.
//

#include <face_controller.h>
#include <faces.h>
#include <lcd_controller.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define BITMAP_HEIGHT 128
#define BITMAP_WIDTH 32
#define OLED_RST (-1)

extern LCDController lcdController;

void FaceController::displayFace() {
  if (frameCount == frameStop && faceMode != 0) {
    lcdController.changeLcdText("Ready!");
    faceMode = 0;
    frameCount = 0;
  }

  if (frame == allArray_LEN[faceMode] - 1) {
    frame = 0;
    if (faceMode != 0) {
      frameCount += 1;
    }
  }

  display.clearDisplay();
  const unsigned char **faceArray;
  int faceArraySize;
  switch (faceMode) {
    case 0:
      faceArray = idle_face_allArray;
      faceArraySize = sizeof(idle_face_allArray) / 4;
      break;
    case 1:
      faceArray = welcome_face_allArray;
      faceArraySize = sizeof(welcome_face_allArray) / 4;
      break;
    case 2:
      faceArray = late_face_allArray;
      faceArraySize = sizeof(late_face_allArray) / 4;
      break;
    case 3:
      faceArray = invalid_face_allArray;
      faceArraySize = sizeof(invalid_face_allArray) / 4;
      break;
    default:
      faceArray = idle_face_allArray;
      faceArraySize = sizeof(idle_face_allArray);
      break;
  }

  if (frame < faceArraySize) {
    if (frame <= 16) {
      display.drawBitmap(0, 0, faceArray[frame], 128, 32, WHITE);
    } else {
      frame = 0;
    }
  }

  display.display();
  frame += 1;
}

FaceController::FaceController() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST) {
  this->faceMode = 1; // Initialize faceMode
  this->frame = 0; // Initialize frame
  this->frameStop = 2; // Initialize frameStop
  this->frameCount = 0; // Initialize frameCount

  // Initialize Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}
