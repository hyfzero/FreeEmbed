#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "global.h"
#include "spi.h"
#include "power.h"
#include "OTPRW.h"
#include "IRC.h"
#include "Config.h"
#include "ADC.h"
#include "LCD.h"
#include "wx_i2c.h"
#include    "mc30f6910.h"
#include    "MTPRW.h"
#include "MC32P5232Pro.h"

u8     crc4_result = 0;

/*******************************************************
 * function name	: CountCRC_4
 * brief                ??CRC4
 *                        CRC_4_POLYNOMIALS  0x03
 *                        INIT               0x00
 *                        REFIN              TRUE
 *                        REFOUT             TRUE 
 *                        XOROUT             0x00
 * input                : ???????????????μ????CRC??????
 * output               : CRCУ????
 ********************************************************/
u8 CountCRC_4(u8 data,u8 crc,u8 len)
{
  u8 i;
//  u8 crc = 0;
  
//  if(crc & 0x01)
//  crc ^= data; 

 for(i=0;i<len;i++)
 {
    if(crc & 0x08)
    {
        if(data&(0x01<<(len-1))) crc =  crc <<1;
        else     crc = (crc <<1) ^ CRC_4_POLYNOMIALS ;
    }      
    else      
    {
        if(data&(0x01<<(len-1))) crc = (crc <<1) ^ CRC_4_POLYNOMIALS ;
        else     crc = crc <<1;
    }
    
    data <<= 1;    
//    data >>= 1;
 }
  
  return crc;  
}


void SDIO_Turnto_Input(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
    DP_DIR_1_B_to_A;
        
}
void SDIO_Turnto_Output(void)
{
    DP_DIR_1_A_to_B;
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
} 

u8 MTP_MODE_IN()
{
    u16 temp_count,time_out_count;
    u32 adc_sum,adc_value;
    u16 temp_adc;
    u8 flag_ok;
    u16 chip_read_id;
    u16 status;
    u8 chip_mode;
    
    flag_ok=0;
    POWER_OFF(vpp00,vdd00);
    CLN_VDD2GND_On;  //power off 时打开

    //power down detect
    ADC_RegularChannelConfig(ADC1,ADC_Channel_1, 1,ADC_SampleTime_1Cycles5);

    time_out_count=0;
    do{
        adc_sum=0;
        for(temp_count=0;temp_count<32;temp_count++)
        {
            
            while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);    //wait for convert complete
            temp_adc=ADC_GetConversionValue(ADC1);
            adc_sum +=temp_adc;
            Delay_1us(10);
        }

        adc_value=adc_sum>>5;
        adc_value=adc_value*46793; //0.0046793
        time_out_count++;
        if(time_out_count>6000)
        {
            return 0;
        }

    }while(adc_value>5000000);  //0.5v

    POWER_ON(vpp00,vdd30);

    time_out_count=0;
    do{
        adc_sum=0;
        for(temp_count=0;temp_count<32;temp_count++)
        {
             while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);    //wait for convert complete
            temp_adc=ADC_GetConversionValue(ADC1);
            adc_sum +=temp_adc;
        }
        adc_value=adc_sum>>5;
        adc_value=adc_value*46793; //0.0046793
        time_out_count++;
        if(time_out_count>6000)
        {
            return 0;
        }
    }while(adc_value<16000000);  //1.6v

    
    //mode in clk input
    Delay_1us(300);
    
