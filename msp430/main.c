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
//#include <stdio.h>
#define MAX_DROP 80
#define TARGET_DROP 60
uchar Flag = 0;                           //��־λ
uint Time = 0;                           //ʱ���������
uint count = 0;
uint set_drop = 60;
uint total = 0;
uint capa = 20 * 500;
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
  P5DIR|= BIT5 + BIT6 + BIT7;     //���ƿ�����Ϊ���ģʽ
  
  LED8DIR = 0xFF;                 //P6�����ģʽ
  LED8  = 0xFF;                   //�ȹر�����LED
}

//*************************************************************************
//               MSP430���ڳ�ʼ��
//*************************************************************************
void UART_Init()
{
  U0CTL|=SWRST;               //��λSWRST
  U0CTL|=CHAR;                //8λ����ģʽ 
  U0TCTL|=SSEL1;              //SMCLKΪ����ʱ��
  U0BR1=baud_h;               //BRCLK=8MHZ,Baud=BRCLK/N
  U0BR0=baud_l;               //N=UBR+(UxMCTL)/8
  U0MCTL=0x00;                //΢���Ĵ���Ϊ0��������9600bps
  ME1|=UTXE0;                 //UART1����ʹ��
  ME1|=URXE0;                 //UART1����ʹ��
  U0CTL&=~SWRST;
  IE1|=URXIE0;                //�����ж�ʹ��λ
  
  P3SEL|= BIT4;               //����IO��Ϊ��ͨI/Oģʽ
  P3DIR|= BIT4;               //����IO�ڷ���Ϊ���
  P3SEL|= BIT5;
}

//*************************************************************************
//              ����0�������ݺ���
//*************************************************************************

void Send_Byte(uchar data)
{
  while((IFG1&UTXIFG0)==0);          //���ͼĴ����յ�ʱ��������
    U0TXBUF=data;
}

//***********************************************************************
//             TIMERA��ʼ��������ΪUPģʽ����
//***********************************************************************
void TIMERA_Init(void)                                   //��������ģʽ��������0XFFFF�����ж�
{
  TACTL |= TASSEL1 + TACLR + ID0 + ID1 + MC0 + TAIE;     //SMCLK��ʱ��Դ��8��Ƶ����������ģʽ��������0XFFFF�����ж�
  TACCR0 = 9999;
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
      P3OUT |= step_up[i];
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
      P3OUT |= step_down[i];
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
//               �������Դ��� 0 �Ľ����ж�
//*************************************************************************
#pragma vector=UART0RX_VECTOR
__interrupt void UART0_RX_ISR(void)
{
  uchar data=0;
  data=U0RXBUF;                       //���յ������ݴ�����
  Send_Byte('R');                    //�����յ��������ٷ��ͳ�ȥ
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
  case 10:count++;if(count > 6000) count = 6000;break;                         //���ñ�־λFlag
  }
}
//**********************************************************************
//	P1���жϷ��������Ҫ�ж�
//**********************************************************************
#pragma vector = PORT1_VECTOR
__interrupt void P1_IRQ(void)
{
  //LED8  = 0x00;
  //delay_ms(100);
  //LED8  = 0xFF;
  switch(P1IFG & 0x1F){
  case 0x10:
    Time = count;
    count = 0;
    Flag = 1;
    P1IFG=0x00;
    delay_ms(70);
    //LCD_clear();
    //LCD_write_int(15,0,total);
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
  UART_Init();
  _EINT();
  LCD_clear();
  uint quene[5];
  for(uchar i = 0;i<5;i++){
    quene[i] = 400;  
  }
  uchar now = 0;
  while(1){
    LCD_write_str(0,0,"SET:");
    LCD_write_int(6,0,set_drop);
    LCD_write_int(15,1,position);
    UART_Init();
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
      if(now % 3 == 0){                                  //����������ڵ���
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
  }
}
