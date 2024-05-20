#include <reg51.h>
#include <intrins.h>
#include <stdlib.h>

sbit SRCLK = P3 ^ 6;
sbit RCLK = P3 ^ 5;
sbit SER = P3 ^ 4;

sbit LEFT = P3 ^ 1;
sbit RIGHT = P3 ^ 0;
sbit UP = P3 ^ 2;
sbit DOWN = P3 ^ 3;

#define COMMONPORTS P0
#define SEGs P2

sbit beep = P2 ^ 5;

// unsigned char code COL[8] = {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};
unsigned char code COL[8] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};
// unsigned char code ROW[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
unsigned char code ROW[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

unsigned char snakeLength = 1;
unsigned char snakeX[31] = {4};
unsigned char snakeY[31] = {0};
unsigned char foodX, foodY;
// 0: up, 1: down, 2: left; 3: right
unsigned char direction = 0;
unsigned int timecount = 40000;
unsigned char count;
unsigned char level;
unsigned char score;

unsigned char code DIG_CODE[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
// DB     0H,0H,24H,0BDH,24H,0H,0H,00H
// DB     0H,24H,3CH,0A5H,3CH,24H,0H,00H
// DB     0H,3CH,0H,0BDH,0H,3CH,0H,00H
// DB     18H,24H,24H,10H,8H,24H,24H,18H
unsigned char code levelcode[4][8] =
    {
        {0x00, 0x00, 0x24, 0xBD, 0x24, 0x00, 0x00, 0x00},
        {0x00, 0x24, 0x3C, 0xA5, 0x3C, 0x24, 0x00, 0x00},
        {0x00, 0x3C, 0x00, 0xBD, 0x00, 0x3C, 0x00, 0x00},
        {0x18, 0x24, 0x24, 0x10, 0x08, 0x24, 0x24, 0x18}
};
// 32H,23H,0H,40H,0E6H,0C9H,49H,32H
unsigned char code logo[8] = {0x32, 0x23, 0x00, 0x40, 0xE6, 0xE9, 0x49, 0x32};
// DB     30H,48H,3DH,7FH,7FH,3DH,48H,30H
unsigned char code win[8] = {0x30, 0x48, 0x3D, 0x7F, 0x7F, 0x3D, 0x48, 0x30};

void delay(unsigned int);
void Hc595SendByte(unsigned char);

void displaySnake();
void moveSnake();
void controlSnake();

char snakeFood();
void eatBeep(int , int);
void snakeGrow();
void snakeDeath();

void displaySnakeLogo(unsigned char[]);
void loading();
void timeInit();

void displayLevel(unsigned char);
void levelSnake();

void victoryMelody();
void startGame();
void ending();

void moveSnake()
{
    unsigned char len;
    len = snakeLength - 1;
    for (len; len > 0; len--)
    {
        snakeX[len] = snakeX[len - 1];
        snakeY[len] = snakeY[len - 1];
    }
    switch (direction)
    {
    case 0:
        if (snakeY[0] == 7 && level != 3)
        {
            if (level == 4)
            {
                snakeY[0] = 0;
                snakeX[0] = (snakeX[0] + 4) % 8;
            }
            else
            {
                snakeY[0] = 0;
            }
        }
        else
        {
            snakeY[0]++;
        }
        break;
    case 1:
        if (snakeY[0] == 0 && level != 3)
        {
            if (level == 4)
            {
                snakeY[0] = 7;
                snakeX[0] = (snakeX[0] + 4) % 8;
            }
            else
            {
                snakeY[0] = 7;
            }
        }
        else
        {
            snakeY[0]--;
        }
        break;
    case 2:
        if (snakeX[0] == 7 && level != 3)
        {
            if (level == 4)
            {
                snakeX[0] = 0;
                snakeY[0] = (snakeY[0] + 4) % 8;
            }
            else
            {
                snakeX[0] = 0;
            }
        }
        else
        {
            snakeX[0]++;
        }
        break;
    case 3:
        if (snakeX[0] == 0 && level != 3)
        {
            if (level == 4)
            {
                snakeX[0] = 7;
                snakeY[0] = (snakeY[0] + 4) % 8;
            }
            else
            {
                snakeX[0] = 7;
            }
        }
        else
        {
            snakeX[0]--;
        }
        break;
    }
    snakeGrow();
    if (level != 1)
    {
        snakeDeath();
    }
}

void controlSnake()
{
    if (direction == 0 || direction == 1)
    {
        if (LEFT == 0)
        {
            direction = 2;
        }
        if (RIGHT == 0)
        {
            direction = 3;
        }
    }
    if (direction == 2 || direction == 3)
    {
        if (UP == 0)
        {
            direction = 0;
        }
        if (DOWN == 0)
        {
            direction = 1;
        }
    }
}

void displaySnake()
{
    unsigned char i;
    for (i = 0; i < snakeLength; i++)
    {
        Hc595SendByte(ROW[snakeY[i]]);
        COMMONPORTS = COL[snakeX[i]];
        delay(80);
        COMMONPORTS = 0xFF;
        Hc595SendByte(ROW[foodY]);
        COMMONPORTS = COL[foodX];
        delay(80);
        COMMONPORTS = 0xFF;
    }
}

char snakeInit()
{
    snakeLength = 1;
    // snakeX[0] = rand() % 8;
    // snakeY[0] = rand() % 8;
    snakeX[0] = 4;
    snakeY[0] = 2;
    return snakeLength, snakeX[0], snakeY[0];
}

char snakeFood()
{
    unsigned char i;
    // srand((int)time(0));
    foodX = rand() % 8;
    foodY = rand() % 8;
    for (i = 0; i < snakeLength; i++)
    {
        if (foodX == snakeX[i] && foodY == snakeY[i])
        {
            return snakeFood();
        }
    }
    return foodX, foodY;
}

void eatBeep(int frequency, int duration)
{
    unsigned int i;
    for (i = 0; i < duration; i++)
    {
        beep = 1;
        delay(frequency);
        beep = 0;
        delay(frequency);
    }
}

void victoryMelody()
{
    // Giai điệu chiến thắng với khoảng 10 nhịp
    unsigned char melody[] = {200, 150, 100, 150}; // Thời gian mỗi nhịp
    unsigned char i, j;

    for (j = 0; j < 5; j++)
    {
        for (i = 0; i < 50; i++)
        {
            beep = 1;
            delay(melody[j]);
            beep = 0;
            delay(melody[j]);
        }
    }
}

void snakeGrow()
{
    if (snakeX[0] == foodX && snakeY[0] == foodY)
    {
        snakeFood();
        eatBeep(100, 50);
        snakeX[snakeLength] = snakeX[snakeLength - 1];
        snakeY[snakeLength] = snakeY[snakeLength - 1];
        snakeLength++;
        switch (level)
        {
        case 1:
            score += 1;
            if (snakeLength % 3 == 0 && timecount > 20000)
                timecount -= 2000;
            if (score == 5)
            {
                delay(5000);
                loading();
                ending();
                displayLevel(level);
                //delay(10000);
                level++;
                score = 0;
                timecount=40000;
                controlSnake();
                displaySnake();
            }
            break;
        case 2:
            score += 1;
            if (snakeLength % 3 == 0 && timecount > 10000)
                timecount -= 3000;
            break;
        case 3:
            score += 1;
            if (snakeLength % 2 == 0 && timecount > 7000)
                timecount -= 4000;
            break;
        case 4:
            score += 1;
            if (snakeLength % 2 == 0 && timecount > 7000)
                timecount -= 4000;
            break;
        default:
            break;
        }
        if (score == 8)
        {
            loading();
            victoryMelody();
            displaySnakeLogo(win);
            delay(200);
            ending();
            startGame();
        }
    }
}

void snakeDeath()
{
    unsigned char i, temp;
    temp = 10;
    for (i = 4; i < snakeLength; i++)
    {
        if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i])
        {
            while (LEFT == 1)
            {
                loading();
                while (temp--)
                {
                    eatBeep(100, 50);
                    delay(10000);
                }
                ending();
            }
            startGame();
        }
    }
    if (level == 3)
    {
        if (snakeX[0] > 7 || snakeY[0] > 7)
        {
            while (LEFT == 1)
            {
                loading();
                while (temp--)
                {
                    eatBeep(100, 50);
                    delay(1000);
                }
                ending();
            }
            startGame();
        }
    }
}