//    Delay_1ms(20);//zqq
    
    time_out_count=0;
    do{
        SDIO_Turnto_Output();
         LineReset();

         Delay_1us(10);
         SendByte(0x53);  //0x53
         SendByte(0x69);  //0x44
         SendByte(0x6e);  //0x42
         SendByte(0x6f);  //0x75
         SendByte(0x6d);  //0x73
         SendByte(0x63);
         SendByte(0x75);
         SendByte(0x0d);
         
         LineReset();

        //Delay_1us(BIT_DELAY_1US);
//        Delay_1ms(30);
         
         //read MODE[3:0]
         chip_mode=MTP_Read_Word_mode(0);
         
         if(chip_mode==CHIP_PROG_MODE || chip_mode==CHIP_DEBUG_MODE || chip_mode==CHIP_TEST_MODE || chip_mode==CHIP_WAIT_MODE)
         {
           if(chip_mode==CHIP_PROG_MODE || chip_mode==CHIP_DEBUG_MODE)
           {
              MTP_Write_Word(EXIT_TO_WAIT,RW_TYPE_CTL); //退出到wait模式         
           }
           if(chip_mode==CHIP_TEST_MODE)
           {
              //--------------按秀峰要求退出时要清测试模式寄存器，还得写一条无效，不然退出模式后芯片状态会乱 zqq 2020.11.30-------------
              MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
              MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
              MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF),RW_TYPE_DATA); 
              MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);              
              MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
              MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
                     
              MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
                     
              MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    //退出测试模式             
           }
           MTP_Write_Word(RELOAD_OPTION_MODE,RW_TYPE_CTL);//配置字重载   
           flag_ok=1;
         }
         else
         {
           flag_ok=0;
         }
         
         time_out_count++;
         if(time_out_count>100)
         {
            return 0;
         }

      }while(flag_ok==0);
         
      Delay_1ms(5); //wait vdd=5v

      //read MCU ID
      do{      
          MTP_Write_Word(MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID_addr,RW_TYPE_ADDR);
          chip_read_id=MTP_Read_Word(RW_TYPE_DATA);

          chip_read_id &=0xfff0; //just check 0xa38x
          if(chip_read_id==(MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID & 0xfff0))
              flag_ok=1;
          else
              flag_ok=0;

          time_out_count++;
          if(time_out_count>100)
          {
              return 0;
          }
      }while(flag_ok==0);

    
//    time_out_count=0;
//    do
//    {
//      status=MTP_Read_Word(RW_TYPE_STATUS);
//    
//        time_out_count++;
//        if(time_out_count>1000)
//        {
//            return 0;
//        }
//    
//    }while((status!=0x000F)&&(status!=0x001D));
    
//    //+++++++++++++++++++++++++++++++++++++++++++++++++   //zqq
//        MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);       
//     // 进模式之后和加密擦除时先将UOPTBIT1和UOPTBIT2的映射地址擦除
//        
//        MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
//        MTP_Write_Word(0x0000,RW_TYPE_STATUS);
//        MTP_Write_Word(0x9003,RW_TYPE_ADDR);
//        MTP_Write_Word(0x0005,RW_TYPE_DATA);        
//        MTP_Write_Word(PROG_CMD_PROG,RW_TYPE_CTL);
//        MTP_Erase_Prog(PROG_TIME0,PROG_TIME1,PROG_TIME2);        
//        
//        
//        MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
//        MTP_Write_Word(0x0000,RW_TYPE_STATUS);
//        MTP_Write_Word(0x9001,RW_TYPE_ADDR);
//        MTP_Write_Word(0x0000,RW_TYPE_DATA);        
//        MTP_Write_Word(PROG_CMD_PROG,RW_TYPE_CTL);
//        MTP_Erase_Prog(PROG_TIME0,PROG_TIME1,PROG_TIME2);
//        
//        Delay_1us(10);
//        
//        MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
//        MTP_Write_Word(0x0000,RW_TYPE_STATUS);
//        MTP_Write_Word(0x9002,RW_TYPE_ADDR);
//        MTP_Write_Word(0x0000,RW_TYPE_DATA);        
//        MTP_Write_Word(PROG_CMD_PROG,RW_TYPE_CTL);
//        MTP_Erase_Prog(PROG_TIME0,PROG_TIME1,PROG_TIME2);
//        
//        //exit prog mode
//        MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);
    
    Delay_1ms(10); //wait vdd=5v
    return flag_ok;
}


void SendByte(u8 sdata)
{
    u8 i;
    for(i=0;i<8;i++)
    {
        OTP_SCK_0;  //下降沿改变数据，上升沿采样
        if(sdata&0x80)
            OTP_SDIO_1;
        else
            OTP_SDIO_0;
        Delay_1us(BIT_DELAY_1US);
        OTP_SCK_1;
        sdata<<=1;
        Delay_1us(BIT_DELAY_1US);   
    }
    OTP_SCK_0;
}

