#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Seven_Segment.h"

unsigned char pencil[32] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xE0,0xF0,0x78,0x3C,0x1E,0x0F,0x1C,0x30
,0xF0,0x98,0xB8,0xDC,0x7E,0x0F,0x07,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

unsigned char eraser[32] = {
0x00,0x00,0x00,0xC0,0x20,0xD8,0xE6,0xF9,0xFD,0x3D,0xCD,0xF1,0xFD,0x7F,0x1E,0x00
,0xF8,0x94,0x93,0x94,0x97,0x97,0x97,0xF3,0xFC,0x7F,0x1F,0x07,0x01,0x00,0x00,0x00
};

volatile uint8_t KEY_Flag, release, lastkey, Bmp[128][64] = {0}, key_press;
volatile uint32_t index_5ms, cnt_5ms, cnt_100ms_1, cnt_100ms_2, cnt_50ms,index_key_scan, Key_time, long_press_flag;
volatile uint32_t digit[4];
volatile uint8_t seg_enable = 0;   // 0 = ????, 1 = ????

void TMR1_IRQHandler(void)
{
cnt_5ms++;
    index_5ms = cnt_5ms % 4;

    // ??????????
    CloseSevenSegment();

    // ? ??? seg_enable == 1 ?,???????
    if (seg_enable) {
        if (index_5ms == 0) {
            ShowSevenSegment(0,digit[0]);
        }
        if (index_5ms == 1)  {
            ShowSevenSegment(1,digit[1]);
        }
        if (index_5ms == 2)  {
            ShowSevenSegment(2,digit[2]);
        }
        if (index_5ms == 3)  {
            ShowSevenSegment(3,digit[3]);
        }
    }

if (KEY_Flag != 0 && KEY_Flag != 5) {
key_press++;
if(key_press > 60){
long_press_flag = 1;
}
} else {
key_press = long_press_flag = 0;
}

if (cnt_5ms % 20 == 0){
cnt_100ms_1 = 1;
cnt_100ms_2 = 1;
}
if (cnt_5ms % 10 == 0)
{
cnt_50ms++;
index_key_scan = cnt_50ms % 3;
if (index_key_scan == 0)
{
PA0=1; PA1=1; PA2=1; PA3=1; PA4=1; PA5=0;
}
if (index_key_scan == 1)
{
PA0=1; PA1=1; PA2=1; PA3=1; PA4=0; PA5=1;
}
if (index_key_scan == 2)
{
PA0=1; PA1=1; PA2=1; PA3=0; PA4=1; PA5=1;
}
NVIC_EnableIRQ(GPAB_IRQn);

if ((cnt_50ms - Key_time)>3)  { release = 1;}
}
TIMER_ClearIntFlag(TIMER1); // Clear Timer1 time-out interrupt flag
}


void GPAB_IRQHandler(void)
{
NVIC_DisableIRQ(GPAB_IRQn);
Key_time = cnt_50ms;
release = 0;

if (PA->ISRC & BIT0) {        // check if PA0 interrupt occurred
PA0=1;
PA->ISRC |= BIT0;         // clear PA0 interrupt status
if (PA3==0) { KEY_Flag =3; PA3=1;}
if (PA4==0) { KEY_Flag =6; PA4=1;}
if (PA5==0) { KEY_Flag =9; PA5=1;}
return;
}
if (PA->ISRC & BIT1) { // check if PA1 interrupt occurred
PA1=1;
PA->ISRC |= BIT1;         // clear PA1 interrupt status  
if (PA3==0) { KEY_Flag =2; PA3=1;}
if (PA4==0) { KEY_Flag =5; PA4=1;}
if (PA5==0) { KEY_Flag =8; PA5=1;}
return;
}
if (PA->ISRC & BIT2) { // check if PB14 interrupt occurred
PA2=1;
PA->ISRC |= BIT2;         // clear PA interrupt status  
if (PA3==0) { KEY_Flag =1; PA3=1;}
if (PA4==0) { KEY_Flag =4; PA4=1;}
if (PA5==0) { KEY_Flag =7; PA5=1;}
return;
}                     // else it is unexpected interrupts
PA->ISRC = PA->ISRC;      // clear all GPB pins
}


void Init_Timer1(void)
{
  TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 200);
  TIMER_EnableInt(TIMER1);
  NVIC_EnableIRQ(TMR1_IRQn);
  TIMER_Start(TIMER1);
}

void Init_KEY(void)
{
GPIO_SetMode(PA, (BIT0 | BIT1 | BIT2 |BIT3 | BIT4 | BIT5), GPIO_MODE_QUASI);
GPIO_EnableInt(PA, 0, GPIO_INT_FALLING);
GPIO_EnableInt(PA, 1, GPIO_INT_FALLING);
GPIO_EnableInt(PA, 2, GPIO_INT_FALLING);  
 NVIC_SetPriority(GPAB_IRQn,3);
GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_LIRC, GPIO_DBCLKSEL_256);
GPIO_ENABLE_DEBOUNCE(PA, (BIT0 | BIT1 | BIT2));
}