void startGame()
{
    UP = DOWN = LEFT = RIGHT = 1;
    score = 0;
    timecount = 40000;
    levelSnake();
    timeInit();
    snakeFood();
}

void ending()
{
    unsigned char i, div_score, mod;
    while (LEFT == 1)
    {
        div_score = score;
        // SEGs = (SEGs << 2) | 3;
        for (i = 0; i < 3; i++)
        {
            mod = div_score % 10;
            SEGs = (1 << (i + 2));
            COMMONPORTS = DIG_CODE[mod];
            delay(200);
            div_score /= 10;
        }
    }
    snakeInit();
}

void displaySnakeLogo(unsigned char dis[])
{
    unsigned int i, j;
    while (LEFT == 1)
    {
        for (j = 0; j < 50; j++)
        {
            for (i = 0; i < 8; i++)
            {
                Hc595SendByte(0x00);
                COMMONPORTS = COL[i];
                Hc595SendByte(dis[i]);
                delay(2);
            }
        }
    }
}

void displayLevel(unsigned char lev)
{
    unsigned char i, j;

    for (j = 0; j < 50; j++)
    {
        for (i = 0; i < 8; i++)
        {
            Hc595SendByte(0x00);
            COMMONPORTS = COL[i];
            Hc595SendByte(levelcode[lev][i]);
            delay(2);
        }
    }
}