void SendWord(u16 sdata)
{
    u8 i;

    /*????Trn3+data????????SDIO??????*/
    SDIO_Turnto_Output(); 
    //Trn3
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    crc4_result = CountCRC_4(1,crc4_result,1);    //pq

    //data[15:0]
    for(i=0;i<16;i++)
    {
        OTP_SCK_0;  //?????????????????????
        if(sdata&0x8000)
        {
          OTP_SDIO_1;
          crc4_result = CountCRC_4(1,crc4_result,1);    //pq
        }
        else
        {
          OTP_SDIO_0;
          crc4_result = CountCRC_4(0,crc4_result,1);    //pq
        }            
        Delay_1us(BIT_DELAY_1US);
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
        sdata<<=1;  
    }
    OTP_SCK_0;
  
  
}

void SendWord_erase(u16 sdata)
{
    u8 i;

//    //Trn3
//    OTP_SCK_0;
//    Delay_1us(BIT_DELAY_1US);
//    OTP_SCK_1;
//    SDIO_Turnto_Output();
//
//    for(i=0;i<16;i++)
//    {
//        OTP_SCK_0;  //下降沿改变数据，上升沿采样
//        if(sdata&0x8000)
//            OTP_SDIO_1;
//        else
//            OTP_SDIO_0;
//        //Delay_1us(BIT_DELAY_1US);
//        OTP_SCK_0;
//        OTP_SCK_0;
//        OTP_SCK_1;
//        OTP_SCK_1;
//        sdata<<=1;
//        //Delay_1us(BIT_DELAY_1US);   
//    }
//    OTP_SCK_0;
//    Delay_1us(BIT_DELAY_1US);

    //Trn3
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    SDIO_Turnto_Output();

    for(i=0;i<16;i++)
    {
      if(i==2||i==13)
      {
         OTP_SCK_0;  //下降沿改变数据，上升沿采样
        if(sdata&0x8000)
          {
            OTP_SDIO_1;
            Delay_1us(8);
          }
        else
          {
            OTP_SDIO_0;
            Delay_1us(8);
          }
        //Delay_1us(BIT_DELAY_1US);
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_1;
        OTP_SCK_1;
        OTP_SCK_1;
        Delay_1us(8);
        sdata<<=1;
        //Delay_1us(BIT_DELAY_1US); 
      }      
      else if(i==5||i==6||i==7||i==8)
      {
         OTP_SCK_0;  //下降沿改变数据，上升沿采样
        if(sdata&0x8000)
          {
            OTP_SDIO_1;
            Delay_1us(800);
          }
        else
          {
            OTP_SDIO_0;
            Delay_1us(400);
          }
        //Delay_1us(BIT_DELAY_1US);
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_1;
        OTP_SCK_1;
        Delay_1us(400);
        sdata<<=1;
      
      }
      if(i==11)
      {
         OTP_SCK_0;  //下降沿改变数据，上升沿采样
        if(sdata&0x8000)
          {
            OTP_SDIO_1;
            Delay_1us(25);
          }
        else
          {
            OTP_SDIO_0;
            Delay_1us(25);
          }
        //Delay_1us(BIT_DELAY_1US);
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_1;
        OTP_SCK_1;
        Delay_1us(25);
        sdata<<=1;
        //Delay_1us(BIT_DELAY_1US); 
      }    
      else if(i==0||i==1||i==3||i==4||i==9||i==10||i==12||i==14||i==15)
      {
        OTP_SCK_0;  //下降沿改变数据，上升沿采样
        if(sdata&0x8000)
            OTP_SDIO_1;
        else
            OTP_SDIO_0;
        //Delay_1us(BIT_DELAY_1US);
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_1;
        OTP_SCK_1;
        sdata<<=1;
        //Delay_1us(BIT_DELAY_1US); 
      }
    }
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);  
  
  
}



u16 ReadWord()
{
    u16 temp =0,receivedata=0;
    u8  i,temp_h,temp_l;

    /*????Trn3+data????????SDIO???????*/
    SDIO_Turnto_Input();
    //Trn3
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    crc4_result = CountCRC_4(1,crc4_result,1);    //pq

    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);//23-5-9 add
    
    for(i=0;i<16;i++)
    {
        temp<<=1;
    
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US); 
        OTP_SCK_0;
        Delay_1us(BIT_DELAY_1US); 

        temp|=READ_SDIO_DATA;       
    }
    temp_h = temp >>8;
    temp_l = temp;
    crc4_result = CountCRC_4(temp_h,crc4_result,8);    //pq
    crc4_result = CountCRC_4(temp_l,crc4_result,8);    //pq
    
    receivedata=temp;
    return receivedata; 
}

