/*
 * Legacy Lego robot project.
 * Original code developed for an EV3-like robotics IDE using evclibrary.h.
 *
 * Notes:
 * - Hardware and test track are no longer available.
 * - The original library is unavailable because it was bundled directly with the IDE.
 * - This file is preserved mainly for archival and portfolio purposes.
 */

#include "evclibrary.h"

// colors: 0-none, 1-schwarz, 2-blau, 3-gruen, 4-gelb, 5-rot, 6-weiss, 7-braun
// lighting: 100-dark...0-bright
#define COLOR_GRAB_PATH 3
#define COLOR_PATH 5
#define COLOR_GROUND 1

// distance in cm
#define WALL_DISTANCE 25
#define WALL_DISTANCE2 30

// speed in %
#define FORWARD_POWER 70
#define BACKWARD_POWER 40

// motor
#define MOTOR_UNCHANGED 101

// sensorcodes
enum Const
{
  NONE,
  GROUND,
  PATH,
  GRAB_PATH,
  OBSTACLE,
  OBSTACLE2,
  CONTACT
};

// already grabbed?
bool grabbed = false;

// -----------------------------------------------------------------------------
// Movement
// -----------------------------------------------------------------------------
void doControlMotor(int powerA, int powerB, int powerC)
{
  if (powerA <= 100)
  {
    WRITE_OUT(OUT_A, MOTOR_POWER, powerA);
    LCD_DRAW_INT(9, 5, powerA);
  }
  if (powerB <= 100)
  {
    WRITE_OUT(OUT_B, MOTOR_POWER, powerB);
    LCD_DRAW_INT(9, 6, powerB);
  }
  if (powerC <= 100)
  {
    WRITE_OUT(OUT_C, MOTOR_POWER, powerC);
    LCD_DRAW_INT(9, 7, powerC);
  }
}
void doStop()
{
  doControlMotor(0, 0, 0);
  SLEEP(1);
}

void doQuaterRight(bool right = true)
{
  if (right == false)
  {
    doControlMotor(-100, 100, MOTOR_UNCHANGED);
    SLEEP(1);
  }
  if (right == true)
  {
    doControlMotor(100, -100, MOTOR_UNCHANGED);
    SLEEP(1);
  }
  doControlMotor(0, 0, MOTOR_UNCHANGED);
}

void doOpenClaw()
{
  for (int i = 0; i < 500; i++)
  {
    doControlMotor(MOTOR_UNCHANGED, MOTOR_UNCHANGED, -100);
  }
  doControlMotor(0, 0, 0);
}
void doCloseClaw()
{
  for (int i = 0; i < 600; i++)
  {
    doControlMotor(MOTOR_UNCHANGED, MOTOR_UNCHANGED, 100);
  }
  doControlMotor(0, 0, 0);
}

// -----------------------------------------------------------------------------
// Sensors
// -----------------------------------------------------------------------------
int getSensorLeft()
{
  switch (READ_IN(IN_1))
  {
  case COLOR_PATH:
    LCD_DRAW_INT(9, 1, COLOR_PATH);
    return PATH;
  case COLOR_GROUND:
    LCD_DRAW_INT(9, 1, COLOR_GROUND);
    return GROUND;
  case COLOR_GRAB_PATH:
    if (grabbed)
    {
      return NONE;
    }
    else
    {
      LCD_DRAW_INT(9, 1, COLOR_GRAB_PATH);
      return GRAB_PATH;
    }
  default:
    return NONE;
  }
}

int getSensorRight()
{
  switch (READ_IN(IN_2))
  {
  case COLOR_PATH:
    LCD_DRAW_INT(9, 2, COLOR_PATH);
    return PATH;
  case COLOR_GROUND:
    LCD_DRAW_INT(9, 2, COLOR_GROUND);
    return GROUND;
  case COLOR_GRAB_PATH:
    if (grabbed)
    {
      return NONE;
    }
    else
    {
      LCD_DRAW_INT(9, 2, COLOR_GRAB_PATH);
      return GRAB_PATH;
    }
  default:
    return NONE;
  }
}

