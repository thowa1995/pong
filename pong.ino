//    Copyright (C) 2018  Tom Howard
//    
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"'
#include <math.h>
// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 5, 4);

#define TICK_RATE     15
#define BAT_WIDTH     2
#define BAT_HEIGHT    12
#define BAT_Y_POS     26
#define BAT_DIST      4
#define PLAYER_SPEED  0.8
#define AI_SPEED      0.65
#define MAX_ANGLE     1.3 /* ~75 degrees */
#define BALL_RADIUS   1
#define BALL_X_POS    64
#define BALL_Y_POS    32
#define BALL_VEL      1
#define SCREEN_LEFT   0
#define SCREEN_RIGHT  127
#define SCREEN_TOP    0
#define SCREEN_BOTTOM 63
#define BUTTON_DELAY  50
long button_time;
double angle;
int left_score, right_score;
bool gooing_down;

typedef struct {
  int width;
  int height;
  float x_pos;
  float y_pos;
} Bat;

typedef struct {
  int radius;
  float x_pos;
  float y_pos;
  float x_vel;
  float y_vel;
} Ball;

Bat new_bat(Bat* bat, int x) {
  bat->width = BAT_WIDTH;
  bat->height = BAT_HEIGHT;
  bat->x_pos = x;
  bat->y_pos = BAT_Y_POS;
}

Ball new_ball(Ball* ball) {
  ball->radius = BALL_RADIUS;
  ball->x_pos = BALL_X_POS;
  ball->y_pos = BALL_Y_POS;
  ball->x_vel = -BALL_VEL;
  ball->y_vel = 0;
}

void draw_border() {
  display.drawRect(SCREEN_LEFT, SCREEN_TOP, SCREEN_RIGHT - SCREEN_LEFT, SCREEN_BOTTOM - SCREEN_TOP);
}

void draw_bat(Bat* bat) {
  display.drawRect((int)bat->x_pos, (int)bat->y_pos, bat->width, bat->height);
}

void draw_ball(Ball* ball) {
  display.drawCircle((int)ball->x_pos, (int)ball->y_pos, ball->radius);
}

bool end_point(Ball* ball) {
  if (ball->x_pos <= SCREEN_LEFT) {
    /* right wins */
    return true;
  }
  if (ball->x_pos >= SCREEN_RIGHT) {
    /* left wins */
    return true;
  }
  return false;    
}

bool contact_bat(Ball* ball, Bat* bat) {
  if (ball->y_pos >= bat->y_pos && ball->y_pos <= bat->y_pos + BAT_HEIGHT) {
      if(((ball->x_pos <= bat->x_pos) && (ball->x_pos + ball->x_vel > bat->x_pos))
         || 
         ((ball->x_pos >= bat->x_pos) && (ball->x_pos + ball->x_vel < bat->x_pos))) {
        return true;
      }
  }
  return false;
}

bool touch_edge(Ball* ball) {
  return (ball->y_pos <= SCREEN_TOP || ball->y_pos >=  SCREEN_BOTTOM);
}

void bounce(Ball* ball) {
  ball->y_vel = -ball->y_vel;
}
void hit(Ball* ball, Bat* bat) {
  beep(0.1,400);
  float relative = (bat->y_pos + (BAT_HEIGHT/2)) - ball->y_pos;
  float normalised = relative/(BAT_HEIGHT/2);
  angle = normalised * MAX_ANGLE;
  if (ball->x_vel <= 0) {
    ball->x_vel = BALL_VEL * cos(angle);
  } else {
    ball->x_vel = BALL_VEL * -cos(angle);
  }
  ball->y_vel = BALL_VEL * -sin(angle);
  return;
}

void ai_bat(Bat* bat, Ball* ball) {
  if (ball->y_pos > bat->y_pos + BAT_HEIGHT/2) {
    bat->y_pos += AI_SPEED;
  }
  if (ball->y_pos < bat->y_pos + BAT_HEIGHT/2) {
    bat->y_pos -= AI_SPEED;
  }
  return;
}



void beep(float delay_s, int freq){
  for (long i = 0; i < 2048 * delay_s; i++ ) {
  
    // 1 / 2048Hz = 488uS, or 244uS high and 244uS low to create 50% duty cycle
    digitalWrite(15, HIGH);
    delayMicroseconds(freq);
    digitalWrite(15, LOW);
    delayMicroseconds(freq);
  }
}
void setup() {
  pinMode(14, INPUT_PULLUP);
  pinMode(25, INPUT_PULLUP);
  pinMode(15,OUTPUT);
  display.init();
  display.flipScreenVertically();

  

}

void loop(){

  int bv4, bv14;
  long last_tick = millis();
  long last_press = millis();
  Bat left_bat, right_bat;
  Ball ball;
  
  new_bat(&left_bat, SCREEN_LEFT + BAT_DIST);
  new_bat(&right_bat, SCREEN_RIGHT - (BAT_DIST+2)); /* taking into account line width */
  new_ball(&ball);
  
  while(!end_point(&ball)) 
  {
    /* deal with input */
    bv14 = digitalRead(14);
    bv4 = digitalRead(25);
    if (bv14 == LOW){
      if (millis() - last_press > BUTTON_DELAY) {
        left_bat.y_pos -= PLAYER_SPEED;
      }
    } 
  
    else if (bv4 == LOW){
        
      if (millis() - last_press > BUTTON_DELAY) {
        //beep(0.1,400);
        left_bat.y_pos += PLAYER_SPEED;
      }
    }
    ai_bat(&right_bat, &ball);
    /* update the game state */
    ball.x_pos += ball.x_vel;
    ball.y_pos += ball.y_vel;
    if (contact_bat(&ball, &left_bat)) {
      hit(&ball, &left_bat);
    }
    if (contact_bat(&ball, &right_bat)) {
      hit(&ball, &right_bat);
    }
    if (touch_edge(&ball)) {
      bounce(&ball);
    }
    /* draw stuff to the screen */
    draw_bat(&left_bat);
    draw_bat(&right_bat);
    draw_ball(&ball);
    draw_border();

    display.display();
    while(millis() - last_tick < TICK_RATE) {} /* just hang for a while */
    last_tick = millis();
    display.clear();
    }
}
