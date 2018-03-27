/********************************************************************
//这是毕业设计的嵌入式部分
//对P1.4口输入的信号计算实时输液滴速 drop/minute
//按照预设滴速控制步进电机进行负反馈调节
//P1.4输入上升沿触发的计数信号  P1.0~P1.3是调节按钮  P3.0~P3.4控制步进电机
//作者：yangyi
//时间：2018.03.01
********************************************************************/
//maxdrop = 95
#include <msp430x14x.h>
#include "Config.h"                     //开发板配置头文件，主要配置IO端口信息
#include "GUI.h"                        //GUI头文件
#include "Ascii_8x16.h"                 //8x16大小字符
#include "GB2424.h"                     //24x24像素大小的汉字
//#include "GB2433.h"                     //24x33像素大小的汉字
#include "Chinese.h"                    //16x16像素大小的汉字
#include "GB2435.h"                     //24x35像素大小的汉字
#include "Touch.h"                      //TFT触摸操作头文件
#include "Touch.c"                      //TFT触摸操作初始化及函数
#include "TFT28.h"                      //TFT显示头文件
#include "TFT28.c"                      //TFT显示操作初始化及函数
#include "GUI.c"                        //GUI函数

#define MAX_DROP 80
#define TARGET_DROP 60
#define VOLUME 22 * 550
uchar Flag = 1;                           //标志位
uint Time = 0;                           //时间计数变量
uint count = 0;
uint set_drop = 60;
uint total = 0;
uint capa = VOLUME;
uint dropspeed;
uint dropq[5] = { 100, 100, 100, 100, 100 };
uint cur = 60;
uint Device_code;                    //TFT控制IC型号，2.8寸为ILI9320
//int position = 0;
//***********************************************************************
//               MSP430IO口初始化
//***********************************************************************
void Port_Init()
{
  
  P1SEL = 0x00;                   //P1普通IO功能
  P1DIR = 0xE0;                   //P1输入模式，外部电路已接上拉电阻
  P1IE  = 0x1F;                   //开启P1.0 1.1 1.4中断
  P1IES = 0x0F;                   //P1.0 P1.1下降沿发中断 P1.4上升沿触发中断
  P1IFG = 0x00;                   //软件清零中断标志寄存器
  
  P3SEL = 0x00;                   //电机口
  P3DIR = 0xFF;
  P3OUT = 0x00;
  
  P4SEL = 0x00;                   //显示屏数据口，控制口
  P4DIR = 0xFF;                   //数据口输出模式
  P5SEL = 0x00;
  P5DIR|= BIT0 + BIT1 + BIT3 + BIT5 + BIT6 + BIT7;  //TFT显示控制线
  
  P2SEL = 0x00;
  P2DIR |= BIT3 + BIT4 + BIT5 + BIT6;               //触摸控制线
  
  LED8DIR = 0xFF;                 //P6口输出模式
  LED8  = 0xFF;                   //先关闭所有LED
}

//***********************************************************************
//             TIMERA初始化，设置为UP模式计数
//***********************************************************************
void TIMERA_Init(void)                                   //连续计数模式，计数到0XFFFF产生中断
{
  TACTL |= TASSEL1 + TACLR + ID0 + ID1 + MC0 + TAIE;     //SMCLK做时钟源，8分频，连续计数模式，计数到0XFFFF，开中断
  TACCR0 = 9999;
}

void DisplayDesk(void){
  CLR_Screen(Black);
  GUIline(0,59,240,59,Yellow);
  GUIfull(0,60,60,320,Green);
  GUIline(61,60,61,320,Yellow);
  LCD_PutString24(65,65,"当前速度", Yellow, Black);
  LCD_PutString24(65,125,"剩余容量", Yellow, Black);
  LCD_PutString24(65,185,"设定速度", Yellow, Black);
  LCD_PutString24(65,245,"设定容量", Yellow, Black);
}

//**********************************************************************
//	步进电机控制函数
//**********************************************************************
int position = 0;
uchar step_up[4] = {0x09,0x0A,0x06,0x05};  //正转  
uchar step_down[4] = {0x05,0x06,0x0A,0x09};  //反转
void motor_step(uchar d){
  if (d){
    for(uchar i = 0; i < 4; i++){
      P3OUT = step_up[i];
      delay_ms(1);
    }
    position++;
    if(position == 20000){
      position = 0;
    }
    P3OUT &= 0xF0;
  }
  else{
    for(char i = 0; i < 4 ; i++){
      P3OUT = step_down[i];
      delay_ms(1);
    }
    position--;
    if(position < 0){
      position = 19999;
    }
    P3OUT &= 0xF0;
  }
}

//*************************************************************************
//               水滴下时处理函数
//*************************************************************************
int now = 0;
void drop_deal(){
  Time += 7;
  if (Time < 25){
    return;
  }
  capa--;
  dropq[now] = Time;
  uint total = 0;
  for(uchar i = 0; i < 5; i++){
    total += dropq[i];  
  }
  total = total / 5;
  dropspeed = 6000 / total;
  if (now == 4){
    now = 0;
  }
  else {
    now++;
  }
}

