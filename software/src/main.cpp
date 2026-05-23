#include <FastLED.h>

#define LED_PIN     2          
#define NUM_LEDS    64         
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

float currentBrightness = 5.0;
int targetBrightness = 5;

#define BTN_DOWN_PIN       3  
#define BTN_CONTROL_PIN    4  
#define BTN_RIGHT_PIN      5  
#define BTN_UP_PIN         6  
#define BTN_LEFT_PIN       7  

CRGB leds[NUM_LEDS];

volatile bool btnDown_ISR = false;
volatile bool btnControl_ISR = false; 
volatile bool btnRight_ISR = false;
volatile bool btnUp_ISR = false;
volatile bool btnLeft_ISR = false;

volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 150; 

unsigned long lastFrameTime = 0;
const unsigned long FRAME_DELAY = 20; 

volatile unsigned long timer2_millis = 0;

void setupCustomMillis() {
    TCCR2A = 0; 
    TCCR2B = 0;
    TCNT2  = 0; 

    OCR2A = 249;
    
    TCCR2A |= (1 << WGM21);
    
    TCCR2B |= (1 << CS22);   
    
    TIMSK2 |= (1 << OCIE2A);
}

ISR(TIMER2_COMPA_vect) {
    timer2_millis++;
}

unsigned long custom_millis() {
    unsigned long m;
    uint8_t oldSREG = SREG; 
    cli(); 
    m = timer2_millis;
    SREG = oldSREG; 
    return m;
}

const uint8_t symbols[8][8] = {
  { 0, 0, 0, 0, 0, 0, 0, 0 }, 
  { 0b00000000, 0b01111100, 0b01000010, 0b01111100, 0b01111100, 0b01000010, 0b01111100, 0b00000000 }, // B
  { 0b00000000, 0b01111110, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b01111110, 0b00000000 }, // I
  { 0b00000000, 0b00011000, 0b00100100, 0b00100100, 0b00111100, 0b00100100, 0b00100100, 0b00000000 }, // A
  { 0b00000000, 0b01000010, 0b01100010, 0b01010010, 0b01001010, 0b01000110, 0b01000010, 0b00000000 }, // N
  { 0b00000000, 0b00111110, 0b01110000, 0b01100000, 0b01100000, 0b01110000, 0b00111110, 0b00000000 }, // C
  { 0b00000000, 0b00011000, 0b00100100, 0b00100100, 0b00111100, 0b00100100, 0b00100100, 0b00000000 }, // A
  { 0b00000000, 0b01100110, 0b11111111, 0b11111111, 0b01111110, 0b00111100, 0b00011000, 0b00000000 }, // ❤️
};

int XY(int x, int y) {
    if (x < 0 || x > 7 || y < 0 || y > 7) return -1;
    return (7 - y) * 8 + (7 - x);
}

void setMatrixLed(int x, int y, CRGB color) {
    int index = XY(x, y);
    if (index != -1) {
        leds[index] = color;
    }
}

void drawSymbol(int index, CRGB color) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (symbols[index][row] & (0x80 >> col)) setMatrixLed(col, row, color);
        }
    }
}

enum AppState { MENU, APP_SNAKE, APP_CONNECT4, APP_CHECKERS, APP_BIANCA, APP_TEST };
void changeState(AppState newState); 

struct ButtonState {
    bool up;
    bool down;
    bool left;
    bool right;
    bool control;
};

class App {
public:
    virtual ~App() = default;
    virtual void init() {} 
    virtual void handleButtons(ButtonState btns) = 0; 
    virtual void run() = 0; 
};

class MenuApp : public App {
private:
    int menuSelection = 0;

    void drawMenuSnake() {
        const int pathX[20] = {1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 1, 1, 1, 1};
        const int pathY[20] = {1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 6, 5, 4, 3, 2};
        
        int step = (custom_millis() / 300) % 20;
        int snakeLen = 4;
        
        int appleIndex = (step + 2) % 20;
        setMatrixLed(pathX[appleIndex], pathY[appleIndex], CRGB::Red);
        
        for (int i = 0; i < snakeLen; i++) {
            int index = (step - i + 20) % 20; 
            CRGB color = (i == 0) ? CRGB::Yellow : CRGB::Green; 
            setMatrixLed(pathX[index], pathY[index], color);
        }
    }