void levelSnake()
{
    unsigned char lev, dState, uState;
    lev = dState = uState = 0;
    while (RIGHT == 1)
    {
        displayLevel(lev);
        if (DOWN == 0)
        {
            if (dState == 0)
            {
                lev++;
                if (lev > 3)
                    lev = 0;
                dState = 1;
            }
            else
            {
                dState = 0;
            }
        }
        if (UP == 0)
        {
            if (uState == 0)
            {
                lev--;
                if (lev == 255)
                    lev = 3;
                uState = 1;
            }
            else
            {
                uState = 0;
            }
        }
    }
    switch (lev)
    {
    case 0:
        level = 1;
        loading();
        break;
    case 1:
        level = 2;
        loading();
        break;
    case 2:
        level = 3;
        loading();
        break;
    case 3:
        level = 4;
        loading();
        break;
    default:
        break;
    }
}

void loading()
{
    unsigned i, j, temp;
    temp = 255;
    for (i = 0; i < 30; i++)
    {
        Hc595SendByte(0x00);
        temp = temp >> 1;
        COMMONPORTS = temp;
        // delay(30);
        for (j = 0; j < 20; j++)
        {
            Hc595SendByte(0x18);
            delay(2);
        }
        delay(10000);
    }
}

void delay(unsigned int time)
{
    while (time--)
        ;
}

void timeInit()
{
    TMOD = 0X01;
    TH0 = 0X3C;
    TL0 = 0XB0;
    EA = 1;
    ET0 = 1;
    TR0 = 1;
}

void refreshSnake() interrupt 1
{
    TH0 = (65536 - timecount) / 256;
    TH1 = (65536 - timecount) % 256;
    count++;
    if (count == 20)
    {
        count = 0;
        moveSnake();
    }
}

void Hc595SendByte(unsigned char dat)
{
    unsigned char a;
    SRCLK = 0;
    RCLK = 0;
    for (a = 0; a < 8; a++)
    {
        SER = dat >> 7;
        dat <<= 1;

        SRCLK = 1;
        _nop_();
        _nop_();
        SRCLK = 0;
    }
    RCLK = 1;
    _nop_();
    _nop_();
    RCLK = 0;
}

void main()
{
    displaySnakeLogo(logo);
    delay(2000);
    startGame();
    while (1)
    {
        controlSnake();
        displaySnake();
    }
}