void LineReset(void)
{
    u8 temp_count;
    //more than 60 clk bit-H
    for(temp_count=0;temp_count<55;temp_count++)
    {
        OTP_SCK_0;
        OTP_SDIO_1;
        Delay_1us(BIT_DELAY_1US); //1US
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
    }
    //more than 2 clk bit-L
    for(temp_count=0;temp_count<2;temp_count++)
    {
        OTP_SCK_0;
        OTP_SDIO_0;
        Delay_1us(BIT_DELAY_1US); //1US
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
    }

    OTP_SCK_0;
}

//-----------分段函数--------------------

//Name: MTP_Start()
//Description:  Start +Trn0 +mode+status 
u8 MTP_Start()
{
    u8 mode_temp,status_temp;
    u8 i,temp;
    
    crc4_result = 0;//?????CRC4
    
    /*????start????????SDIO??????*/
    SDIO_Turnto_Output();
    //start
    OTP_SCK_0;
    OTP_SDIO_1;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    crc4_result = CountCRC_4(1,crc4_result,1);    //pq

    /*????trn&mode????????SDIO??????*/
    SDIO_Turnto_Input();  
    //Trn
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    crc4_result = CountCRC_4(1,crc4_result,1);    //pq

    //mode[3:0]
    OTP_SCK_0;
    temp=0;
    for(i=0;i<4;i++)
    {
        temp<<=1;
        
        Delay_1us(BIT_DELAY_1US);
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);        
        OTP_SCK_0;

        temp|=READ_SDIO_DATA;//SDIO??????????????????????????????
    }
    mode_temp=temp;
    crc4_result = CountCRC_4(mode_temp,crc4_result,4);    //pq
    
    temp=0;
    for(i=0;i<2;i++)
    {
        temp<<=1;
        
        Delay_1us(BIT_DELAY_1US);
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);        
        OTP_SCK_0;

        temp|=READ_SDIO_DATA;//SDIO??????????????????????????????
    }        
    if(mode_temp==0x01)
      status_temp=(temp|=0x03);
    else
      status_temp=temp;
    crc4_result = CountCRC_4(status_temp,crc4_result,2);    //pq     

    return mode_temp;

}

//------------------------------
//Funciton: MTP_ACK
//Description: Trn1+ACK
void MTP_ACK()
{
    /*????trn+ack????????SDIO??????*/
    SDIO_Turnto_Output();    
    //Trn
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    crc4_result = CountCRC_4(1,crc4_result,1);    //pq
    
    //nACK
    OTP_SCK_0;
    OTP_SDIO_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1; 
    Delay_1us(BIT_DELAY_1US); 
    crc4_result = CountCRC_4(0,crc4_result,1);    //pq     
}

void MTP_NACK()
{
    /*????trn+nack????????SDIO??????*/
    SDIO_Turnto_Output();  
     //Trn4
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    crc4_result = CountCRC_4(1,crc4_result,1);    //pq
    
    //NACK
    OTP_SCK_0;
    OTP_SDIO_1;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US); 
    crc4_result = CountCRC_4(1,crc4_result,1);    //pq      
}

//---------------------------
//functinName:  MTP_TYPE_RW
//Description:  Trn2+type +r/w
void MTP_TYPE_RW(u8 type_data,u8 rw_flag)
{
 u8 i;

    /*????Trn2+type +r/w????????SDIO??????*/
    SDIO_Turnto_Output(); 
    //Trn2
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    crc4_result = CountCRC_4(1,crc4_result,1);    //pq
    
    //type[1:0]
    for(i=0;i<2;i++)
    {
        OTP_SCK_0;
        if(type_data & 0x02)
        {
          OTP_SDIO_1;
          crc4_result = CountCRC_4(1,crc4_result,1);    //pq
        }            
        else
        {
          OTP_SDIO_0;
          crc4_result = CountCRC_4(0,crc4_result,1);    //pq
        }    
        Delay_1us(BIT_DELAY_1US);
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
        type_data <<=1;
    }
    
    //R/W
    OTP_SCK_0;
    if(rw_flag==1)
    {
      OTP_SDIO_1;     //L write
      crc4_result = CountCRC_4(1,crc4_result,1);    //pq
    }   
    else
    {
      OTP_SDIO_0;
      crc4_result = CountCRC_4(0,crc4_result,1);    //pq
    }
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    
    OTP_SCK_0;
}

