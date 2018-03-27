/********************************************************************
//���Ǳ�ҵ��Ƶ�Ƕ��ʽ����
//��P1.4��������źż���ʵʱ��Һ���� drop/minute
//����Ԥ����ٿ��Ʋ���������и���������
//P1.4���������ش����ļ����ź�  P1.0~P1.3�ǵ��ڰ�ť  P3.0~P3.4���Ʋ������
//���ߣ�yangyi
//ʱ�䣺2018.03.01
********************************************************************/
//maxdrop = 95
#include <msp430x14x.h>
#include "Config.h"                     //����������ͷ�ļ�����Ҫ����IO�˿���Ϣ
#include "GUI.h"                        //GUIͷ�ļ�
#include "Ascii_8x16.h"                 //8x16��С�ַ�
#include "GB2424.h"                     //24x24���ش�С�ĺ���
//#include "GB2433.h"                     //24x33���ش�С�ĺ���
#include "Chinese.h"                    //16x16���ش�С�ĺ���
#include "GB2435.h"                     //24x35���ش�С�ĺ���
#include "Touch.h"                      //TFT��������ͷ�ļ�
#include "Touch.c"                      //TFT����������ʼ��������
#include "TFT28.h"                      //TFT��ʾͷ�ļ�
#include "TFT28.c"                      //TFT��ʾ������ʼ��������
#include "GUI.c"                        //GUI����

#define MAX_DROP 80
#define TARGET_DROP 60
#define VOLUME 22 * 550
uchar Flag = 1;                           //��־λ
uint Time = 0;                           //ʱ���������
uint count = 0;
uint set_drop = 60;
uint total = 0;
uint capa = VOLUME;
uint dropspeed;
uint dropq[5] = { 100, 100, 100, 100, 100 };
uint cur = 60;
uint Device_code;                    //TFT����IC�ͺţ�2.8��ΪILI9320
//int position = 0;
//***********************************************************************
//               MSP430IO�ڳ�ʼ��
//***********************************************************************
void Port_Init()
{
  
  P1SEL = 0x00;                   //P1��ͨIO����
  P1DIR = 0xE0;                   //P1����ģʽ���ⲿ��·�ѽ���������
  P1IE  = 0x1F;                   //����P1.0 1.1 1.4�ж�
  P1IES = 0x0F;                   //P1.0 P1.1�½��ط��ж� P1.4�����ش����ж�
  P1IFG = 0x00;                   //��������жϱ�־�Ĵ���
  
  P3SEL = 0x00;                   //�����
  P3DIR = 0xFF;
  P3OUT = 0x00;
  
  P4SEL = 0x00;                   //��ʾ�����ݿڣ����ƿ�
  P4DIR = 0xFF;                   //���ݿ����ģʽ
  P5SEL = 0x00;
  P5DIR|= BIT0 + BIT1 + BIT3 + BIT5 + BIT6 + BIT7;  //TFT��ʾ������
  
  P2SEL = 0x00;
  P2DIR |= BIT3 + BIT4 + BIT5 + BIT6;               //����������
  
  LED8DIR = 0xFF;                 //P6�����ģʽ
  LED8  = 0xFF;                   //�ȹر�����LED
}

//***********************************************************************
//             TIMERA��ʼ��������ΪUPģʽ����
//***********************************************************************
void TIMERA_Init(void)                                   //��������ģʽ��������0XFFFF�����ж�
{
  TACTL |= TASSEL1 + TACLR + ID0 + ID1 + MC0 + TAIE;     //SMCLK��ʱ��Դ��8��Ƶ����������ģʽ��������0XFFFF�����ж�
  TACCR0 = 9999;
}

void DisplayDesk(void){
  CLR_Screen(Black);
  GUIline(0,59,240,59,Yellow);
  GUIfull(0,60,60,320,Green);
  GUIline(61,60,61,320,Yellow);
  LCD_PutString24(65,65,"��ǰ�ٶ�", Yellow, Black);
  LCD_PutString24(65,125,"ʣ������", Yellow, Black);
  LCD_PutString24(65,185,"�趨�ٶ�", Yellow, Black);
  LCD_PutString24(65,245,"�趨����", Yellow, Black);
}

//**********************************************************************
//	����������ƺ���
//**********************************************************************
int position = 0;
uchar step_up[4] = {0x09,0x0A,0x06,0x05};  //��ת  
uchar step_down[4] = {0x05,0x06,0x0A,0x09};  //��ת
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
//               ˮ����ʱ������
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
//               չʾ��Ϣ
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
//             TIMERA�жϷ��������Ҫ�ж��ж�����
//***********************************************************************
#pragma vector = TIMERA1_VECTOR
__interrupt void Timer_A(void)
{
  switch(TAIV)                                  //��Ҫ�ж��жϵ�����
  {
  case 2:break;
  case 4:break;
  case 10:count++;if(count > 3000) {if(count > 6000) count = 0; dropspeed = 1;};break;                         //���ñ�־λFlag
  }
}
//**********************************************************************
//	P1���жϷ��������Ҫ�ж�
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
//           ������
//*************************************************************************
void main(void)
{ 
  WDT_Init();                                   //���Ź�����
  Clock_Init();                                 //ϵͳʱ������
  Port_Init();                                  //�˿ڳ�ʼ��
  LCD_init();                                   //Һ��������ʼ������
  TIMERA_Init();                                //����TIMERA
  Device_code=0x9320;                //TFT����IC�ͺ�
  TFT_Initial();                     //��ʼ��LCD	
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