    void drawMenuConnect4() {
        for (int x = 0; x < 8; x++) setMatrixLed(x, 1, CRGB::White);

        setMatrixLed(2, 7, CRGB::Red); setMatrixLed(3, 7, CRGB::Yellow); setMatrixLed(4, 7, CRGB::Red); setMatrixLed(5, 7, CRGB::Yellow);
        setMatrixLed(2, 6, CRGB::Yellow); setMatrixLed(3, 6, CRGB::Red); setMatrixLed(5, 6, CRGB::Yellow);
        setMatrixLed(4, 6, CRGB::Red); setMatrixLed(4, 5, CRGB::Yellow);
        
        int step = (custom_millis() / 250) % 14;
        int xPos = (step < 8) ? step : 14 - step;     
        setMatrixLed(xPos, 0, CRGB::Yellow);
    }

    void drawMenuCheckers() {
        for(int y=0; y<8; y++) {
            for(int x=0; x<8; x++) {
                if ((x+y)%2 != 0) {
                    if (y<3) setMatrixLed(x, y, CRGB::Blue);
                    else if (y>4) setMatrixLed(x, y, CRGB::Red);
                    else setMatrixLed(x, y, CRGB(5,5,5)); 
                }
            }
        }
        int step = (custom_millis() / 700) % 2;
        if (step == 1) {
            setMatrixLed(2, 5, CRGB(5,5,5)); 
            setMatrixLed(3, 4, CRGB::Red);   
        }
    }

public:
    void init() override {
        menuSelection = 0; 
    }

    void handleButtons(ButtonState btns) override {
        if (btns.left)  { menuSelection = (menuSelection - 1 + 5) % 5; }
        if (btns.right) { menuSelection = (menuSelection + 1) % 5; }
        if (btns.control) {
            if (menuSelection == 0) changeState(APP_SNAKE);
            else if (menuSelection == 1) changeState(APP_CONNECT4);
            else if (menuSelection == 2) changeState(APP_CHECKERS);
            else if (menuSelection == 3) changeState(APP_BIANCA);
            else if (menuSelection == 4) changeState(APP_TEST);
        }
    }

    void run() override {
        FastLED.clear();
        if (menuSelection == 0) drawMenuSnake();
        else if (menuSelection == 1) drawMenuConnect4();
        else if (menuSelection == 2) drawMenuCheckers();
        else if (menuSelection == 3) drawSymbol(1, CRGB::Aqua); 
        else if (menuSelection == 4) {
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    setMatrixLed(x, y, CHSV(x * 32 + y * 4, 255, 255));
                }
            }
        }
    }
};

class SnakeApp : public App {
private:
    int snakeX[64], snakeY[64], snakeLen, snakeDirX, snakeDirY, foodX, foodY;
    unsigned long lastSnakeMove = 0;
    bool snakeGameOver = false;

    void spawnFood() {
        if (snakeLen >= 64) { foodX = -1; foodY = -1; return; }
        bool validSpot = false;
        while (!validSpot) {
            foodX = random(0, 8); foodY = random(0, 8);
            validSpot = true;
            for (int i = 0; i < snakeLen; i++) {
                if (foodX == snakeX[i] && foodY == snakeY[i]) {
                    validSpot = false; break;
                }
            }
        }
    }

public:
    void init() override {
        snakeLen = 3;
        snakeX[0] = 4; snakeY[0] = 4; snakeX[1] = 4; snakeY[1] = 5; snakeX[2] = 4; snakeY[2] = 6;
        snakeDirX = 0; snakeDirY = -1; 
        snakeGameOver = false;
        spawnFood(); 
    }

    void handleButtons(ButtonState btns) override {
        if (btns.control) { changeState(MENU); return; }
        if (!snakeGameOver) {
            if (btns.up)    { if (snakeDirY == 0) { snakeDirX = 0; snakeDirY = -1; } }
            if (btns.down)  { if (snakeDirY == 0) { snakeDirX = 0; snakeDirY = 1;  } }
            if (btns.left)  { if (snakeDirX == 0) { snakeDirX = -1; snakeDirY = 0; } }
            if (btns.right) { if (snakeDirX == 0) { snakeDirX = 1; snakeDirY = 0;  } }
        } else if (btns.up || btns.down || btns.left || btns.right) {
            init(); 
        }
    }