void Init_EXTINT(void)
{
    // Configure EINT1 pin and enable interrupt by rising and falling edge trigger
    GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);
    GPIO_EnableEINT1(PB, 15, GPIO_INT_RISING); // RISING, FALLING, BOTH_EDGE, HIGH, LOW
    NVIC_EnableIRQ(EINT1_IRQn);

    // Enable interrupt de-bounce function and select de-bounce sampling cycle time
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_LIRC, GPIO_DBCLKSEL_64);
    GPIO_ENABLE_DEBOUNCE(PB, BIT15);
}

void EINT1_IRQHandler(void)
{
int i,j;
for(i = 0;i < 128;i++){
for(j = 0;j < 64;j++){
Bmp[i][j] = 0;
}
}
    clear_LCD();
    GPIO_CLR_INT_FLAG(PB, BIT15); // Clear GPIO interrupt flag
}

void Draw_cover_pixel(int8_t x, int8_t y){
int i,j;
for(i = x;i < x+16;i++){
for(j = y;j < y+16;j++){
if(i >= 0 && i < 128 && j >= 0 && j < 64){
if(Bmp[i][j] == 1){
draw_Pixel(i, j, FG_COLOR, BG_COLOR);
}
}
}
}
}

int main(void)
{
int8_t flag = 1;
int8_t x=0,y=0,old_x=0,old_y=0,i = 0;
int8_t dirX=0;
int8_t dirY=0;

SYS_Init();
GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
GPIO_SetMode(PB, BIT11, GPIO_MODE_OUTPUT);
GPIO_SetMode(PB, BIT15, GPIO_MODE_OUTPUT);
init_LCD();
Init_KEY();
Init_EXTINT();
Init_Timer1();
clear_LCD();

index_5ms=cnt_5ms=cnt_100ms_1=cnt_100ms_2=cnt_50ms=0;
KEY_Flag = 0, lastkey = 0;
  release =0;
x=64; y=32;
old_x = x;
old_y = y;

draw_Bmp16x16(x - 8 ,y - 8,FG_COLOR,BG_COLOR,pencil);
printS_5x7(x - 16, y - 8 + 22, "Paint!");

while(KEY_Flag == 0);

    // ??????:?? "Paint!"
    KEY_Flag = 0;
    printS_5x7(x - 16, y - 8 + 22, "      ");

    // ? ??????
    digit[0] = x / 16;
    digit[1] = x % 16;
    digit[2] = y / 10;
    digit[3] = y % 10;
    seg_enable = 1;    // ??????,TMR1_IRQHandler ??????
while(1){

if(x < 8) x = 8;
if(y < 8) y = 8;
if(x > LCD_Xmax-8) x = LCD_Xmax-8;
if(y > LCD_Ymax-8) y = LCD_Ymax-8;


digit[0] = x / 16;
digit[1] = x % 16;
digit[2] = y / 10;
digit[3] = y % 10;

if(flag){
draw_Bmp16x16(x - 8 ,y - 8,FG_COLOR,BG_COLOR,pencil);
} else {
draw_Bmp16x16(x - 8 ,y - 8,FG_COLOR,BG_COLOR,eraser);
}

if(release == 1 || long_press_flag == 1){
switch(KEY_Flag){
case 1: dirX=-1;  dirY=-1; break;
case 2: dirX=0;  dirY=-1; break;
case 3: dirX=1;  dirY=-1; break;
case 4: dirX=-1; dirY=0;  break;
case 5:
if(flag){
draw_Bmp16x16(x - 8 ,y - 8,BG_COLOR,BG_COLOR,pencil);
} else {
draw_Bmp16x16(x - 8 ,y - 8,BG_COLOR,BG_COLOR,eraser);
}
flag = !flag;
break;
case 6: dirX=+1; dirY=0;  break;
case 7: dirX=-1;  dirY=1; break;
case 8: dirX=0;  dirY=+1; break;
case 9: dirX=1;  dirY=1; break;
}

if(KEY_Flag != 0 && KEY_Flag != 5 && long_press_flag == 0){

if(flag){
draw_Bmp16x16(x - 8 ,y - 8,BG_COLOR,BG_COLOR,pencil);
} else {
draw_Bmp16x16(x - 8 ,y - 8,BG_COLOR,BG_COLOR,eraser);
}

if(flag){
Bmp[x - 8][y + 7] = 1;
} else {
for(i = 0;i < 9;i++){
Bmp[x - 8 + i][y + 7] = 0;  
}
}

old_x = x;
old_y = y;

x = x + dirX;
y = y + dirY;

Draw_cover_pixel(old_x - 8,old_y - 8);

} else if (KEY_Flag != 0 && KEY_Flag != 5 && long_press_flag == 1 && cnt_100ms_2 == 1){
if(flag){
draw_Bmp16x16(x - 8 ,y - 8,BG_COLOR,BG_COLOR,pencil);
} else {
draw_Bmp16x16(x - 8 ,y - 8,BG_COLOR,BG_COLOR,eraser);
}

if(flag){
Bmp[x - 8][y + 7] = 1;
} else {
for(i = 0;i < 9;i++){
Bmp[x - 8 + i][y + 7] = 0;  
}
}

old_x = x;
old_y = y;

x = x + dirX;
y = y + dirY;

Draw_cover_pixel(old_x - 8,old_y - 8);

cnt_100ms_2 = 0;
}
if(release == 1) KEY_Flag = 0;
}
}
}
