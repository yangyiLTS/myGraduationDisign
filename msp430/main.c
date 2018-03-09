/********************************************************************
//���Ǳ�ҵ��Ƶ�Ƕ��ʽ����
//��P1.4��������źż���ʵʱ��Һ���� drop/minute
//����Ԥ����ٿ��Ʋ���������и���������
//P1.4���������ش����ļ����ź�  P1.0~P1.3�ǵ��ڰ�ť  P3.0~P3.4���Ʋ������
//���ߣ�yangyi
//ʱ�䣺2018.03.01
********************************************************************/

#include <msp430x14x.h>
#include "Config.h"                     //����������ͷ�ļ�����Ҫ����IO�˿���Ϣ
#define MAX_DROP 150
#define TARGET_DROP 60
uchar Flag = 0;                           //��־λ
uint Time = 0;                           //ʱ���������
uint count = 0;
uint set_drop = MAX_DROP;
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

//***********************************************************************
//             TIMERA��ʼ��������ΪUPģʽ����
//***********************************************************************
void TIMERA_Init(void)                                   //��������ģʽ��������0XFFFF�����ж�
{
  TACTL |= TASSEL1 + TACLR + ID0 + ID1 + MC0 + TAIE;     //SMCLK��ʱ��Դ��8��Ƶ����������ģʽ��������0XFFFF�����ж�
  TACCR0 = 999;
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
    P3OUT = 0x00;
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
    P3OUT = 0x00;
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
  case 10:count++;if(count > 60000) count = 0;break;                         //���ñ�־λFlag
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
    delay_ms(55);
    break;
  case 0x01:  if(set_drop<MAX_DROP) set_drop++; P1IFG=0x00; delay_ms(20); break;
  case 0x02:  if(set_drop>0) set_drop--; P1IFG=0x00; delay_ms(20); break;
  case 0x04: while(~P1IN & 0x04) motor_step(1); P1IFG=0x00; break;
  case 0x08: while(~P1IN & 0x08) motor_step(0); P1IFG=0x00; break;
  }
}



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
  _EINT();
  //LED8  = 0x00;
  //delay_ms(1000);
  //LED8  = 0xFF;
  LCD_clear();
  //LCD_write_char(0, 1, 'a');
  //LCD_write_int(15,0,6666666);
  //delay_ms(1000);
  uint quene[5];
  for(uchar i = 0;i<5;i++){
    quene[i] = 400;  
  }
  uchar now = 0;
  while(1){
    ulong total = 0;

    LCD_write_str(0,0,"SET:");
    LCD_write_int(6,0,set_drop);
    LCD_write_int(15,1,position);
    if(Flag){
      LCD_clear();
      Time -= 55;
      quene[now] = Time;
      //LCD_write_int(15,0,Time);
      for(uchar i = 0; i < 5; i++){
        total += (unsigned long)quene[i];  
      }
      total = 300000 / total;
      LCD_write_str(9,0,"CUR:");
      LCD_write_int(15,0,total);
      now++;
      
      if(now){                                  //����������ڵ���
        if(total > set_drop){
          int _ = total - set_drop; 
          while(_--)
            motor_step(0);
        }
        else if(total < set_drop){
          int _ = set_drop - total;
          while(_--)
            motor_step(1);
        }
      }
      
      Flag = 0;
      if (now == 4){
        now = 0;
      }
    }
  }
}