//-----------------------
//FunctionName: MTP_STOP
//Description: Trn5+crc+Trn6+Stop
u8 MTP_STOP()
{
    u8 crc_temp;
    u8 i,temp;
    u8 reCRC_flag;
    
    /*????Trn5+crc????????SDIO???????*/
    SDIO_Turnto_Input();
    //Trn5
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US); 
    
    OTP_SCK_0;
    temp=0;
    for(i=0;i<4;i++)
    {
        temp<<=1;
        
        Delay_1us(BIT_DELAY_1US);
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);        
        OTP_SCK_0;

        temp|=READ_SDIO_DATA;
    }        
    crc_temp=temp; 
    
    if((crc4_result & 0x0f) == (crc_temp & 0x0f))
    {
        reCRC_flag = TRUE;
    }
    else
    {
        reCRC_flag = FALSE;
    }

    /*????Trn6+stop????????SDIO??????*/
    SDIO_Turnto_Output(); 
    //Trn6
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);

    //stop
    OTP_SCK_0;
    OTP_SDIO_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;    
    Delay_1us(BIT_DELAY_1US);

    return reCRC_flag;
}

//========================================
void MTP_Write_Word(u16 buf_data,u8 type_data)
{
    MTP_Start();
    MTP_ACK();
    MTP_TYPE_RW(type_data,0); //write
    SendWord(buf_data);

    MTP_NACK();
    MTP_STOP();
}

void TEST_STATUS_dig_signal()
{
    u8 mode_temp,status_temp;

    u8 i,temp;

    SDIO_Turnto_Output();
    //starte
    OTP_SCK_0;
    OTP_SDIO_1;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;

    //Trn
    SDIO_Turnto_Input();  
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    //mode
    temp=0;
    for(i=0;i<4;i++)
    {
//        temp<<=1;
        OTP_SCK_0;
        Delay_1us(BIT_DELAY_1US);
        temp|=READ_SDIO_DATA;  
        temp=temp<<1;
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
    }
    mode_temp=temp;
    //status
    temp=0;
//    for(i=0;i<2;i++)
//    {
        temp<<=1;
        OTP_SCK_0;
        Delay_1us(BIT_DELAY_1US);
        temp|=READ_SDIO_DATA;        
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
//    }        
//    status_temp=temp;

}


u16 MTP_Read_Word(u8 type_data)
{
    u16 data_temp;

    MTP_Start();
    MTP_ACK();
    MTP_TYPE_RW(type_data,1); //read data    
    data_temp=ReadWord();
    MTP_NACK();
    MTP_STOP();
    return data_temp;
}

void MTP_Write_Word_16Byte(u16 addr_start,u16 *data_buf)
{
    u8 i;
    MTP_Start();
    MTP_ACK();
    MTP_TYPE_RW(RW_TYPE_DATA,0); //write
    for(i=0;i<15;i++)
    {
        SendWord(data_buf[i]);
        MTP_ACK();
    }
    SendWord(data_buf[15]);
    MTP_NACK();
    MTP_STOP();   
}

//========erase & prog=============================
void SendWord_Erase(u8 Terase0,u16 Terase1,u8 Terase2)
{
    u8 i;
    for(i=0;i<3;i++)
    {
        OTP_SCK_0;  //下降沿改变数据，上升沿采样
        OTP_SDIO_0;
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_1;
        OTP_SCK_1;
    }
    //clk 3
    OTP_SCK_0;  //下降沿改变数据，上升沿采样
    Delay_1us(Terase0);
    OTP_SCK_1;
    Delay_1us(Terase0);
    for(i=4;i<6;i++)
    {
        OTP_SCK_0;  //下降沿改变数据，上升沿采样
        OTP_SDIO_0;
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_1;
        OTP_SCK_1;
    }
    for(i=6;i<10;i++)
    {
        OTP_SCK_0;  //下降沿改变数据，上升沿采样
        Delay_1us(Terase1);
        OTP_SCK_1;
        Delay_1us(Terase1);
    } 
     for(i=10;i<12;i++)
    {
        OTP_SCK_0;  //下降沿改变数据，上升沿采样
        OTP_SDIO_0;
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_1;
        OTP_SCK_1;
    } 
     //clk 12
    OTP_SCK_0;  //下降沿改变数据，上升沿采样
    Delay_1us(Terase2);
    OTP_SCK_1; 
    Delay_1us(Terase2);
    OTP_SCK_0;  //下降沿改变数据，上升沿采样
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);

      //clk 14
    OTP_SCK_0;  //下降沿改变数据，上升沿采样
    Delay_1us(Terase0);
    OTP_SCK_1;
    Delay_1us(Terase0);  

    OTP_SCK_0;  //下降沿改变数据，上升沿采样
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_0;
}

