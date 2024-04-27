#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define PADDLE_WIDTH 30
#define PADDLE_HEIGHT 4
#define PADDLE_Y SCREEN_HEIGHT - 10
#define PADDLE_SPEED 3

#define BALL_SIZE 3
#define BALL_SPEED_X 1
#define BALL_SPEED_Y 1

#define OBSTACLE_WIDTH 20
#define OBSTACLE_HEIGHT 4
#define OBSTACLE_GAP 2 // Aralık boyutu
#define OBSTACLE_ROWS 3 // Tugla satır sayısı
#define OBSTACLE_COLS 6 // Tugla sütun sayısı

#define MAX_OBSTACLES (OBSTACLE_ROWS * OBSTACLE_COLS) // Maksimum engel sayısı
#define MAX_LIVES 3 // Maksimum can sayısı

#define OLED_RESET -1

#define LED_PIN_1 12
#define LED_PIN_2 11
#define LED_PIN_3 10

#define LATCH 6 //STCP
#define DATA 5 //DS
#define CLK 7 //SHCP

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int paddleX = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
int ballX = SCREEN_WIDTH / 2;
int ballY = SCREEN_HEIGHT / 2;
int ballSpeedX = BALL_SPEED_X;
int ballSpeedY = BALL_SPEED_Y;
int score = 0;
int lives = MAX_LIVES;
bool obstacleActive[MAX_OBSTACLES] = {false};
int obstacleX[MAX_OBSTACLES];
int obstacleY[MAX_OBSTACLES];

int potPin = A0;
int downButtonPin = 2;
int upButtonPin = 3;
int selectButtonPin = 4;

enum GameState {
  START_SCREEN,
  PLAYING,
  GAME_OVER
};

GameState gameState = START_SCREEN;

void setup() {
  Serial.begin(9600);
  
  pinMode(downButtonPin, INPUT_PULLUP);
  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(selectButtonPin, INPUT_PULLUP);
  
  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(LED_PIN_3, OUTPUT);


  pinMode(LATCH, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DATA, OUTPUT);  
  
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.display();
  drawStartScreen();
}

void loop() {
  switch (gameState) {
    case START_SCREEN:
      checkStartScreenInput();
      break;
    case PLAYING:
      movePaddle();
      moveBall();
      checkCollision();
      display.clearDisplay();
      drawPaddle();
      drawBall();
      for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacleActive[i])
          drawObstacle(i);
      }
      displayScore();
      displayLives();
      display.display();
      break;
    case GAME_OVER:
      // Game over state
      break;
  }
}

void movePaddle() {
  int potValue = analogRead(potPin);
  paddleX = map(potValue, 0, 1023, 0, SCREEN_WIDTH - PADDLE_WIDTH);
}

void moveBall() {
  ballX += ballSpeedX;
  ballY += ballSpeedY;
}

void drawPaddle() {
  display.fillRect(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);
}

void drawBall() {
  display.fillCircle(ballX, ballY, BALL_SIZE, SSD1306_WHITE);
}

void drawObstacle(int index) {
  display.fillRect(obstacleX[index], obstacleY[index], OBSTACLE_WIDTH, OBSTACLE_HEIGHT, SSD1306_WHITE);
}

void checkCollision() {
  if (ballX <= 0 || ballX >= SCREEN_WIDTH - BALL_SIZE) {
    ballSpeedX = -ballSpeedX;
  }
  if (ballY <= 0) {
    ballSpeedY = -ballSpeedY;
  }
  if (ballY >= PADDLE_Y - BALL_SIZE && ballX >= paddleX && ballX <= paddleX + PADDLE_WIDTH) {
    // Check if the ball hits the top surface of the paddle
    if (ballY + BALL_SIZE < PADDLE_Y) {
      ballSpeedY = -ballSpeedY;
      score++; // Only increment score if the ball hits above the paddle
    } else {
      ballSpeedY = -ballSpeedY;
    }
  }
  if (ballY > SCREEN_HEIGHT) {
    ballX = SCREEN_WIDTH / 2;
    ballY = SCREEN_HEIGHT / 2;
    ballSpeedX = BALL_SPEED_X;
    ballSpeedY = BALL_SPEED_Y;
    lives--;
    if (lives <= 0) {
      gameState = GAME_OVER;
      drawGameOverScreen();
    }
  }
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacleActive[i] && ballY <= obstacleY[i] + OBSTACLE_HEIGHT && ballY >= obstacleY[i] && ballX >= obstacleX[i] && ballX <= obstacleX[i] + OBSTACLE_WIDTH) {
      obstacleActive[i] = false;
      score++;
      if (ballY + BALL_SIZE >= obstacleY[i] && ballY <= obstacleY[i] + OBSTACLE_HEIGHT)
        ballSpeedY = -ballSpeedY;
      if (random(0, 10) == 0 && lives < MAX_LIVES) // %10 olasılıkla can düşsün
        lives++;
    }
  }
  
  // Tüm engeller yok olduysa yeni engelleri oluştur ve topun hızını artır
  bool allObstaclesCleared = true;
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacleActive[i]) {
      allObstaclesCleared = false;
      break;
    }
  }
  if (allObstaclesCleared) {
    initObstacles();
    increaseBallSpeed();
  }
}