    void run() override {
        if (!snakeGameOver) {
            if (custom_millis() - lastSnakeMove > 1500) {
                for (int i = snakeLen - 1; i > 0; i--) {
                    snakeX[i] = snakeX[i - 1]; snakeY[i] = snakeY[i - 1];
                }
                snakeX[0] += snakeDirX; snakeY[0] += snakeDirY;

                if (snakeX[0] < 0) snakeX[0] = 7; else if (snakeX[0] > 7) snakeX[0] = 0;
                if (snakeY[0] < 0) snakeY[0] = 7; else if (snakeY[0] > 7) snakeY[0] = 0;

                for (int i = 1; i < snakeLen; i++) {
                    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) snakeGameOver = true;
                }

                if (snakeX[0] == foodX && snakeY[0] == foodY) {
                    if(snakeLen < 64) {
                        snakeX[snakeLen] = snakeX[snakeLen - 1]; 
                        snakeY[snakeLen] = snakeY[snakeLen - 1];
                        snakeLen++;
                    }
                    spawnFood(); 
                }
                lastSnakeMove = custom_millis();
            }
        }
        FastLED.clear();
        setMatrixLed(foodX, foodY, CRGB::Red); 
        for (int i = 0; i < snakeLen; i++) setMatrixLed(snakeX[i], snakeY[i], (i == 0) ? CRGB::Yellow : CRGB::Green);
        if (snakeGameOver && ((custom_millis() / 250) % 2 == 0)) FastLED.clear();
    }
};

class Connect4App : public App {
private:
    uint8_t c4Grid[8][8]; 
    bool c4WinMask[8][8]; 
    int c4CursorX, c4Player, c4Winner, c4AnimX, c4AnimY_current, c4AnimY_target, c4AnimPlayer;
    bool c4GameOver, c4Animating;
    unsigned long lastC4AnimTime, lastCursorMove; 

    bool checkConnect4Win(int p) {
        bool won = false;
        for (int y = 2; y < 8; y++) {
            for (int x = 0; x < 5; x++) {
                if (c4Grid[y][x] == p && c4Grid[y][x+1] == p && c4Grid[y][x+2] == p && c4Grid[y][x+3] == p) {
                    c4WinMask[y][x] = c4WinMask[y][x+1] = c4WinMask[y][x+2] = c4WinMask[y][x+3] = true; won = true;
                }
            }
        }
        for (int x = 0; x < 8; x++) {
            for (int y = 2; y < 5; y++) {
                if (c4Grid[y][x] == p && c4Grid[y+1][x] == p && c4Grid[y+2][x] == p && c4Grid[y+3][x] == p) {
                    c4WinMask[y][x] = c4WinMask[y+1][x] = c4WinMask[y+2][x] = c4WinMask[y+3][x] = true; won = true;
                }
            }
        }
        for (int y = 2; y < 5; y++) {
            for (int x = 0; x < 5; x++) {
                if (c4Grid[y][x] == p && c4Grid[y+1][x+1] == p && c4Grid[y+2][x+2] == p && c4Grid[y+3][x+3] == p) {
                    c4WinMask[y][x] = c4WinMask[y+1][x+1] = c4WinMask[y+2][x+2] = c4WinMask[y+3][x+3] = true; won = true;
                }
            }
        }
        for (int y = 5; y < 8; y++) {
            for (int x = 0; x < 5; x++) {
                if (c4Grid[y][x] == p && c4Grid[y-1][x+1] == p && c4Grid[y-2][x+2] == p && c4Grid[y-3][x+3] == p) {
                    c4WinMask[y][x] = c4WinMask[y-1][x+1] = c4WinMask[y-2][x+2] = c4WinMask[y-3][x+3] = true; won = true;
                }
            }
        }
        return won;
    }
    bool checkConnect4Tie() {
        for (int x = 0; x < 8; x++) if (c4Grid[2][x] == 0) return false;
        return true; 
    }

public:
    void init() override {
        for(int i=0; i<8; i++) for(int j=0; j<8; j++) { c4Grid[i][j] = 0; c4WinMask[i][j] = false; }
        c4CursorX = 3; c4Player = 2; c4GameOver = false; c4Winner = 0; c4Animating = false; 
        lastCursorMove = custom_millis();
    }

