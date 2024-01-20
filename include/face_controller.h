//
// Created by neltri on 1/20/24.
//

#ifndef FACE_CONTROLLER_H
#define FACE_CONTROLLER_H

#include <Adafruit_SSD1306.h>

class FaceController {
public:
  int faceMode;

  void displayFace();

  FaceController();

private:
  Adafruit_SSD1306 display;
  int frame;
  int frameStop;
  int frameCount;
};

#endif //FACE_CONTROLLER_H
