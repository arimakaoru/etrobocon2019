/**
 *  @file   Calibrator.cpp
 *  @brief  キャリブレーションを行うクラス
 *  @author korosuke613
 **/
#include "Calibrator.h"

Calibrator::Calibrator(Controller& controller_)
  : controller(controller_),
    isCameraMode(true),
    isLeft(true),
    brightnessOfWhite(0),
    brightnessOfBlack(0)
{
}

bool Calibrator::calibration()
{
  Display::print(2, "now calibration...");

  if(!setCameraMode()) {
    Display::print(2, "Error setCameraMode!");
    return false;
  }

  if(!setLRCourse()) {
    Display::print(2, "Error setLRCourse!");
    return false;
  }

  if(!setBrightness(Brightness::WHITE)) {
    Display::print(2, "Error setBrightness White!");
    return false;
  }

  if(!setBrightness(Brightness::BLACK)) {
    Display::print(2, "Error setBrightness Black!");
    return false;
  }

  Display::print(5, "White: %3d", brightnessOfWhite);
  Display::print(6, "Black: %3d", brightnessOfBlack);

  // 精度が悪いため使用しない。今後、精度を上げて使用しても良い。
  // if(!setupPosition()) return false;

  return true;
}

bool Calibrator::setCameraMode()
{
  char cameraMode[8] = "ON";

  controller.tslpTsk(500);
  while(!controller.buttonIsPressedEnter()) {
    if(isCameraMode) {
      std::strcpy(cameraMode, "ON");
    } else {
      std::strcpy(cameraMode, "OFF");
    }
    Display::print(3, "camera system: %s ?", cameraMode);

    if(controller.buttonIsPressedLeft() || controller.buttonIsPressedRight()) {
      isCameraMode = !isCameraMode;
      controller.speakerPlayToneFS6(50);
      controller.tslpTsk(500);
    }
    controller.tslpTsk(4);
  }
  Display::print(3, "camera system: %s", cameraMode);

  controller.speakerPlayToneFS6(100);
  return true;
}

bool Calibrator::setLRCourse()
{
  char course[8] = "Left";

  controller.tslpTsk(500);
  while(!controller.buttonIsPressedEnter()) {
    if(isLeft) {
      std::strcpy(course, "Left");
    } else {
      std::strcpy(course, "Right");
    }
    Display::print(4, "Set LRCourse: %s ?", course);

    if(controller.buttonIsPressedLeft() || controller.buttonIsPressedRight()) {
      isLeft = !isLeft;
      controller.speakerPlayToneFS6(50);
      controller.tslpTsk(500);
    }
    controller.tslpTsk(4);
  }
  Display::print(4, "course: %s", course);

  controller.speakerPlayToneFS6(100);
  return true;
}

bool Calibrator::setupPosition()
{
  int r, g, b, currentBrightness;
  int tragetBrightness = (brightnessOfBlack + brightnessOfWhite) / 2;
  LineTracer lineTracer(controller, tragetBrightness, isLeft);
  LineTracer reverseLineTracer(controller, tragetBrightness, !isLeft);

  NormalCourseProperty normalCourseProperty = { 10, 100, { 0.1, 0.0, 0.0 }, { 0.12, 0.0, 0.10 } };

  // 緑までライントレースしながら進む
  while(!controller.buttonIsPressedLeft()) {
  }
  while(true) {
    reverseLineTracer.run(normalCourseProperty);
    controller.getRawColor(r, g, b);
    controller.convertHsv(r, g, b);
    if(controller.hsvToColor(controller.getHsv()) == Color::green) {
      controller.stopMotor();
      break;
    }
    controller.tslpTsk(4);
  }
  controller.speakerPlayToneFS6(100);

  Navigator navigator(controller);
  // バック
  navigator.move(-80, 10);
  // 180度回転
  navigator.spin(180);
  // ライントレースしながら進む
  NormalCourseProperty normalCourseProperty2 = { 170, 100, { 0.1, 0.0, 0.0 }, { 0.12, 0.0, 0.10 } };
  lineTracer.run(normalCourseProperty2);
  // 光センサーの値が(白+黒)/2 ± 5になるように走行体の位置を微調整する
  while(true) {
    currentBrightness = controller.getBrightness();
    if(tragetBrightness - 5 <= currentBrightness && currentBrightness <= tragetBrightness + 5) {
      break;
    } else if(currentBrightness < tragetBrightness) {
      controller.setLeftMotorPwm(0);
      controller.setRightMotorPwm(1);
    } else {
      controller.setLeftMotorPwm(1);
      controller.setRightMotorPwm(0);
    }
    controller.tslpTsk(4);
  }

  controller.stopMotor();
  controller.speakerPlayToneFS6(100);
  // Display::print(7, "%d %d", tragetBrightness, currentBrightness);

  return true;
}

bool Calibrator::setBrightness(Brightness brightness)
{
  char name[8] = "none";

  if(brightness == Brightness::WHITE) {
    std::strcpy(name, "White");
  } else if(brightness == Brightness::BLACK) {
    std::strcpy(name, "Black");
  } else {
    return false;
  }

  controller.tslpTsk(500);

  while(1) {
    // ENTERボタンが押されたらループを抜ける
    if(controller.buttonIsPressedEnter()) {
      controller.speakerPlayToneFS6(100);
      break;
    }

    int tmpColor = controller.getBrightness();
    Display::print(5, "Set brightness of %s: %3d ?", name, tmpColor);

    controller.tslpTsk(4);
  }

  controller.speakerPlayToneFS6(200);

  if(brightness == Brightness::WHITE) {
    brightnessOfWhite = averageBrightness();
  } else {
    brightnessOfBlack = averageBrightness();
  }

  return true;
}

int Calibrator::averageBrightness()
{
  // 4ms毎に10回明るさを取得して、その平均値をメンバ変数に代入する処理
  int meanBrightness = 0;
  int times = 10;
  for(int i = 0; i < times; i++) {
    meanBrightness += controller.getBrightness();
    controller.tslpTsk(4);
  }

  return meanBrightness / times;
}

bool Calibrator::getCameraMode() const
{
  return isCameraMode;
}

bool Calibrator::isLeftCourse() const
{
  return isLeft;
}

int Calibrator::getWhiteBrightness() const
{
  return brightnessOfWhite;
};

int Calibrator::getBlackBrightness() const
{
  return brightnessOfBlack;
};
