#include <QueueList.h>

// Syntactic sugar
#define ON 1
#define OFF 0

#define SIDES 4
#define SIZE 5

#define REFRESH_DELAY 1000
#define MOVE_DELAY 300UL

#define UP 0
#define RIGHT 1
#define LEFT 2
#define DOWN 3

typedef struct {
    int side;
    int row;
    int col;
} pos;

// FIFO stack for snake blocks
QueueList<pos> snake;

// Timer for non-blocking refresh
unsigned long timer;

// ON/OFF status for every LED
int leds[SIDES][SIZE][SIZE];

// Linking indexes with physical pins
int row_pins[][SIZE] = { 
  {2, 3, 4, 5, 6},
  {7, 8, 9, 10, 11},
  {12, 13, 14, 15, 16},
  {17, 18, 19, 20, 21} 
};
int col_pins[][SIZE] = { 
  {28, 29, 30, 31, 32},
  {33, 34, 35, 36, 37},
  {38, 39, 40, 41, 42},
  {43, 44, 45, 46, 47}
};
int btn_pins[] = { A0, A1, A2, A3 };

// Keypad button states
int states[] = { 0, 0, 0, 0 };
int prev_states[] = { 0, 0, 0, 0 };

pos curr;
pos apple;
int dir = RIGHT;

void loop() {
  refresh_leds();
  read_keypad();
  update_snake();
}

void setup() {
  // seed PRG
  randomSeed(analogRead(A5));
  
  // prepare led matrix
  for (int side = 0; side < SIDES; side++) {
    for (int row = row_pins[side][0]; row <= row_pins[side][SIZE-1]; row++) {
      pinMode(row, OUTPUT);
      digitalWrite(row, LOW);
    }
    for (int col = col_pins[side][0]; col <= col_pins[side][SIZE-1]; col++) {
      pinMode(col, OUTPUT);
      pinMode(col, HIGH);
    }
  }

  // prepare joystick
  for(int btn = 0; btn < 4; btn++) {
    pinMode(btn_pins[btn], INPUT_PULLUP);
  }

  clear();
}

// Refresh the LED matrix with the latest values of the status array
void refresh_leds() {
  for (int side = 0; side < SIDES; side++) {
    int prev_row = SIZE - 1;
    
    for (int row = 0; row < SIZE; prev_row = row++) {
      digitalWrite(row_pins[side][row], HIGH);
      digitalWrite(row_pins[side][prev_row], LOW);
  
      for (int col = SIZE - 1; col >= 0; col--) {
        digitalWrite(col_pins[side][col], leds[side][row][col] == 1 ? LOW : HIGH);
      }
    
      delayMicroseconds(REFRESH_DELAY);
    }
  }
}

// Save most recent pressed direction from keypad
void read_keypad() {
  for (int btn = 0; btn < 4; btn++) {
    states[btn] = digitalRead(btn_pins[btn]);
    
    if (states[btn] != prev_states[btn] && states[btn] == LOW) {
      bool valid = 1;

      if ((btn == UP && dir == DOWN) 
       || (btn == DOWN && dir == UP) 
       || (btn == LEFT && dir == RIGHT) 
       || (btn == RIGHT && dir == LEFT)) {
        valid = 0;
      }
      
      if (valid) {
        dir = btn;
      }
    }

    prev_states[btn] = states[btn];
  }
}

// Snake movement logic
void update_snake() {
  if (millis() - timer >= MOVE_DELAY) {
    timer = millis();
    pos next;

    next.side = curr.side;
    next.row = curr.row;
    next.col = curr.col;
        
    switch (dir) {
      case UP:
        next.row = curr.row == 0 ? SIZE - 1 : curr.row - 1;
        break;
        
      case RIGHT:      
        if (curr.col + 1 == SIZE) {
          next.col = 0;
          next.side = (curr.side + 1) % SIDES;
        } else {
          next.col++;
        }
        break;

      case DOWN:
        next.row = curr.row + 1 == SIZE ? 0 : curr.row + 1;
        break;

      case LEFT:
        if (curr.col == 0) {
          next.col = SIZE - 1;
          next.side = next.side == 0 ? SIDES - 1 : next.side - 1;
        } else {
          next.col = curr.col - 1;
        }
        next.col = curr.col == 0 ? SIZE - 1 : curr.col - 1;
        break;
    }

    if (next.side == apple.side && next.row == apple.row && next.col == apple.col) {
      place_apple();
    } else if (leds[next.side][next.row][next.col] == ON) {
      game_over();
      return;
    } else {
      pos last = snake.pop();
      leds[last.side][last.row][last.col] = OFF;
    }

    snake.push(next);
    leds[next.side][next.row][next.col] = ON;
    curr = next;
  }
}

// Put snake item at a random place
void place_apple() {
  do {
    apple.side = random(0, SIDES - 1);
    apple.row = random(0, SIZE - 1);
    apple.col = random(0, SIZE - 1);
  } while (apple.side == curr.side
        && apple.row == curr.row
        && apple.col == curr.col);
  leds[apple.side][apple.row][apple.col] = ON;
}

void non_blocking_refresh(unsigned long ms) {
  while (millis() - timer < ms) {
    refresh_leds();
  }
}

void game_over_animation() {
  for (int side = 0; side < SIDES; side++) {
    leds[side][SIZE / 2][SIZE / 2] = ON;
  }

  non_blocking_refresh(1000UL);
  timer = millis();

  for (int side = 0; side < SIDES; side++) {
    leds[side][1][1] = ON;
    leds[side][1][3] = ON;
    leds[side][3][1] = ON;
    leds[side][3][3] = ON;
  }

  non_blocking_refresh(1000UL);
  timer = millis();

  for (int side = 0; side < SIDES; side++) {
    leds[side][0][0] = ON;
    leds[side][0][4] = ON;
    leds[side][4][0] = ON;
    leds[side][4][4] = ON;
  }

  non_blocking_refresh(1000UL);
}

// Reset and animation
void game_over() {
  while (!snake.isEmpty()) {
    pos last = snake.pop();
    leds[last.side][last.row][last.col] = OFF;
  }
  leds[apple.side][apple.row][apple.col] = OFF;
  refresh_leds();

  game_over_animation();

  clear();
}

void clear() {
  for (int side = 0; side < SIDES; side++) {
    for (int row = 0; row < SIZE; row++) {
      for (int col = 0; col < SIZE; col++) {
        leds[side][row][col] = OFF;
      }
    }
  }

  curr.side = 0;
  curr.row = 0;
  curr.col = 0;
  snake.push(curr);

  leds[0][0][0] = ON;

  place_apple();

  timer = millis();  
}