//烧写和编程时序一样，只是时间有所调整
void MTP_Erase_Prog(u8 delay0,u16 delay1,u8 delay2)
{
    u8 mode_temp,status_temp,crc_temp;

    u8 i,temp,type_data;

    SDIO_Turnto_Output();
    //starte
    OTP_SCK_0;
    OTP_SDIO_1;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    //Trn
    SDIO_Turnto_Input();  
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);

    //mode
    temp=0;
    for(i=0;i<4;i++)
    {
        temp<<=1;
        OTP_SCK_0;
        Delay_1us(BIT_DELAY_1US);
        temp|=READ_SDIO_DATA;        
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
    }
    mode_temp=temp;
    //status
    temp=0;
    for(i=0;i<2;i++)
    {
        temp<<=1;
        OTP_SCK_0;
        Delay_1us(BIT_DELAY_1US);
        temp|=READ_SDIO_DATA;        
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
    }        
    status_temp=temp;
    //Trn
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    SDIO_Turnto_Output();  
    Delay_1us(BIT_DELAY_1US);
    //nACK
    OTP_SDIO_0;
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    //Trn2
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);        
    //type[1:0]
    type_data=RW_TYPE_STATUS;

    for(i=0;i<2;i++)
    {
        OTP_SCK_0;
        Delay_1us(BIT_DELAY_1US);
        if(type_data & 0x02)
            OTP_SDIO_1;
        else
            OTP_SDIO_0;
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
        type_data <<=1;
    }

    //R/W
    OTP_SDIO_0;     //L write
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    //Trn3
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);

    SendWord_Erase(delay0,delay1,delay2);

    //Trn4
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    //ACK
    OTP_SDIO_1;
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    //Trn5
    SDIO_Turnto_Input();
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);

    temp=0;
    for(i=0;i<4;i++)
    {
        OTP_SCK_0;
        Delay_1us(BIT_DELAY_1US);
        temp|=READ_SDIO_DATA;
        temp<<=i;
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
    }        
    crc_temp=temp;   

    //Trn6
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    SDIO_Turnto_Output();
    Delay_1us(BIT_DELAY_1US);

    //stop
    OTP_SDIO_0;
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
}
//==============================================================================

//panqian新增20230720
//Name: MTP_Start_mode()
//Description:  Start_mode +Trn0 +mode+status 
u8 MTP_Start_mode()
{
    u8 mode_temp,status_temp;

    u8 i,temp;

    SDIO_Turnto_Output();
    //starte
    OTP_SCK_0;
    OTP_SDIO_1;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;

    //Trn
    SDIO_Turnto_Input();  
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    //mode
    OTP_SCK_0;  //add 
    temp=0;
    for(i=0;i<4;i++)
    {
        temp<<=1;
        
        Delay_1us(BIT_DELAY_1US);
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
        OTP_SCK_0;
        
        temp|=READ_SDIO_DATA;  
        
//        temp<<=1;
//        Delay_1us(BIT_DELAY_1US);
//        OTP_SCK_1;
//        Delay_1us(BIT_DELAY_1US);
//        OTP_SCK_0;
//        temp|=READ_SDIO_DATA; 
    }
    mode_temp=temp;
    //status
    temp=0;
    for(i=0;i<2;i++)
    {
        temp<<=1;
        OTP_SCK_0;
        Delay_1us(BIT_DELAY_1US);
        temp|=READ_SDIO_DATA;        
        OTP_SCK_1;
        Delay_1us(BIT_DELAY_1US);
    }        
    status_temp=temp;
    
    return mode_temp;

}

u8 MTP_Read_Word_mode(u8 type_data)
{
    u8 data_temp;

    data_temp=MTP_Start_mode();
    MTP_ACK();
    MTP_TYPE_RW(type_data,1); //read data    
    ReadWord();
    MTP_NACK();
    MTP_STOP();
    return data_temp;
}