int getSensorObstacle()
{
  if (READ_IN(IN_3) <= WALL_DISTANCE && READ_IN(IN_3) >= 15)
  {
    LCD_DRAW_INT(9, 3, 1);
    return OBSTACLE;
  }
  else if (READ_IN(IN_3) <= WALL_DISTANCE2 && READ_IN(IN_3) >= 15)
  {
    LCD_DRAW_INT(9, 3, 2);
    return OBSTACLE2;
  }
  else
  {
    LCD_DRAW_INT(9, 3, 0);
    return NONE
  }
}

int getSensorGrab()
{
  if (READ_IN(IN_4) == 1)
  {
    return CONTACT;
  }
  return NONE;
}

// -----------------------------------------------------------------------------
// Behavior: obstacle avoidance
// -----------------------------------------------------------------------------
bool isObstacleRight()
{
  bool obstacleRight;
  LCD_DRAW_TEXT(1, 10, "calculating...     ");
  doControlMotor(0, 0, 0);
  SLEEP(1);
  // Neuer Blickwinkel
  doControlMotor(-10, -60, MOTOR_UNCHANGED);
  SLEEP(1);
  // Messen
  doControlMotor(0, 0, 0);
  if (getSensorObstacle() == OBSTACLE2 || getSensorObstacle() == OBSTACLE)
  {
    obstacleRight = true;
    LCD_DRAW_TEXT(1, 10, "goLeftSide      ");
  }
  else
  {
    obstacleRight = false;
    LCD_DRAW_TEXT(1, 10, "goRightSide       ");
  }
  // zum Ausgangspunkt
  doControlMotor(10, 50, MOTOR_UNCHANGED);
  SLEEP(1);
  doControlMotor(0, 0, 0);
  return obstacleRight;
}

// -----------------------------------------------------------------------------
// Behavior: follow path
// -----------------------------------------------------------------------------
void doFollowPath()
{
  if (getSensorLeft() != PATH && getSensorRight() != PATH)
  {
    doControlMotor(FORWARD_POWER, FORWARD_POWER, MOTOR_UNCHANGED);
  }
  if (getSensorLeft() == PATH && getSensorRight() != PATH)
  {
    doControlMotor(-BACKWARD_POWER, FORWARD_POWER, MOTOR_UNCHANGED);
  }
  if (getSensorLeft() != PATH && getSensorRight() == PATH)
  {
    doControlMotor(FORWARD_POWER, -BACKWARD_POWER, MOTOR_UNCHANGED);
  }
}

// -----------------------------------------------------------------------------
// Behavior: follow path leading to object
// -----------------------------------------------------------------------------
void doFollowGrabPath()
{
  if (getSensorLeft() != GRAB_PATH && getSensorRight() != GRAB_PATH)
  {
    doControlMotor(50, 50, MOTOR_UNCHANGED);
  }
  if (getSensorLeft() == GRAB_PATH && getSensorRight() != GRAB_PATH)
  {
    doControlMotor(-40, 50, MOTOR_UNCHANGED);
  }
  if (getSensorLeft() != GRAB_PATH && getSensorRight() == GRAB_PATH)
  {
    doControlMotor(50, -40, MOTOR_UNCHANGED);
  }
}