    void handleButtons(ButtonState btns) override {
        if (btns.control) { changeState(MENU); return; }
        if (!c4Animating && !c4GameOver) {
            if (btns.left)  { 
                int tempX = c4CursorX - 1; if (tempX < 0) tempX = 7; 
                int startX = tempX;
                while (c4Grid[2][tempX] != 0) { tempX--; if (tempX < 0) tempX = 7; if (tempX == startX) break; }
                c4CursorX = tempX; lastCursorMove = custom_millis(); 
            }
            if (btns.right) { 
                int tempX = c4CursorX + 1; if (tempX > 7) tempX = 0; 
                int startX = tempX;
                while (c4Grid[2][tempX] != 0) { tempX++; if (tempX > 7) tempX = 0; if (tempX == startX) break; }
                c4CursorX = tempX; lastCursorMove = custom_millis(); 
            }
            if (btns.down || btns.up) {
                if (c4Grid[2][c4CursorX] == 0) { 
                    for (int y = 7; y >= 2; y--) {
                        if (c4Grid[y][c4CursorX] == 0) {
                            c4AnimX = c4CursorX; c4AnimY_target = y; c4AnimY_current = 0; 
                            c4AnimPlayer = c4Player; c4Animating = true; lastC4AnimTime = custom_millis(); break;
                        }
                    }
                }
            }
        } else if (c4GameOver) {
            if (btns.up || btns.down || btns.left || btns.right) init(); 
        }
    }

    void run() override {
        if (c4Animating) {
            if (custom_millis() - lastC4AnimTime > 60) {
                lastC4AnimTime = custom_millis();
                if (c4AnimY_current < c4AnimY_target) c4AnimY_current++;
                else {
                    c4Grid[c4AnimY_target][c4AnimX] = c4AnimPlayer;
                    if (checkConnect4Win(c4AnimPlayer)) { c4GameOver = true; c4Winner = c4AnimPlayer; } 
                    else if (checkConnect4Tie()) { c4GameOver = true; c4Winner = 3; } 
                    else { 
                        c4Player = (c4Player == 1) ? 2 : 1; 
                        if (c4Grid[2][c4CursorX] != 0) {
                            for (int i = 1; i <= 8; i++) {
                                int checkX = (c4CursorX + i) % 8;
                                if (c4Grid[2][checkX] == 0) { c4CursorX = checkX; break; }
                            }
                        }
                        lastCursorMove = custom_millis(); 
                    }
                    c4Animating = false; 
                }
            }
        } 
        FastLED.clear();
        for (int x = 0; x < 8; x++) setMatrixLed(x, 1, CRGB::White);
        for (int y = 2; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (c4Grid[y][x] == 1) setMatrixLed(x, y, CRGB::Red);
                if (c4Grid[y][x] == 2) setMatrixLed(x, y, CRGB::Yellow);
            }
        }
        if (c4Animating) {
            setMatrixLed(c4AnimX, c4AnimY_current, (c4AnimPlayer == 1) ? CRGB::Red : CRGB::Yellow);
        } else if (!c4GameOver) {
            if (((custom_millis() - lastCursorMove) / 500) % 2 == 0 && c4Grid[2][c4CursorX] == 0) {
                setMatrixLed(c4CursorX, 0, (c4Player == 1) ? CRGB::Red : CRGB::Yellow);
            }
        } else {
            if (c4Winner == 3) { 
                if ((custom_millis() / 300) % 2 == 0) {
                    for (int y = 2; y < 8; y++) for (int x = 0; x < 8; x++) if (c4Grid[y][x] != 0) setMatrixLed(x, y, CRGB::Black);
                }
            } else { 
                if ((custom_millis() / 300) % 2 == 0) {
                    for (int y = 2; y < 8; y++) for (int x = 0; x < 8; x++) if (c4WinMask[y][x]) setMatrixLed(x, y, CRGB::Black); 
                }
            }
        }
    }
};

class CheckersApp : public App {
private:
    uint8_t board[8][8];
    int cursorX = 3, cursorY = 4;
    int selectedX = -1, selectedY = -1;
    int turn = 1; 
    int winner = 0;

    bool isEnemy(int p1, int p2) {
        if (p1 == 1 || p1 == 3) return (p2 == 2 || p2 == 4);
        if (p1 == 2 || p1 == 4) return (p2 == 1 || p2 == 3);
        return false;
    }

    void checkWin() {
        int rCount = 0, bCount = 0;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (board[y][x] == 1 || board[y][x] == 3) rCount++;
                if (board[y][x] == 2 || board[y][x] == 4) bCount++;
            }
        }
        if (rCount == 0) winner = 2;
        if (bCount == 0) winner = 1;
    }