void displayScore() {
  // display.setTextSize(1);
  // display.setTextColor(SSD1306_WHITE);
  // display.setCursor(0, 0);
  // display.print("Score: ");
  // display.print(score);

  // Score'u yedi segment ekrana yazdır
  displayScoreOnSevenSegment(score);
}

void displayLives() {
  // display.setTextSize(1);
  // display.setTextColor(SSD1306_WHITE);
  // display.setCursor(0, 10);
  // display.print("Lives: ");
  // display.print(lives);

  // Can sayısını LED'lerle göster
  updateLivesIndicator();
}

void drawStartScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 10);
  display.println("YUSUF SAS");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
 
  display.setCursor(10, 40);
  display.println("Start");
  display.setCursor(10, 50);
  display.println("Exit");
  display.display();
}

void checkStartScreenInput() {
  if (digitalRead(upButtonPin) == LOW) {
    gameState = PLAYING;
    initObstacles(); // Oyun başladığında engelleri yerleştir
  }
  if (digitalRead(downButtonPin) == LOW) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("Game Over!");
    display.display();
    delay(2000);
    display.clearDisplay();
    display.display();
    while (true);
  }
}

void drawGameOverScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 10);
  display.println("Game Over!");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 40);
  display.println("Score: ");
  display.print(score);
  display.display();
}

void initObstacles() {
  int x = (SCREEN_WIDTH - OBSTACLE_COLS * (OBSTACLE_WIDTH + OBSTACLE_GAP)) / 2; // Engellerin sol üst köşesinin X koordinatı
  int y = 10; // Engellerin sol üst köşesinin Y koordinatı
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacleActive[i] = true;
    obstacleX[i] = x;
    obstacleY[i] = y;
    x += OBSTACLE_WIDTH + OBSTACLE_GAP; // Bir sonraki engelin başlangıç konumunu ayarla
    if ((i + 1) % OBSTACLE_COLS == 0) { // Satır sonuna gelindiğinde
      x = (SCREEN_WIDTH - OBSTACLE_COLS * (OBSTACLE_WIDTH + OBSTACLE_GAP)) / 2; // X koordinatını başa döndür
      y += OBSTACLE_HEIGHT + OBSTACLE_GAP; // Bir sonraki satıra geç
    }
  }
}

void increaseBallSpeed() {
  ballSpeedX += BALL_SPEED_X * 0.2; // Topun X eksenindeki hızını %20 artır
  ballSpeedY += BALL_SPEED_Y * 0.2; // Topun Y eksenindeki hızını %20 artır
}

void updateLivesIndicator() {
  digitalWrite(LED_PIN_1, LOW);
  digitalWrite(LED_PIN_2, LOW);
  digitalWrite(LED_PIN_3, LOW);
  
  switch (lives) {
    case 3:
      digitalWrite(LED_PIN_1, HIGH);
    case 2:
      digitalWrite(LED_PIN_2, HIGH);
    case 1:
      digitalWrite(LED_PIN_3, HIGH);
    default:
      break;
  }
}

void displayScoreOnSevenSegment(int score) {
  // Her basamağı temsil eden rakamların dizisi
  byte cc_rakamlar[10]={
    63,6,91,79,102,109,125,7,127,111
  };
  
  int hundreds = score / 100; // Yüzler basamağı
  int tens = (score / 10) % 10; // Onlar basamağı
  int ones = score % 10; // Birler basamağı
  
  digitalWrite(LATCH, LOW);

  // Yüzler basamağını yazdır
  shiftOut(DATA, CLK, MSBFIRST, cc_rakamlar[hundreds]);
  // Onlar basamağını yazdır
  shiftOut(DATA, CLK, MSBFIRST, cc_rakamlar[tens]);
  // Birler basamağını yazdır
  shiftOut(DATA, CLK, MSBFIRST, cc_rakamlar[ones]);

  digitalWrite(LATCH, HIGH);
}