// -----------------------------------------------------------------------------
// Behavior: avoid obstacle
// -----------------------------------------------------------------------------
void doDodge(bool right = isObstacleRight(), bool scnd = false)
{
  LCD_DRAW_TEXT(9, 8, "doDodge     ");
  // if obstacle right:
  if (right == true)
  {
    LCD_DRAW_TEXT(1, 10, "goLeftSide      ");
    LCD_DRAW_TEXT(9, 8, "doDodgeLeft   ");
    while (getSensorObstacle() == OBSTACLE && NOTEXITBUTTON)
    {
      doControlMotor(-100, -20, MOTOR_UNCHANGED);
    }
    doControlMotor(-80, -20, MOTOR_UNCHANGED);
    SLEEP(1);
    if (scnd == false)
    {
      while ((getSensorRight() == PATH || getSensorLeft() == PATH) && NOTEXITBUTTON)
      {
        doControlMotor(100, 100, MOTOR_UNCHANGED);
      }
      doControlMotor(100, 100, MOTOR_UNCHANGED);
      SLEEP(400);
    }
    if (scnd == true)
    {
      doControlMotor(40, 20, MOTOR_UNCHANGED);
      SLEEP(1);
      while (!(getSensorRight() == PATH && getSensorLeft() == PATH) && NOTEXITBUTTON)
      {
        doFollowPath();
      }
      doControlMotor(30, 30, MOTOR_UNCHANGED);
      SLEEP(1);
    }
    // return to path
    while (getSensorLeft() != PATH && NOTEXITBUTTON)
    {
      doControlMotor(100, 35, MOTOR_UNCHANGED);
      if (getSensorObstacle() == OBSTACLE || getSensorObstacle() == OBSTACLE2)
      {
        doDodge(false, true);
        break;
      }
    }
    while (getSensorLeft() == PATH && getSensorRight() == PATH && NOTEXITBUTTON)
    {
      doControlMotor(-BACKWARD_POWER, FORWARD_POWER, MOTOR_UNCHANGED);
    }
  }
  else
  {
    // if obstacle left:
    LCD_DRAW_TEXT(9, 8, "doDodgeRight   ");
    LCD_DRAW_TEXT(1, 10, "goRightSide       ");
    while (getSensorObstacle() == OBSTACLE && NOTEXITBUTTON)
    {
      doControlMotor(-20, -100, MOTOR_UNCHANGED);
    }
    doControlMotor(-20, -80, MOTOR_UNCHANGED);
    SLEEP(1);

    if (scnd == false)
    {
      while ((getSensorRight() == PATH || getSensorLeft() == PATH) && NOTEXITBUTTON)
      {
        doControlMotor(100, 100, MOTOR_UNCHANGED);
      }
      doControlMotor(100, 100, MOTOR_UNCHANGED);
      SLEEP(400);
    }
    if (scnd == true)
    {
      doControlMotor(20, 40, MOTOR_UNCHANGED);
      SLEEP(1);
      while (!(getSensorRight() == PATH && getSensorLeft() == PATH) && NOTEXITBUTTON)
      {
        doFollowPath();
      }
      doControlMotor(30, 30, MOTOR_UNCHANGED);
      SLEEP(1);
    } // return to path
    while (getSensorRight() != PATH && NOTEXITBUTTON)
    {
      doControlMotor(35, 100, MOTOR_UNCHANGED);
      if (getSensorObstacle() == OBSTACLE || getSensorObstacle() == OBSTACLE2)
      {
        doDodge(true, true);
        break;
      }
    }
    while (getSensorLeft() == PATH && getSensorRight() == PATH && NOTEXITBUTTON)
    {
      doControlMotor(FORWARD_POWER, -BACKWARD_POWER, MOTOR_UNCHANGED);
    }
  }
  LCD_DRAW_TEXT(9, 8, "doDodgeEnd  ");
}

// -----------------------------------------------------------------------------
// Behavior: close claw
// -----------------------------------------------------------------------------
void doGrab()
{
  LCD_DRAW_TEXT(9, 8, "doGrab     ");
  // turn left
  doControlMotor(-50, 50, MOTOR_UNCHANGED);
  SLEEP(1);
  doControlMotor(0, 70, MOTOR_UNCHANGED);
  SLEEP(1);
  // open claw
  doOpenClaw();
  // follow path
  while (getSensorGrab() != CONTACT && NOTEXITBUTTON)
  {
    doFollowGrabPath();
  }
  // stop and close claw
  doControlMotor(0, 0, 0);
  doCloseClaw();
  doControlMotor(-10, -10, MOTOR_UNCHANGED);
  SLEEP(1);
  while (getSensorRight() != GRAB_PATH && NOTEXITBUTTON)
  {
    doControlMotor(-10, -50, 0);
  }
  while (getSensorLeft() != GRAB_PATH && NOTEXITBUTTON)
  {
    doControlMotor(55, -30, MOTOR_UNCHANGED);
  }
  SLEEP(2);
  while (getSensorLeft() != GRAB_PATH && NOTEXITBUTTON)
  {
    doControlMotor(50, -20, 0);
  }

  while ((getSensorLeft() != PATH || getSensorRight() != PATH) && NOTEXITBUTTON)
  {
    doFollowGrabPath();
  }
  for (int i = 0; i < 300; i++)
  {
    doControlMotor(-100, 100, MOTOR_UNCHANGED);
  }
  grabbed = true;
  LCD_DRAW_TEXT(10, 10, "grabbed=true");
  LCD_DRAW_TEXT(9, 8, "doGrabEnd  ");
}

