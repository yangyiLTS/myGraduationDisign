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
//#include <stdio.h>
#define MAX_DROP 80
#define TARGET_DROP 60
uchar Flag = 0;                           //标志位
uint Time = 0;                           //时间计数变量
uint count = 0;
uint set_drop = 60;
uint total = 0;
uint capa = 22 * 500;
uint dropspeed;
uint dropq[5] = { 100, 100, 100, 100, 100 };
uint cur;

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
  P5DIR|= BIT5 + BIT6 + BIT7;     //控制口设置为输出模式
  
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
  case 10:count++;if(count > 6000) count = 6000;break;                         //设置标志位Flag
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
  _EINT();
  LCD_clear();
  while (1){
    LCD_Show();
    if(Flag){
      speed_adjust();
      Flag = 0;
      delay_ms(500);
    }
    delay_ms(2000);
  }
  /*uint quene[5];
  for(uchar i = 0;i<5;i++){
    quene[i] = 400;  
  }
  uchar now = 0;
  while(1){
    LCD_write_str(0,0,"SET:");
    LCD_write_int(6,0,set_drop);
    LCD_write_int(15,1,position);
    if(Flag){
      capa--;
      LCD_clear();
      Time += 7;
      LCD_write_int(2,1,capa / 20);
      LCD_write_str(3,1, "mL");
      quene[now] = Time;
      //LCD_write_int(15,0,Time);
      total = 0;
      for(uchar i = 0; i < 5; i++){
        total += quene[i];  
      }
      total = total / 5;
      uint cur = 6000 / total;
      LCD_write_str(9,0,"CUR:");
      LCD_write_int(15,0,cur);
      if(now % 3 == 0){                                  //启动电机调节滴速
        uint para; 
        switch(cur / 10){
        case 4:para = 3;break;
        case 3:para = 3;break;
        case 2:para = 5;break;
        case 1:para = 8;break;
        case 0:para = 8;break;
        default: para = 1;break;
        }
        if(cur > set_drop){
          int _ = (cur - set_drop) * 2 / para + 1; 
          while(_--)
            motor_step(0);
        }
        else if(cur < set_drop){
          int _ = (set_drop - cur) * 2 / para + 1;
          while(_--)
            motor_step(1);
        }
      }
      if (now == 4){
        now = 0;
      }
      else {
        now++;
      }
      Flag = 0;
    }
    if (capa == 0){
      while(position != 19880){
        motor_step(0);
      }
      set_drop = 0;
    }
  }*/
}