public:
    void init() override {
        for(int y=0; y<8; y++) {
            for(int x=0; x<8; x++) {
                if ((x+y)%2 != 0) { 
                    if (y < 3) board[y][x] = 2; 
                    else if (y > 4) board[y][x] = 1; 
                    else board[y][x] = 0;
                } else {
                    board[y][x] = 0;
                }
            }
        }
        cursorX = 3; cursorY = 4;
        selectedX = -1; selectedY = -1;
        turn = 1; winner = 0;
    }

    void handleButtons(ButtonState btns) override {
        if (winner != 0) { if (btns.control) changeState(MENU); return; }

        if (btns.up)    { if (cursorY > 0) cursorY--; }
        if (btns.down)  { if (cursorY < 7) cursorY++; }
        if (btns.left)  { if (cursorX > 0) cursorX--; }
        if (btns.right) { if (cursorX < 7) cursorX++; }

        if (btns.control) {
            if (selectedX == -1) {
                if ((cursorX + cursorY) % 2 == 0) {
                    changeState(MENU);
                    return;
                }
                int p = board[cursorY][cursorX];
                if ((turn == 1 && (p == 1 || p == 3)) || (turn == 2 && (p == 2 || p == 4))) {
                    selectedX = cursorX;
                    selectedY = cursorY;
                }
            } else {
                if (cursorX == selectedX && cursorY == selectedY) {
                    selectedX = -1; selectedY = -1; 
                } else if (board[cursorY][cursorX] == 0 && ((cursorX + cursorY) % 2 != 0)) {
                    int p = board[selectedY][selectedX];
                    int dx = cursorX - selectedX;
                    int dy = cursorY - selectedY;
                    bool isKing = (p == 3 || p == 4);
                    int forwardDir = (p == 1 || p == 3) ? -1 : 1; 
                    
                    bool validMove = false;
                    bool isJump = false;
                    int midX = 0, midY = 0;

                    if (abs(dx) == 1) {
                        if (dy == forwardDir || (isKing && abs(dy) == 1)) validMove = true;
                    } 
                    else if (abs(dx) == 2) {
                        if (dy == 2 * forwardDir || (isKing && abs(dy) == 2)) {
                            midX = selectedX + dx / 2;
                            midY = selectedY + dy / 2;
                            int midPiece = board[midY][midX];
                            if (midPiece != 0 && isEnemy(p, midPiece)) {
                                validMove = true;
                                isJump = true;
                            }
                        }
                    }

                    if (validMove) {
                        board[cursorY][cursorX] = p;
                        board[selectedY][selectedX] = 0;
                        if (isJump) board[midY][midX] = 0;

                        if (p == 1 && cursorY == 0) board[cursorY][cursorX] = 3;
                        if (p == 2 && cursorY == 7) board[cursorY][cursorX] = 4;

                        turn = (turn == 1) ? 2 : 1;
                        selectedX = -1; selectedY = -1;
                        checkWin();
                    }
                }
            }
        }
    }

    void run() override {
        FastLED.clear();
        
        if (winner != 0) {
            if ((custom_millis() / 300) % 2 == 0) {
                CRGB c = (winner == 1) ? CRGB::Red : CRGB::Blue;
                for (int i = 0; i < NUM_LEDS; i++) leds[i] = c;
            }
            return;
        }

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if ((x + y) % 2 != 0) {
                    int p = board[y][x];
                    CRGB c = CRGB(5, 5, 5); 
                    if (p == 1) c = CRGB::Red;
                    else if (p == 2) c = CRGB::Blue;
                    else if (p == 3) c = CRGB::Magenta; 
                    else if (p == 4) c = CRGB::Cyan;    

                    if (selectedX == x && selectedY == y && (custom_millis() / 250) % 2 == 0) c = CRGB::White;
                    setMatrixLed(x, y, c);
                }
            }
        }

        if (selectedX == -1) {
            if ((custom_millis() / 250) % 2 == 0) setMatrixLed(cursorX, cursorY, CRGB::White);
        } else {
            if (cursorX != selectedX || cursorY != selectedY) {
                if ((custom_millis() / 250) % 2 == 0) setMatrixLed(cursorX, cursorY, CRGB::Green);
            }
        }
    }
};

class BiancaApp : public App {
private:
    int scrollOffset = 0;
    unsigned long lastScrollTime = 0;
    
    const int textSeq[8] = {1, 2, 3, 4, 5, 6, 7, 0};
    
    const CRGB textColors[8] = { 
        CRGB::Aqua,    CRGB::Purple,  CRGB::Blue,    CRGB::Green,   
        CRGB::Orange,  CRGB::Red,     CRGB::Red,     CRGB::Black    
    };

public:
    void init() override {
        scrollOffset = 0;
        lastScrollTime = custom_millis();
    }
    
    void handleButtons(ButtonState btns) override {
        if (btns.control) { changeState(MENU); return; }
    }
    