int main()
{
  // setup.configure motors
  SET_OUT(OUT_A, OUT_MOTOR); // motorLeft
  SET_OUT(OUT_B, OUT_MOTOR); // motorRight
  SET_OUT(OUT_C, OUT_MOTOR); // motorGrab

  // setup.configure sensors
  SET_IN(IN_1, IN_EV3_COLOR); // sensorLeft
  SET_IN(IN_2, IN_EV3_COLOR); // sensorRight
  SET_IN(IN_3, IN_EV3_SONAR); // sensorObstacle
  SET_IN(IN_4, IN_EV3_TOUCH); // sensorGrab

  // setup.initialise
  EVC_INIT();

  // setup.display
  int Wert1, Wert2, Wert3, Wert4;
  LCD_DRAW_TEXT(1, 1, "Color-L:");
  LCD_DRAW_TEXT(1, 2, "Color-R:");
  LCD_DRAW_TEXT(1, 3, "Sonar:  ");
  LCD_DRAW_TEXT(1, 4, "Touch:  ");

  LCD_DRAW_TEXT(1, 5, "Motor-A:");
  LCD_DRAW_TEXT(1, 6, "Motor-B:");
  LCD_DRAW_TEXT(1, 7, "Motor-C:");

  LCD_DRAW_TEXT(1, 8, "State:  ");

  // program ##########################################################################

  while (NOTEXITBUTTON)
  {
    // loop.display
    Wert1 = READ_IN(IN_1);
    switch (Wert1)
    {
    case 0:
      LCD_DRAW_TEXT(9, 1, "transparent");
      break;
    case 1:
      LCD_DRAW_TEXT(9, 1, "schwarz    ");
      break;
    case 2:
      LCD_DRAW_TEXT(9, 1, "blau       ");
      break;
    case 3:
      LCD_DRAW_TEXT(9, 1, "gruen      ");
      break;
    case 4:
      LCD_DRAW_TEXT(9, 1, "gelb       ");
      break;
    case 5:
      LCD_DRAW_TEXT(9, 1, "rot        ");
      break;
    case 6:
      LCD_DRAW_TEXT(9, 1, "weiss      ");
      break;
    case 7:
      LCD_DRAW_TEXT(9, 1, "braun      ");
      break;
    }

    Wert2 = READ_IN(IN_2);
    switch (Wert2)
    {
    case 0:
      LCD_DRAW_TEXT(9, 2, "transparent");
      break;
    case 1:
      LCD_DRAW_TEXT(9, 2, "schwarz    ");
      break;
    case 2:
      LCD_DRAW_TEXT(9, 2, "blau       ");
      break;
    case 3:
      LCD_DRAW_TEXT(9, 2, "gruen      ");
      break;
    case 4:
      LCD_DRAW_TEXT(9, 2, "gelb       ");
      break;
    case 5:
      LCD_DRAW_TEXT(9, 2, "rot        ");
      break;
    case 6:
      LCD_DRAW_TEXT(9, 2, "weiss      ");
      break;
    case 7:
      LCD_DRAW_TEXT(9, 2, "braun      ");
      break;
    }
    Wert3 = getSensorObstacle() == OBSTACLE;
    LCD_DRAW_INT(9, 3, Wert3);
    Wert4 = READ_IN(IN_4);
    LCD_DRAW_INT(9, 4, Wert4);

    // loop.button ------------------------------------------------------------------------------
    if (READ_BUTTON(BUTTON_LEFT) == 1)
    {
      doQuaterRight(true);
    }
    if (READ_BUTTON(BUTTON_RIGHT) == 1)
    {
      doQuaterRight(false);
    }
    if (READ_BUTTON(BUTTON_UP) == 1)
    {
      // bool i = isObstacleRight();
    }
    if (READ_BUTTON(BUTTON_DOWN) == 1)
    {
      // doDodge();
    }
    if (READ_BUTTON(BUTTON_CENTER) == 1)
    {
    }
    // loop.behavior -------------------------------------------------------------------------------------
    if (getSensorLeft() == GRAB_PATH)
    {
      doGrab();
    }
    if (getSensorObstacle() == OBSTACLE)
    {
      doDodge();
    }
    // default
    doFollowPath();
  }
  // end
  EVC_CLOSE();
  return 0;
}