void TFT_write_int(unsigned short x, unsigned short y, unsigned int data)
{
  if(data >= 10){
    TFT_write_int(x - 8, y, data / 10);
  }
  LCD_PutChar(x, y, data % 10 + 0x30, Blue2, Black);
} 

void TFT_show(){
  uint progress = (VOLUME/22 - capa/22)*280 / (VOLUME/22) + 60;
  uint resttime = capa / dropspeed;
  GUIline(0,progress,60,progress,Black);
  GUIfull(65,95,121,105,Black);
  TFT_write_int(120,95, dropspeed);
  LCD_PutString(136,95," mL/s",Blue,Black);
  GUIfull(70,155,136,165,Black);
  TFT_write_int(120,155, capa/22);
  LCD_PutString(136,155," mL",Blue,Black);
  TFT_write_int(120, 215, set_drop);
  LCD_PutString(136,215, " mL/s",Blue,Black);
  TFT_write_int(120, 275, VOLUME / 22);
  LCD_PutString(136,275," mL",Blue,Black);
  LCD_PutSingleChar2435(108,13,10,Yellow,Black);
  uchar h1,h2,m1,m2;
  h1 = resttime / 600;
  h2 = resttime / 60 % 10;
  m1 = resttime % 60 / 10;
  m2 = resttime % 10;
  LCD_PutSingleChar2435(60,13,h1,Yellow,Black);
  LCD_PutSingleChar2435(84,13,h2,Yellow,Black);
  LCD_PutSingleChar2435(132,13,m1,Yellow,Black);
  LCD_PutSingleChar2435(156,13,m2,Yellow,Black);
}

//*************************************************************************
//               展示信息
//*************************************************************************
void LCD_Show(){
  LCD_clear();
  LCD_write_str(0,0,"SET:");
  LCD_write_int(6,0,set_drop);
  LCD_write_int(15,1,position);
  LCD_write_int(2,1,capa / 22);
  LCD_write_str(3,1, "mL");
  LCD_write_str(9,0,"CUR:");
  LCD_write_int(15,0,dropspeed);
}


void speed_adjust(){
  if(now % 1 == 0){
    uint para; 
    switch(dropspeed / 10){
    case 4:para = 3;break;
    case 3:para = 3;break;
    case 2:para = 5;break;
    case 1:para = 8;break;
    case 0:para = 8;break;
    default: para = 2;break;
    }
    if(dropspeed > set_drop){
      uchar _ = (dropspeed - set_drop) * 2 / para + 1; 
      while(_--)
      motor_step(0);
    }
    else if(dropspeed < set_drop){
      uchar _ = (set_drop - dropspeed) * 2 / para + 1;
      while(_--)
      motor_step(1);
    }
  }
}

//***********************************************************************
//             TIMERA中断服务程序，需要判断中断类型
//***********************************************************************
#pragma vector = TIMERA1_VECTOR
__interrupt void Timer_A(void)
{
  switch(TAIV)                                  //需要判断中断的类型
  {
  case 2:break;
  case 4:break;
  case 10:count++;if(count > 3000) {if(count > 6000) count = 0; dropspeed = 1;};break;                         //设置标志位Flag
  }
}
//**********************************************************************
//	P1口中断服务程序，需要判断
//**********************************************************************
#pragma vector = PORT1_VECTOR
__interrupt void P1_IRQ(void)
{
  switch(P1IFG & 0x1F){
  case 0x10:
    Time = count;
    count = 0;
    Flag = 1;
    P1IFG=0x00;
    delay_ms(70);
    drop_deal();
    //LCD_Show();
    //speed_adjust();
    break;
  case 0x01:  if(set_drop<MAX_DROP) set_drop++; P1IFG=0x00; delay_ms(50); break;
  case 0x02:  if(set_drop>0) set_drop--; P1IFG=0x00; delay_ms(50); break;
  case 0x04: while(~P1IN & 0x04) motor_step(1); P1IFG=0x00; break;
  case 0x08: while(~P1IN & 0x08) motor_step(0); P1IFG=0x00; break;
  }
}
/*
void LCD_write_int2(unsigned char x,unsigned char y,unsigned int data)
{
  uchar s[16];
  sprintf(s, "%d", data);
  LCD_write_str(x, y, s);
} */



//*************************************************************************
//           主函数
//*************************************************************************
void main(void)
{ 
  WDT_Init();                                   //看门狗设置
  Clock_Init();                                 //系统时钟设置
  Port_Init();                                  //端口初始化
  LCD_init();                                   //液晶参数初始化设置
  TIMERA_Init();                                //设置TIMERA
  Device_code=0x9320;                //TFT控制IC型号
  TFT_Initial();                     //初始化LCD	
  //LCD_clear();
  DisplayDesk();
  _EINT();
  while (1){
    LCD_Show();
    TFT_show();
    if(Flag){
      speed_adjust();
      Flag = 0;
      delay_ms(1500);
    }
    delay_ms(1000);
  }
}