    void run() override {
        FastLED.clear();
        
        if (custom_millis() - lastScrollTime > 240) { 
            lastScrollTime = custom_millis();
            scrollOffset = (scrollOffset + 1) % 64; 
        }
        
        for (int col = 0; col < 8; col++) {
            int globalCol = (col + scrollOffset) % 64;
            int charIndex = globalCol / 8;
            int symIndex = textSeq[charIndex];
            int colInsideSym = globalCol % 8;
            CRGB charColor = textColors[charIndex];
            
            for (int row = 0; row < 8; row++) {
                if (symbols[symIndex][row] & (0x80 >> colInsideSym)) {
                    setMatrixLed(col, row, charColor);
                }
            }
        }
    }
};

class TestApp : public App {
public:
    void handleButtons(ButtonState btns) override {
        if (btns.control) { 
            changeState(MENU); 
            return; 
        }
        
        if (btns.left) {
            targetBrightness -= 2;
            if (targetBrightness < 1) targetBrightness = 1;
        }
        
        if (btns.right) {
            targetBrightness += 2;
            if (targetBrightness > 255) targetBrightness = 255;
        }
    }

    void run() override {
        FastLED.clear();
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                setMatrixLed(x, y, CHSV(x * 32 + y * 4, 255, 255));
            }
        }
    }
};

MenuApp menuApp;
SnakeApp snakeApp;
Connect4App connect4App;
CheckersApp checkersApp;
BiancaApp biancaApp;
TestApp testApp;

App* activeApp = &menuApp;

void changeState(AppState newState) {
    switch(newState) {
        case MENU:         activeApp = &menuApp; break;
        case APP_SNAKE:    activeApp = &snakeApp; break;
        case APP_CONNECT4: activeApp = &connect4App; break;
        case APP_CHECKERS: activeApp = &checkersApp; break;
        case APP_BIANCA:   activeApp = &biancaApp; break;
        case APP_TEST:     activeApp = &testApp; break;
    }
    activeApp->init(); 
}

ISR(PCINT2_vect) {
    unsigned long interruptTime = custom_millis();
    if (interruptTime - lastInterruptTime > debounceDelay) {
        if (digitalRead(BTN_DOWN_PIN) == LOW) btnDown_ISR = true;
        else if (digitalRead(BTN_CONTROL_PIN) == LOW) btnControl_ISR = true;
        else if (digitalRead(BTN_RIGHT_PIN) == LOW) btnRight_ISR = true;
        else if (digitalRead(BTN_UP_PIN) == LOW) btnUp_ISR = true;
        else if (digitalRead(BTN_LEFT_PIN) == LOW) btnLeft_ISR = true;
        
        lastInterruptTime = interruptTime;
    }
}

void setup() {
    pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
    pinMode(BTN_CONTROL_PIN, INPUT_PULLUP);
    pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
    pinMode(BTN_UP_PIN, INPUT_PULLUP);
    pinMode(BTN_LEFT_PIN, INPUT_PULLUP);

    cli(); 
    PCICR |= (1 << PCIE2);    
    PCMSK2 |= (1 << PCINT19) | (1 << PCINT20) | (1 << PCINT21) | (1 << PCINT22) | (1 << PCINT23); 
    setupCustomMillis();
    sei();

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness((uint8_t)currentBrightness);

    activeApp->init();
}

void loop() {
    ButtonState btnState;
    btnState.up      = btnUp_ISR;      btnUp_ISR = false;
    btnState.down    = btnDown_ISR;    btnDown_ISR = false;
    btnState.left    = btnLeft_ISR;    btnLeft_ISR = false;
    btnState.right   = btnRight_ISR;   btnRight_ISR = false;
    btnState.control = btnControl_ISR; btnControl_ISR = false;

    activeApp->handleButtons(btnState);
    
    unsigned long currentMillis = custom_millis();
    if (currentMillis - lastFrameTime >= FRAME_DELAY) {
        lastFrameTime = currentMillis; 
        
        if (currentBrightness < targetBrightness) {
            currentBrightness += 0.5;
            if (currentBrightness > targetBrightness) currentBrightness = targetBrightness;
            FastLED.setBrightness((uint8_t)currentBrightness);
        } else if (currentBrightness > targetBrightness) {
            currentBrightness -= 0.5;
            if (currentBrightness < targetBrightness) currentBrightness = targetBrightness;
            FastLED.setBrightness((uint8_t)currentBrightness);
        }
        
        activeApp->run(); 
        FastLED.show();
    }
}
