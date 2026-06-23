/*
 * @Copyright: Shanghai Sinomcu Microelectronics Co.,Ltd.
 * @Author: Mike.Mo
 * @Emain: mgf@sinomcu.com
 * @Date: 2019-06-25 11:07:18
 * @Encoding: GB2312
 * @Description: 
 */

#include "stm32f10x.h"
#include "global.h"
#include "spi.h"
#include "OTPRW.h"
#include "power.h"
#include "IRC.h"
#include "Config.h"
#include "ADC.h"
#include "erorrNum.h"
#include "delay.h"
#include    "mc30f6910.h"
#include    "MTPRW.h"
#include "MC32P5232Pro.h"

struct
{
    u16 nowAddr;        //rom 当前地址 word
    u16 firstAddr;      //rom 起始地址 全部使用half word
    u16 endAddr;        //rom 结束地址 全部使用half word
    u16 size;           //rom 大小     全部使用half word
    u16 rdata;          //rom 读取的数据
    u16 wdata;          //rom 待写数据
    u8  wdataH;         //rom 待写数据高8位
    u8  wdataL;         //rom 待写数据低8位
}ROM;   
struct
{
    u8  ROLLENABLE;     //滚码开关
    u8  LIMTENABLE;     //烧录次数开关
    u16 BSIZE;
    u16 ADDR0;
    u16 ADDR1;
    u16 ADDR2;
    u16 ADDR3;
    u32 max_value;
    u8  DATA0;
    u8  DATA1;
    u8  DATA2;
    u8  DATA3;

    u32 current_value;
    u32 OKMAX;
}roll; 
struct 
{
    u16 addrshift;      //不同的页对应的地址偏移

    u16 size;           //待修复点的个数 
    u16 data[4];        
    u16 addr[4];
    u16 dotaddrremap[4];  //修复地址所在的地址
    

    u16 sizeMargin1;        //margin1
    u16 dataMargin1[2];     //margin1
    u16 addrMargin1[2];

    u16 sizeOffMargin;        //off margin
    u16 dataOffMargin[2];     //off margin
    u16 addrOffMargin[2];  

    u16 dotaddrremap1;  //修复地址所在的地址
    u16 dotaddrremap2;  //修复地址所在的地址

    u16 dotremap1;      //修复数据烧写的地址
    u16 dotremap2;      //修复数据烧写的地址


    u8  remain;         //剩余能修复的个数
    u8  flag;           //临时的标识


}badDot;

u8 mc30f6910_program()
{
    u8  ReCnt=0,op_addr=0;
    u16  Fdata,ROMReadData,temp;
    u16  data_buf[16];
    u16  bad_point[10];
    u8	FdataL,FdataH;
    u8  ReFlag=0;
    u16 RomCrcResult=0,RomCrcResultOpt = 0;//2025.12.31
    u16 uoption_backup[16];

    u16   i,j;
    u16 CheckSum,crc_calculator;

    u8     id_length=0;
    u16    id_addr0,id_addr1,id_addr2,id_addr3,mcu_addr;
    u32    id_end,current_id,max_id;

//    struct
//    {
//        u16 addr;
//
//        u16 loop1;         //循环计数器，第一层
//        u16 loop2;         //循环计数器，第二层
//        u16 loop3;         //循环计数器，第三层
//        u16 loop4;         //循环计数器，第四层    
//    }Cnt;     
//
//   

    struct
    {
        u16 temp;           //临时存储区          
        u16 result;         //计算结果值
        u16 hardware;       //硬件结果
        u16 flag;           //标识
        u16 OTPsize;

    }Crc;  


    //reserve baddot fixed function
    // for(i=0;i<MCU_Config_xx.BadDotConfig_xx.BadDotNum;i++)
    // {
    //   badDot.addr[i]=0xffff;
    // }
    // badDot.size = 0;

    //load rom or eeprom program flag
    FMReadOne(Addr_Flash_ROM_OR_EEPROM);
    ROM_OR_EEPROM=Rxdata;
    
    //------------------------------进模式----------------------------       
     if( MTP_MODE_IN() == 0)
     {
          Delay_1ms(300);//给机台busy信号时间不能太短
          ERORR_VALUE=OTP_ModeIn_false;
          return 0;       
     }    

//===================== check product id =======================================
    //get in prog mode
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
    MTP_Write_Word(MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H_addr,RW_TYPE_ADDR);            
    ROMReadData=MTP_Read_Word(RW_TYPE_DATA);    //read product id
    
    if(((ROMReadData & 0x00ff)==MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H) || ((ROMReadData & 0x00ff)==MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L))
    {
    }
    else
    {
        ERORR_VALUE=OTP_ModeIn_false;
        return 0;
    }

    MTP_Write_Word(MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_addr,RW_TYPE_ADDR);           
    ROMReadData=MTP_Read_Word(RW_TYPE_DATA);    //read product id
    
    if((ROMReadData & 0x0f00)!=MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_version)
    {
        ERORR_VALUE=OTP_ModeIn_false;
        return 0;
    }  
    
    // MTP_Write_Word(0x802C,RW_TYPE_ADDR);            
    // ROMReadData=MTP_Read_Word(RW_TYPE_DATA);    //read CP信息  
    // if( ROMReadData != 0x0000 )
    // {
    // }
    // else
    // {
    //     ERORR_VALUE=OTP_ModeIn_false;
    //     return 0;
    // }
    
    MTP_Write_Word(0x802D,RW_TYPE_ADDR);            
    ROMReadData=MTP_Read_Word(RW_TYPE_DATA);    //read FT信息  
    if( ROMReadData != 0x0000 )
    {
    }
    else
    {
        ERORR_VALUE=OTP_ModeIn_false;
        return 0;
    }
    
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    //退出prog模式
     
//================EEPROM 烧写=============================================
    //ROM_OR_EEPROM bit1==1
    if(ROM_OR_EEPROM & 0x02)
    {
        if(erase_eeprom()==0)
        {
            Delay_1ms(100);//给机台busy信号时间不能太短
            ERORR_VALUE=EEPROM_Write_ERROR; //擦除EEPROM数据出错
            return 0;           
        }
        
        if(write_eeprom()==0)
        {
            Delay_1ms(100);//给机台busy信号时间不能太短
            ERORR_VALUE=EEPROM_Write_ERROR; //擦除EEPROM数据出错
            return 0;            
        }

    }

    if(ROM_OR_EEPROM & 0x01)    //rom program
    {      
//==================手动加载坏点寄存器中数据到映射寄存器中==========省略=====
//================= pq 23.2.9 烧录前backup uoption==============
      
        //get in program
        MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);  
        
        MTP_Write_Word(0X8010,RW_TYPE_ADDR);
        for(i=0;i<16;i++)
            uoption_backup[i]=MTP_Read_Word(RW_TYPE_DATA);    //bukup uoptbit data 
        
        //exit program
        MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);
   
//===============Eraser Flash & EEPROM ================
        if(erase_chip()==0)
        {               
            Delay_1ms(100);//给机台busy信号时间不能太短
            ERORR_VALUE=FLASH_ERASE_ERROR;
            return 0;    
        }
        
        
//================================IRC 校准 ==================================
        //====HIRC校准===============
        if(MCU_Config_xx.IRC_CONFIG_xx.IRC_Select==0x01)
        {
            if(hirc_adj(MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMin,MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMax,MCU_Config_xx.IRC_CONFIG_xx.IRC_FrType)==0)
            {
              Delay_1ms(100);//给机台busy信号时间不能太短
              ERORR_VALUE=IRC_Value_false; 
              return 0;             
            }             
        }          
        
        //====PFRC校准================
        if(MCU_Config_xx.IRC_CONFIG_xx.IRC_Select==0x01)
        {
            if(PFRC_adj(MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMin-5,MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMax+5,MCU_Config_xx.IRC_CONFIG_xx.IRC_FrType)==0)
            {
              Delay_1ms(100);//给机台busy信号时间不能太短
              ERORR_VALUE=IRC_Value_false; 
              return 0;             
            }             
        }          
                                      
        //=================填写频率校准值=========================================
        if(MCU_Config_xx.WriteConfig_xx.PassEnginTest==0)
        {      
                                             
            // fill buf
            for(i=0;i<16;i++)
                data_buf[i]=0;  //clera buf
            
            //get in prog mode
            MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
            
            MTP_Write_Word(0X8020,RW_TYPE_ADDR);
            for(i=0;i<16;i++)
                data_buf[i]=MTP_Read_Word(RW_TYPE_DATA);    //bukup doptbit data            
            
            //doptbit
            data_buf[1] =OSCCAL; //bit[15:8]=0 
            data_buf[2] =PFRC_OSCCAL; //bit[15:8]=0
            
            data_buf[1] &=0x00ff;       //bit[15:8]=0
            data_buf[2] &=0x00ff;       //bit[15:8]=0

            
            Crc.result=0xffff;
            for(i=1;i<11;i++)
            {
                FdataL=(u8)data_buf[i];
                FdataH=data_buf[i]>>8;
                Crc.result=CountCRC(FdataH,FdataL,Crc.result);
            }
            data_buf[0]=Crc.result;
            
            //erase 0x8020 0x8021 0x8022
            for(i=0;i<3;i++)
            {
              MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
              MTP_Write_Word(0x0000,RW_TYPE_STATUS);    
              MTP_Write_Word(0X8020+i,RW_TYPE_ADDR);    
              MTP_Write_Word(0X0000,RW_TYPE_DATA);
              MTP_Write_Word(PROG_CMD_ERASE,RW_TYPE_CTL);
              MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2); 
            }            
            
            //write 0x8020 0x8021 0x8022
            for(i=0;i<3;i++)
            {
              MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
              MTP_Write_Word(0x0000,RW_TYPE_STATUS);    
              MTP_Write_Word(0X8020+i,RW_TYPE_ADDR);    
              MTP_Write_Word(data_buf[i],RW_TYPE_DATA);
              MTP_Write_Word(PROG_CMD_PROG,RW_TYPE_CTL);
              MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2); 
            }  

            //verify   
            MTP_Write_Word(0x8020,RW_TYPE_ADDR);
            for(i=0;i<16;i++)
            {
                temp=MTP_Read_Word(RW_TYPE_DATA);
                if(temp !=data_buf[i])
                {
                    ERORR_VALUE=OPTION_Write_false;
                    return 0;
                }
            }
         MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    //退出prog模式   
        } 
        
 
       
//----------------------------------------------------------------------         
                      
     Delay_1ms(2);      

      
     //修改加密位为不加密
     MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);  
     
     //erase 0x8011
     MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
     MTP_Write_Word(0x0000,RW_TYPE_STATUS);    
     MTP_Write_Word(0X8011,RW_TYPE_ADDR);    
     MTP_Write_Word(0X0000,RW_TYPE_DATA);
     MTP_Write_Word(PROG_CMD_ERASE,RW_TYPE_CTL);
     MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2); 
                                    
     //write 0x8020 0x8021 0x8022
     MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
     MTP_Write_Word(0x0000,RW_TYPE_STATUS);    
     MTP_Write_Word(0X8011,RW_TYPE_ADDR);    
     MTP_Write_Word(0x0000,RW_TYPE_DATA);
     MTP_Write_Word(PROG_CMD_PROG,RW_TYPE_CTL);
     MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2);      

     MTP_Write_Word(0x9011,RW_TYPE_ADDR);
     MTP_Write_Word(0x0000,RW_TYPE_DATA);                         
                                    
     MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL); 
     
     Delay_1ms(2);
     MTP_Write_Word(RELOAD_OPTION_MODE,RW_TYPE_CTL);  //配置字重载  
     Delay_1ms(40);

    
     
    if(CheckRomCRC() == 0)
    {
          ERORR_VALUE = CRC_CODE_NOT_MATCH_ERROR;
          return 0;
    }

    //------------------------------初始化滚动码----------------------------  
        if(iic_data[0]==0xFF)
        {
            roll.ROLLENABLE   = 0x00;
            roll.LIMTENABLE   = 0x00;       
        }
        else if(iic_data[0]==0xA0)
        {
            roll.ROLLENABLE   = 0x00;
            roll.LIMTENABLE   = 0x01;        
        }
        else if(iic_data[0]==0xAA)
        {
            roll.ROLLENABLE   = 0x01;
            roll.LIMTENABLE   = 0x00;       
        }
        else if(iic_data[0]==0x0A)
        {
            roll.ROLLENABLE   = 0x01;
            roll.LIMTENABLE   = 0x01;       
        }

            roll.BSIZE    =iic_data[1];
            roll.ADDR0    =(iic_data[2]<<8)+iic_data[3];// ADDRL
            roll.ADDR1    =(iic_data[4]<<8)+iic_data[5];
            roll.ADDR2    =(iic_data[6]<<8)+iic_data[7];
            roll.ADDR3    =(iic_data[8]<<8)+iic_data[9];//ADDRH

            roll.current_value  =(iic_data[10]<<24)+(iic_data[11]<<16)+(iic_data[12]<<8)+iic_data[13];   //当前准备写入的编号
            roll.OKMAX    =(iic_data[15]<<24)+(iic_data[16]<<16)+(iic_data[17]<<8)+iic_data[18];         //烧写成功的次数 限制值
            roll.max_value  =(iic_data[27]<<24)+(iic_data[28]<<16)+(iic_data[29]<<8)+iic_data[30];       //max-id//芯片最大编号

            if (roll.current_value > roll.max_value)
            {
                iic_data[10]=iic_data[23];  //重新初始化id
                iic_data[11]=iic_data[24];
                iic_data[12]=iic_data[25];
                iic_data[13]=iic_data[26];
            }

            roll.DATA0    =iic_data[13];
            roll.DATA1    =iic_data[12];
            roll.DATA2    =iic_data[11];
            roll.DATA3    =iic_data[10];
            

        if (roll.LIMTENABLE == 0x01)          //烧写次数限制功能打开
        {
            if (OKcounter >=roll.OKMAX)   //烧写成功的次数达到限制值
            {
                ERORR_VALUE=OK_Counter_Full;
                return 0;                         // full limit
            }
        }     

        //ROM write
        if(MCU_Config_xx.WriteConfig_xx.WriteEn & 0x0f) //rom write enable
        {
            
            Crc.result = 0xffff;
            ROM.nowAddr = MCU_Config_xx.WriteConfig_xx.RomFirAddr;
            ROM.endAddr = MCU_Config_xx.WriteConfig_xx.RomEndAddr>>1;
            ROM.firstAddr = MCU_Config_xx.WriteConfig_xx.RomFirAddr;

            //get in program mode
            MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);

        // ROM.nowAddr +=1;                                    //地址+1
            for(ROM.nowAddr = MCU_Config_xx.WriteConfig_xx.RomFirAddr; ROM.nowAddr <= ROM.endAddr; )
            {
                
                
                for(i=0;i<16;i++)
                {
                    ROM.wdata = WRITE_DATA_xx.rom_word[ROM.nowAddr] ;        /*组成16bit 待烧录数据*/
                    if (roll.ROLLENABLE == 0x01)                
                    {//使能滚动码功能
                        if (ROM.nowAddr   ==roll.ADDR0)
                        {
                            ROM.wdata      =(ROM.wdata & 0xff00) + roll.DATA0; // must keep the H byte for code
                        }
                        else if ((ROM.nowAddr     ==roll.ADDR1)&& (roll.BSIZE >1 ))
                        {
                            ROM.wdata      =(ROM.wdata & 0xff00) + roll.DATA1; // must keep the H byte for code
                        }
                        else if ((ROM.nowAddr     ==roll.ADDR2)&& (roll.BSIZE>2))
                        {
                            ROM.wdata      =(ROM.wdata & 0xff00) + roll.DATA2; // must keep the H byte for code
                        }
                        else if ((ROM.nowAddr     ==roll.ADDR3)&&(roll.BSIZE>3))
                        {
                            ROM.wdata      =(ROM.wdata & 0xff00) + roll.DATA3; // must keep the H byte for code
                        }
                    }

                    ROM.wdataH = ROM.wdata>>8;
                    ROM.wdataL = ROM.wdata;
                    if(ROM.nowAddr <= ROM.endAddr)
                    {
                        Crc.result =CountCRC(ROM.wdataH , ROM.wdataL , Crc.result); 
                    }

                    data_buf[i]=ROM.wdata;

                    ROM.nowAddr ++;
                }


                MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
                MTP_Write_Word(0x0000,RW_TYPE_STATUS);

                MTP_Write_Word(ROM.nowAddr-16,RW_TYPE_ADDR);
                MTP_Write_Word_16Byte(ROM.nowAddr-16,(u16*)data_buf);

                MTP_Write_Word(PROG_CMD_PROG,RW_TYPE_CTL);
                MTP_Erase_Prog(PROG_TIME0,PROG_TIME1,PROG_TIME2);
                        
            }
            
            //=====read rom verify============
            MTP_Write_Word(0X0000,RW_TYPE_ADDR);
            for(ROM.nowAddr = MCU_Config_xx.WriteConfig_xx.RomFirAddr; ROM.nowAddr <= ROM.endAddr; )
            {
              ROM.wdata = WRITE_DATA_xx.rom_word[ROM.nowAddr] ;        /*组成16bit 待烧录数据*/
              
              if (roll.ROLLENABLE == 0x01)                
              {//使能滚动码功能
                  if (ROM.nowAddr   ==roll.ADDR0)
                  {
                      ROM.wdata      =(ROM.wdata & 0xff00) + roll.DATA0; // must keep the H byte for code
                  }
                  else if ((ROM.nowAddr     ==roll.ADDR1)&& (roll.BSIZE >1 ))
                  {
                      ROM.wdata      =(ROM.wdata & 0xff00) + roll.DATA1; // must keep the H byte for code
                  }
                  else if ((ROM.nowAddr     ==roll.ADDR2)&& (roll.BSIZE>2))
                  {
                      ROM.wdata      =(ROM.wdata & 0xff00) + roll.DATA2; // must keep the H byte for code
                  }
                  else if ((ROM.nowAddr     ==roll.ADDR3)&&(roll.BSIZE>3))
                  {
                      ROM.wdata      =(ROM.wdata & 0xff00) + roll.DATA3; // must keep the H byte for code
                  } 
              }
                            
              Fdata=MTP_Read_Word(RW_TYPE_DATA);
              ROM.nowAddr ++;
              if(ROM.wdata !=Fdata)
              {
                ERORR_VALUE=ROM_Write_false;
                return 0;
              }            
            }
            
            //--------- test rom data ------------------
            // MTP_Write_Word(0x0000,RW_TYPE_ADDR);
            // for(i=0;i<32;i++)
            //     Fdata=MTP_Read_Word(RW_TYPE_DATA);
            
            //CRC check
            //change to debug mode
//            MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);
//            
//           MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);                                           
//           MTP_Write_Word(0x9012,RW_TYPE_ADDR);
//           MTP_Write_Word(0x0006,RW_TYPE_DATA);                                          
//           MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);              
//            
//            MTP_Write_Word(DEBUG_MODE,RW_TYPE_CTL);
//            MTP_Write_Word(DEBUG_CRC,RW_TYPE_CTL);  //start crc
//            delay_ms(30);
//            temp=1;
//            Fdata=0;
//            while(temp&0x0001)
//            {
//                temp=MTP_Read_Word(RW_TYPE_STATUS);
//                Fdata++;
//                if(Fdata>3000)
//                {
//                    ERORR_VALUE=CRC_ERROR;
//                    return  0;
//                }
//            }
//            delay_ms(1);
//            MTP_Write_Word(0xfffd,RW_TYPE_ADDR);
//            Fdata=MTP_Read_Word(RW_TYPE_DATA);       
//            //exit debug mode
//            MTP_Write_Word(DEBUG_EXIT,RW_TYPE_CTL);        
//
//            delay_ms(1);
//            if(Fdata !=Crc.result)
//            {
////                ERORR_VALUE=ROM_Write_false;
////                return 0;
//            }

        } //end of rom write loop
    
        
        
        
        //写之前先擦一下user option
        erase_user_option();
        if(CheckOptionCRC() == 0)
        {
            ERORR_VALUE = CRC_OPT_NOT_MATCH_ERROR;
            return 0;
        }     
        //write opiton
        if((MCU_Config_xx.WriteConfig_xx.WriteEn & 0xf0)==0xf0)
        {
            // fill buf
            for(i=0;i<16;i++)
                data_buf[i]=0;  //clera buf

            // FMReadOne(Addr_Flash_OptionStart+0);
            // temp=Rxdata;
            // FMReadOne(Addr_Flash_OptionStart+1);
            data_buf[1]=WRITE_DATA_xx.option_word[0] ;
            data_buf[1] &=0x2777; //bit[13:8]=0

            // FMReadOne(Addr_Flash_OptionStart+2);
            // temp=Rxdata;
            // FMReadOne(Addr_Flash_OptionStart+3);
            data_buf[2]=WRITE_DATA_xx.option_word[1] ; 
            data_buf[2] &=0x00ff; //bit[13:8]=0

            //----pq 2023.2.9 回填uoption[0x8013:0x801b]
            for(i=0x03;i<0x0c;i++)
            {
              data_buf[i]=uoption_backup[i];   //回填uoption
            }
            
            
            data_buf[0x0c]=Crc.result;
            data_buf[0x0e]=FirmwareVersion[3]*0x100+FirmwareVersion[4];
            data_buf[0x0f]= (*(u32*)(0x1ffff7e8) & 0x0000ffff);

            //--------calculator-------------------
            Crc.result=0xffff;
            for(i=1;i<3;i++)
            {
                FdataL=(u8)data_buf[i];
                FdataH=data_buf[i]>>8;
                Crc.result=CountCRC(FdataH,FdataL,Crc.result);
            }
            data_buf[0]=Crc.result;

            //get in prog mode
            MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);            
            MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
            MTP_Write_Word(0x0000,RW_TYPE_STATUS);
            MTP_Write_Word(0x8010,RW_TYPE_ADDR);
            MTP_Write_Word_16Byte(0x8010,(u16*)data_buf);
            
            MTP_Write_Word(PROG_CMD_PROG,RW_TYPE_CTL);
            MTP_Erase_Prog(PROG_TIME0,PROG_TIME1,PROG_TIME2);

            //verify   
            MTP_Write_Word(0x8010,RW_TYPE_ADDR);
            for(i=0;i<16;i++)
            {
                temp=MTP_Read_Word(RW_TYPE_DATA);

                  if(temp !=data_buf[i])
                  {
                    ERORR_VALUE=OPTION_Write_false;
                    return 0;
                  }
            }
            
            MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);  

        }//end of option write            
        
        
        Delay_1ms(2);         
        
     //=============频率check=========================  
     if(hirc_adj_check(MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMin,MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMax,MCU_Config_xx.IRC_CONFIG_xx.IRC_FrType)==0)
     {
        Delay_1ms(100);//给机台busy信号时间不能太短
        ERORR_VALUE=IRC_Value_false; 
        return 0;             
     } 
     
     //=============频率check=========================  
     if(PFRC_adj_check(MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMin-5,MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMax+5,MCU_Config_xx.IRC_CONFIG_xx.IRC_FrType)==0)
     {
        Delay_1ms(100);//给机台busy信号时间不能太短
        ERORR_VALUE=IRC_Value_false; 
        return 0;             
     }      
                      
    }
    
      //退出wait模式
      MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);  
    //check system option & designed option
    //0x8000-0x8005 0x8006-0x801f,0x8020-0x8029    
    
    Delay_1ms(2);

    VDD30V_Off;
    POWER_OFF(vpp00,vdd00);
    return 1;
    
}

//================================
// 1．	设置编程控制寄存器选择清HVPL模式，写编程状态寄存器
// 2．	设置擦除地址
// 3．	设置编程控制寄存器选择擦除模式（片擦/块擦/页擦）
// 4．	写编程状态寄存器控制擦除时序
//
u8 erase_chip()
{
    u16 temp_wait;
    u16 temp_status;
    u8  retry_count;
    u8 i;
    u16  data_buf[16];
    u16 Addr;

  
    
     //修改系统时钟，设置为8M
     MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);                  

     MTP_Write_Word(0x9011,RW_TYPE_ADDR);
     MTP_Write_Word(0x0000,RW_TYPE_DATA);        
            
     MTP_Write_Word(0x9012,RW_TYPE_ADDR);
     MTP_Write_Word(0x0006,RW_TYPE_DATA); 
     
     MTP_Write_Word(0x9021,RW_TYPE_ADDR);
     MTP_Write_Word(0x0080,RW_TYPE_DATA);  
     
     MTP_Write_Word(0x9023,RW_TYPE_ADDR);
     MTP_Write_Word(0x0018,RW_TYPE_DATA);      
     
     MTP_Write_Word(0x9024,RW_TYPE_ADDR);
     MTP_Write_Word(0x003F,RW_TYPE_DATA);      
                                    
     MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);     
    
    
    //-----------------加密擦除 ------------------------------
     
    retry_count=0;
    do{      
    
    // fill buf
    for(i=0;i<16;i++)
        data_buf[i]=0;  //clera buf     
                                 
        //get in prog mode  //erase rom
        MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);         
        MTP_Write_Word(CHIP_SELECT|PROG_CMD_CLRPL,RW_TYPE_CTL);
        MTP_Write_Word(0x0000,RW_TYPE_STATUS);
        MTP_Write_Word(0x0000,RW_TYPE_ADDR); 
        MTP_Write_Word(CHIP_SELECT|PROG_CMD_ERASE,RW_TYPE_CTL);
        MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2); 

        //rom write 0
        MTP_Write_Word(CHIP_SELECT|PROG_CMD_CLRPL,RW_TYPE_CTL);
        MTP_Write_Word(0x0000,RW_TYPE_STATUS);
        MTP_Write_Word(0x0000,RW_TYPE_ADDR);
        MTP_Write_Word_16Byte(0x0000,(u16*)data_buf);
        MTP_Write_Word(CHIP_SELECT|PROG_CMD_PROG,RW_TYPE_CTL);
        MTP_Erase_Prog(PROG_TIME0,PROG_TIME1,PROG_TIME2);
        //exit 
        MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);
                
        
        Delay_1ms(1);      
      
        
        //get in debug mode
        MTP_Write_Word(DEBUG_MODE,RW_TYPE_CTL);   
        MTP_Write_Word(DEBUG_RESET,RW_TYPE_CTL);//panqian新增，按秀峰要求，在发CRC之前，插入reset命令 20230720
        MTP_Write_Word(DEBUG_CRC_ENCR,RW_TYPE_CTL);
        Delay_1ms(30);
        MTP_Write_Word(DEBUG_CRC,RW_TYPE_CTL);
        Delay_1ms(30);
        temp_status=1;                
        temp_wait=0;
        while(temp_status&0x0001)
        {
            temp_status=MTP_Read_Word(RW_TYPE_STATUS);
            temp_wait++;
            if(temp_wait>3000)
            {
                return  0;
            }

        }

        //exit debug mode
        MTP_Write_Word(DEBUG_EXIT,RW_TYPE_CTL);

        if((temp_status & 0x000C) ==0x000C)  
            break;
        retry_count++;
    }while(retry_count<3);

    if(retry_count>=3)
    {
        ERORR_VALUE=FLASH_ERASE_ERROR;
        return 0;
    }
    
//-----------------option_erase ------------------------------        
//1.用户配置区页擦除
//2.用户配置区页烧写，加密位写为0
//3.用户配置区再擦除
//4.DOPTBIT1擦除       
    
        
// fill buf
    for(i=0;i<16;i++)
        data_buf[i]=0;  //clera buf       
        
    //get in program
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);              
    
    //erase 0x8010
    MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
    MTP_Write_Word(0x0000,RW_TYPE_STATUS);    
    MTP_Write_Word(0X8010,RW_TYPE_ADDR);   
    MTP_Write_Word_16Byte(0x8010,(u16*)data_buf); 
    MTP_Write_Word(PROG_CMD_ERASE,RW_TYPE_CTL);
    MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2);
     
    //write 0x8010
    MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
    MTP_Write_Word(0x0000,RW_TYPE_STATUS);
    MTP_Write_Word(0x8010,RW_TYPE_ADDR);
    MTP_Write_Word_16Byte(0x8010,(u16*)data_buf);            
    MTP_Write_Word(PROG_CMD_PROG,RW_TYPE_CTL);
    MTP_Erase_Prog(PROG_TIME0,PROG_TIME1,PROG_TIME2);
    //exit program
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);
    
    //-------------擦除---------------------------------------

              
        //get in prog mode  //erase rom
        MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);         
        MTP_Write_Word(CHIP_SELECT|PROG_CMD_CLRPL,RW_TYPE_CTL);
        MTP_Write_Word(0x0000,RW_TYPE_STATUS);
        MTP_Write_Word(0x0000,RW_TYPE_ADDR); 
        MTP_Write_Word(CHIP_SELECT|PROG_CMD_ERASE,RW_TYPE_CTL);
        MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2);           
    
        //erase 0x8010
        MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
        MTP_Write_Word(0x0000,RW_TYPE_STATUS);    
        MTP_Write_Word(0X8010,RW_TYPE_ADDR);   
        MTP_Write_Word_16Byte(0x8010,(u16*)data_buf); 
        MTP_Write_Word(PROG_CMD_ERASE,RW_TYPE_CTL);
        MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2);
     
        //exit program
        MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);

}


//================================
// 1．	设置编程控制寄存器选择清HVPL模式，写编程状态寄存器
// 2．	设置擦除地址
// 3．	设置编程控制寄存器选择擦除模式（片擦/块擦/页擦）
// 4．	写编程状态寄存器控制擦除时序
//
u8 erase_eeprom()
{
    u16 i;
    u16  data_buf[16];
    u16 Addr;
    
    // fill buf
    for(i=0;i<16;i++)
        data_buf[i]=0;  //clera buf 
    
    /***************页擦 eeprom******************/
    //get in prog mode
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
    
    //get in program
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
    
    for(i=0;i<0x80;i=i+0x10)
    {
      MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
      MTP_Write_Word(0x0000,RW_TYPE_STATUS);

      MTP_Write_Word(0xc000+i,RW_TYPE_ADDR);
      MTP_Write_Word_16Byte(0xc000+i,(u16*)data_buf);
      MTP_Write_Word(PROG_CMD_ERASE,RW_TYPE_CTL);
      MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2);      
    }
    
    //exit program
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);  
    
//eeprom字擦    
//    //get in prog mode
//    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
//        
//    for(i=0;i<0x80;i++)  
//    {
//        MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
//        MTP_Write_Word(0x0000,RW_TYPE_STATUS);
//
//        MTP_Write_Word(0xc000+i,RW_TYPE_ADDR);        
//        MTP_Write_Word(0x0000,RW_TYPE_DATA);
//        MTP_Write_Word(PROG_CMD_ERASE,RW_TYPE_CTL);
//        MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2);               
//    }  
//    
//    //exit 
//    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);  
    
  
//eeprom原片擦   error 
//   //get in program
//    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
//    
//
//    MTP_Write_Word(CHIP_SELECT|PROG_CMD_CLRPL,RW_TYPE_CTL);
//    MTP_Write_Word(0x0000,RW_TYPE_STATUS);
//
//    MTP_Write_Word(0xc000,RW_TYPE_ADDR);
//    
//    MTP_Write_Word(CHIP_SELECT|PROG_CMD_ERASE,RW_TYPE_CTL);
//    MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2);
//    
//    //exit program
//    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    
}

u8 erase_user_option()
{
    u16 temp_wait;
    u16 temp_status;
    u8  retry_count;
    u8 i;
    u16  data_buf[16];
    u16 Addr;
    
    
        // fill buf
    for(i=0;i<16;i++)
        data_buf[i]=0;  //clera buf      
    //---------------option擦除----------------------------------    
    //get in program
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);   
    
    //erase 0x8010
    MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
    MTP_Write_Word(0x0000,RW_TYPE_STATUS);    
    MTP_Write_Word(0X8010,RW_TYPE_ADDR);   
    MTP_Write_Word_16Byte(0x8010,(u16*)data_buf); 
    MTP_Write_Word(PROG_CMD_ERASE,RW_TYPE_CTL);
    MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2);
    //exit program
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);

}

u8 erase_rom()
{
    u16 temp_wait;
    u16 temp_status;
    u8  retry_count;
    u8 i;
    u16  data_buf[16];
    u16 Addr;

  
    
    //-----------------只擦除rom ------------------------------
        
    // fill buf
    for(i=0;i<16;i++)
        data_buf[i]=0;  //clera buf     
             
        //get in prog mode
        MTP_Write_Word(PROG_MODE,RW_TYPE_CTL); 
        
        //MTP_Write_Word(0x0000,RW_TYPE_ADDR);
        MTP_Write_Word(CHIP_SELECT|PROG_CMD_CLRPL,RW_TYPE_CTL);
        MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
        MTP_Write_Word(0x0000,RW_TYPE_STATUS);              
         for(Addr = MCU_Config_xx.WriteConfig_xx.RomFirAddr; Addr <= 0x7FF;)
         {
          Addr=Addr+16;
          MTP_Write_Word(Addr-16,RW_TYPE_ADDR); 
          MTP_Write_Word_16Byte(Addr-16,(u16*)data_buf);
         }                 
        MTP_Write_Word(CHIP_SELECT|PROG_CMD_ERASE,RW_TYPE_CTL);
        MTP_Erase_Prog(PROG_TIME0,ERASE_TIME,PROG_TIME2);
        //exit 
        MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);


}




u8 write_eeprom()
{
    u16 i,temp_data,temp_couter;
    u16  data_buf[16];
    

    //get in prog mode
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
        
    for(i=0;i<0x80;i++)  
    {
        MTP_Write_Word(PROG_CMD_CLRPL,RW_TYPE_CTL);
        MTP_Write_Word(0x0000,RW_TYPE_STATUS);
        
        FMReadOne(Addr_Flash_MTPStart+(i*2));                    
        ROM.wdataL = Rxdata;        /*组成16bit 待烧录数据*/                        
        FMReadOne(Addr_Flash_MTPStart+((i*2)+1));                    
        ROM.wdataH = Rxdata;                    
        ROM.wdata = ROM.wdataH;
        ROM.wdata = ROM.wdata<<8;                    
        ROM.wdata = ROM.wdata+ROM.wdataL;

        MTP_Write_Word(0xc000+i,RW_TYPE_ADDR);        
        MTP_Write_Word(ROM.wdata,RW_TYPE_DATA);
        MTP_Write_Word(PROG_CMD_PROG,RW_TYPE_CTL);
        MTP_Erase_Prog(PROG_TIME0,PROG_TIME1,PROG_TIME2);       
        
    }   
    
    //verify    
    MTP_Write_Word(0xc000,RW_TYPE_ADDR);
    for(i=0;i<0x80;i++)
    {
       temp_data= MTP_Read_Word(RW_TYPE_DATA); 
       
        FMReadOne(Addr_Flash_MTPStart+(i*2));                    
        ROM.wdataL = Rxdata;        /*组成16bit 待烧录数据*/                        
        FMReadOne(Addr_Flash_MTPStart+((i*2)+1));                    
        ROM.wdataH = Rxdata;                    
        ROM.wdata = ROM.wdataH;
        ROM.wdata = ROM.wdata<<8;                    
        ROM.wdata = ROM.wdata+ROM.wdataL;  
        
        if(ROM.wdata!=temp_data)
        {
         ERORR_VALUE=EEPROM_Write_ERROR;
         return 0;
        }       
    }    
        
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);
     
}


// //--------------------------------------------------
 u8 hirc_adj(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
 {
//     u8 i;
    u16 i,j=0;
    u16 osccal_mid,osccal;
    u16 temp0_FT,temp1_FT,dif0,dif1;
    u16 data_buf_temp[32];

    //get in prog mode
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
            
    MTP_Write_Word(0X8020,RW_TYPE_ADDR);
       for(i=0;i<16;i++)
          data_buf_temp[i]=MTP_Read_Word(RW_TYPE_DATA);    //bukup doptbit data     
    
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);  
    
    
     MTP_Write_Word(TEST_MODE,RW_TYPE_CTL);         
     MTP_Write_Word(0xFFFC,RW_TYPE_ADDR);
     
     for(i=0;i<4;i++)
         MTP_Write_Word(0x0000,RW_TYPE_DATA); //3 NOP
     
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);      
        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x01 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);              
     
     //write reg-option
     for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
     {
         //处理固定校准条件相关值 关闭看门狗；关闭外部复位；振荡器模式OSCM选择内部高频HIRC振荡器； 
         MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i] &= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
         TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
         TestModeRegisterValue_H_L[i] |= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i];
        
         //取上位机取界面配置的IRC校准相关的option值 (内部RC振荡器频率选择位FAS(16M/8M/4M/2M/1M/455K);VDSEL
         //TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
         //TestModeRegisterValue_H_L[i] |= WRITE_DATA_xx.option_byte_H_L[i] & MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];

                       
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0XFF00)>>8) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0X00FF) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);       
     } 
     
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x26 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (data_buf_temp[4] & 0x00ff),RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA); 
     
     for(i=0;i<13;i++)
     {
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (0x27+i) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA); 
     }
          
        
     
       //设置高频输出
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF),RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + MCU_Config_xx.IRC_CONFIG_xx.TMODE_value ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
     
       
        MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
        MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
//        TEST_STATUS_dig_signal(TEST_MODE,0x02);
          
          
        for(i=0;i<3;i++)
        MTP_Write_Word(0x0000,RW_TYPE_DATA); //3 NOP

//     MTP_Write_Word(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE);                     /*无效运行指令*/
//     MTP_Write_Word(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE);  

     osccal_mid=(MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit+1)/2;   
    
     for(i=0;i<MCU_Config_xx.IRC_CONFIG_xx.TadjNum;i++)
     {   
         osccal=osccal_mid;
         for(j=0;j<MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit_num;j++)   
         {
//          MTP_Write_Word(MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID_addr,RW_TYPE_ADDR);
//          osccal_mid=MTP_Read_Word(RW_TYPE_DATA);            
           
             MC32P5232FT(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);
             if (IRC_VALUE == IRC_FreqType)
             {
                 break;
             }
             if (IRC_VALUE<IRC_FreqType)
             {
                 osccal |=osccal_mid >>j;
             } 
             else
             {
                 osccal &=~(osccal_mid>>j);
             }
             osccal |=(osccal_mid>>1)>>j;
         } 

         MC32P5232FT(osccal+1,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);
         temp1_FT=IRC_VALUE;
         ; 
         MC32P5232FT(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);
         temp0_FT=IRC_VALUE;

         if(temp1_FT>IRC_FreqType)
         {
             dif1=temp1_FT-IRC_FreqType;
         }
         else
         {
             dif1=IRC_FreqType-temp1_FT;
         }

         if(temp0_FT>IRC_FreqType)
         {
             dif0=temp0_FT-IRC_FreqType;
         }
         else
         {
             dif0=IRC_FreqType-temp0_FT;
         }    

         if(dif1<dif0)
         {
             osccal=osccal+1;
             IRC_VALUE=temp1_FT;
         }

         if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
         {
             OSCCAL=osccal;
             TADJ=MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i];
             MC32P5232FT(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);//通过调用这个函数，可以处理、记录 频率校准值 和 温度校准值 
             
             //--------------按秀峰要求退出时要清测试模式寄存器，还得写一条无效，不然退出模式后芯片状态会乱 zqq 2020.11.30-------------
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF),RW_TYPE_DATA); 
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);              
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
             
             MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
             
             MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    //退出测试模式
             
             if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
             {
                 return 1;
             }
         }     
     }
     //panqian新增20230720
     //--------------按秀峰要求退出时要清测试模式寄存器，还得写一条无效，不然退出模式后芯片状态会乱 zqq 2020.11.30-------------
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF),RW_TYPE_DATA); 
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);              
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
     
     MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
     
     MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    //退出测试模式
     //------------------------------------------------------------------------------------------------------------------------
     
     OSCCAL=MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit;
     TADJ=MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit;
     return 0;

 }

 u8 hirc_adj_check(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
 {
    u16 i,j=0;
    u16 osccal_mid,osccal;
    u16 cnt[3];
    u16 temp0_FT,temp1_FT,dif0,dif1;
    u16 data_buf_temp[32];

    //get in prog mode
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
            
    MTP_Write_Word(0X8020,RW_TYPE_ADDR);
       for(i=0;i<16;i++)
          data_buf_temp[i]=MTP_Read_Word(RW_TYPE_DATA);    //bukup doptbit data     
    
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL); 
    
    
     MTP_Write_Word(TEST_MODE,RW_TYPE_CTL);         
     MTP_Write_Word(0xFFFC,RW_TYPE_ADDR);
     
     for(i=0;i<4;i++)
         MTP_Write_Word(0x0000,RW_TYPE_DATA); //3 NOP
     
     
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);      
        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x01 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);      
     
          //write reg-option
     for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
     {
         //处理固定校准条件相关值 关闭看门狗；关闭外部复位；振荡器模式OSCM选择内部高频HIRC振荡器； 
         MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i] &= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
         TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
         TestModeRegisterValue_H_L[i] |= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i];
        
         //取上位机取界面配置的IRC校准相关的option值 (内部RC振荡器频率选择位FAS(16M/8M/4M/2M/1M/455K);VDSEL
         //TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
         //TestModeRegisterValue_H_L[i] |= WRITE_DATA_xx.option_byte_H_L[i] & MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
                       
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0XFF00)>>8) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0X00FF) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
        
     }   
     
     
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x26 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (data_buf_temp[4] & 0x00ff),RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA); 
     
       for(i=0;i<13;i++)
       {
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (0x27+i) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA); 
       }     
     
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF),RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + MCU_Config_xx.IRC_CONFIG_xx.TMODE_value ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
       
       
      MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP     /*无效运行指令*/
      MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
   
      TEST_STATUS_dig_signal();
        
      SDIO_Turnto_Input();  //PDT设置成输入
      //----------------------------
      Delay_1ms(5);
      for (i=0;i<3;i++)
      {
        TIM_SetCounter(TIM3,0);
        TIM_SetCounter(TIM4,0);
        TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
        TIM_Cmd(TIM4,ENABLE);
        TIM_Cmd(TIM3,ENABLE);
        while(TIM_GetITStatus(TIM3,TIM_IT_Update) == RESET) ;
        cnt[i] = TIM_GetCounter(TIM4);
      }
      IRC_VALUE= (u16)((cnt[0]+cnt[1]+cnt[2])/3 );
    
      SDIO_Turnto_Output();   //PDT设置成输出
      Delay_1ms(1);
    
      OTP_SCK_0;
      Delay_1us(BIT_DELAY_1US);
      Delay_1us(BIT_DELAY_1US);    
      OTP_SCK_1;
      Delay_1us(BIT_DELAY_1US);
      Delay_1us(BIT_DELAY_1US);
    
      MTP_ACK();
      MTP_TYPE_RW(0x00,0); //write
      SendWord(0x00);

      MTP_NACK();
      MTP_STOP();
       
     
      //--------------按秀峰要求退出时要清测试模式寄存器，还得写一条无效，不然退出模式后芯片状态会乱 zqq 2020.11.30-------------
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF),RW_TYPE_DATA); 
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);              
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
             
      MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
             
      MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    //退出测试模式
   
      if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
      {
         return 1;
      }
      else
      {
         return 0;
      }
   
 }


// //--------------------------------------------------
 u8 PFRC_adj(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
 {
//     u8 i;
    u16 i,j=0;
    u16 osccal_mid,osccal;
    u16 temp0_FT,temp1_FT,dif0,dif1;
    u16 data_buf_temp[32];

     MTP_Write_Word(PROG_MODE,RW_TYPE_CTL); 
     //使能PFRC
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((PFRCCR & 0XFF00)>>8) ,RW_TYPE_DATA);
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (PFRCCR & 0X00FF) ,RW_TYPE_DATA);
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0X80,RW_TYPE_DATA);
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);   
     
     MTP_Write_Word(0X8020,RW_TYPE_ADDR);
       for(i=0;i<16;i++)
          data_buf_temp[i]=MTP_Read_Word(RW_TYPE_DATA);    //bukup doptbit data       
     
     MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);  //退出编程模式    
    
     Delay_1ms(2);
    
     MTP_Write_Word(TEST_MODE,RW_TYPE_CTL);         
     MTP_Write_Word(0xFFFC,RW_TYPE_ADDR);
     
     for(i=0;i<4;i++)
         MTP_Write_Word(0x0000,RW_TYPE_DATA); //3 NOP
     
          
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);      
        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x01 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);      
//     MTP_Write_Word(MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID_addr,RW_TYPE_ADDR);
//     MTP_Read_Word(RW_TYPE_DATA); 
     
     //write reg-option
     for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
     {
         //处理固定校准条件相关值 关闭看门狗；关闭外部复位；振荡器模式OSCM选择内部高频HIRC振荡器； 
         MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i] &= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
         TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
         TestModeRegisterValue_H_L[i] |= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i];
        
         //取上位机取界面配置的IRC校准相关的option值 (内部RC振荡器频率选择位FAS(16M/8M/4M/2M/1M/455K);VDSEL
         //TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
         //TestModeRegisterValue_H_L[i] |= WRITE_DATA_xx.option_byte_H_L[i] & MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
                       
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0XFF00)>>8) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0X00FF) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
        
     }   
     
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x26 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (data_buf_temp[4] & 0x00ff),RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA); 
     
     for(i=0;i<13;i++)
     {
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (0x27+i) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA); 
     }     
     
     
       //set TMODE
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF) ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);       
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + MCU_Config_xx.IRC_CONFIG_xx.TMODE_value ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
       
       //set TREG 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0XFF,RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0XFE ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);       
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0X40 ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);       
     
       
        MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
        MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
//        TEST_STATUS_dig_signal(TEST_MODE,0x02);
          
          
        for(i=0;i<3;i++)
        MTP_Write_Word(0x0000,RW_TYPE_DATA); //3 NOP

//     MTP_Write_Word(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE);                     /*无效运行指令*/
//     MTP_Write_Word(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE);  

     osccal_mid=(MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit+1)/2;   
    
     for(i=0;i<MCU_Config_xx.IRC_CONFIG_xx.TadjNum;i++)
     {   
         osccal=osccal_mid;
         for(j=0;j<MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit_num;j++)   
         {
             MC32P5232FT1(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);
             if (IRC_VALUE == IRC_FreqType)
             {
                 break;
             }
             if (IRC_VALUE<IRC_FreqType)
             {
                 osccal |=osccal_mid >>j;
             } 
             else
             {
                 osccal &=~(osccal_mid>>j);
             }
             osccal |=(osccal_mid>>1)>>j;
         } 

         MC32P5232FT1(osccal+1,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);
         temp1_FT=IRC_VALUE;
         ; 
         MC32P5232FT1(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);
         temp0_FT=IRC_VALUE;

         if(temp1_FT>IRC_FreqType)
         {
             dif1=temp1_FT-IRC_FreqType;
         }
         else
         {
             dif1=IRC_FreqType-temp1_FT;
         }

         if(temp0_FT>IRC_FreqType)
         {
             dif0=temp0_FT-IRC_FreqType;
         }
         else
         {
             dif0=IRC_FreqType-temp0_FT;
         }    

         if(dif1<dif0)
         {
             osccal=osccal+1;
             IRC_VALUE=temp1_FT;
         }

         if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
         {
             PFRC_OSCCAL=osccal;
             TADJ=MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i];
             MC32P5232FT1(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);//通过调用这个函数，可以处理、记录 频率校准值 和 温度校准值 
             
             //--------------按秀峰要求退出时要清测试模式寄存器，还得写一条无效，不然退出模式后芯片状态会乱 zqq 2020.11.30-------------
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF),RW_TYPE_DATA); 
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);              
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
             MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
             
             MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
             
             MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    //退出测试模式
             
             if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
             {
                 return 1;
             }
         }     
     }
     OSCCAL=MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit;
     TADJ=MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit;
     return 0;

 }

 u8 PFRC_adj_check(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
 {
    u16 i,j=0;
    u16 osccal_mid,osccal;
    u16 cnt[3];
    u16 temp0_FT,temp1_FT,dif0,dif1;

    u16 data_buf_temp[32];

     MTP_Write_Word(PROG_MODE,RW_TYPE_CTL); 
     //使能PFRC
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((PFRCCR & 0XFF00)>>8) ,RW_TYPE_DATA);
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (PFRCCR & 0X00FF) ,RW_TYPE_DATA);
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0X80,RW_TYPE_DATA);
     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);   
     
     MTP_Write_Word(0X8020,RW_TYPE_ADDR);
       for(i=0;i<16;i++)
          data_buf_temp[i]=MTP_Read_Word(RW_TYPE_DATA);    //bukup doptbit data       
     
     MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);  //退出编程模式    
    
     Delay_1ms(2);    
    
     MTP_Write_Word(TEST_MODE,RW_TYPE_CTL);         
     MTP_Write_Word(0xFFFC,RW_TYPE_ADDR);
     
     for(i=0;i<4;i++)
         MTP_Write_Word(0x0000,RW_TYPE_DATA); //3 NOP
     
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);      
        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x01 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);      
     
          //write reg-option
     for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
     {
         //处理固定校准条件相关值 关闭看门狗；关闭外部复位；振荡器模式OSCM选择内部高频HIRC振荡器； 
         MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i] &= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
         TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
         TestModeRegisterValue_H_L[i] |= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i];
        
         //取上位机取界面配置的IRC校准相关的option值 (内部RC振荡器频率选择位FAS(16M/8M/4M/2M/1M/455K);VDSEL
         //TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
         //TestModeRegisterValue_H_L[i] |= WRITE_DATA_xx.option_byte_H_L[i] & MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
                       
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0XFF00)>>8) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0X00FF) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
        
     }   
     
     
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x26 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (data_buf_temp[4] & 0x00ff),RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA); 
     
        for(i=0;i<13;i++)
        {
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0xf0 ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (0x27+i) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);        
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA); 
        }     
     
     
       //set TMODE
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF) ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);       
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + MCU_Config_xx.IRC_CONFIG_xx.TMODE_value ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
       
       //set TREG 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0XFF,RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0XFE ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);       
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0X40 ,RW_TYPE_DATA);
       MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);       
       
       
      MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP     /*无效运行指令*/
      MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
   
      TEST_STATUS_dig_signal();
        
      SDIO_Turnto_Input();  //PDT设置成输入
      //----------------------------
      Delay_1ms(5);
      for (i=0;i<3;i++)
      {
        TIM_SetCounter(TIM3,0);
        TIM_SetCounter(TIM4,0);
        TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
        TIM_Cmd(TIM4,ENABLE);
        TIM_Cmd(TIM3,ENABLE);
        while(TIM_GetITStatus(TIM3,TIM_IT_Update) == RESET) ;
        cnt[i] = TIM_GetCounter(TIM4);
      }
      IRC_VALUE= (u16)((cnt[0]+cnt[1]+cnt[2])/3 );
    
      SDIO_Turnto_Output();   //PDT设置成输出
      Delay_1ms(1);
    
      OTP_SCK_0;
      Delay_1us(BIT_DELAY_1US);
      Delay_1us(BIT_DELAY_1US);    
      OTP_SCK_1;
      Delay_1us(BIT_DELAY_1US);
      Delay_1us(BIT_DELAY_1US);
    
      MTP_ACK();
      MTP_TYPE_RW(0x00,0); //write
      SendWord(0x00);

      MTP_NACK();
      MTP_STOP();
       
     
      //--------------按秀峰要求退出时要清测试模式寄存器，还得写一条无效，不然退出模式后芯片状态会乱 zqq 2020.11.30-------------
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0XFF00)>>8),RW_TYPE_DATA); 
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA); 
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr & 0X00FF),RW_TYPE_DATA); 
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);       
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + 0x00 ,RW_TYPE_DATA);
      MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
             
      MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
             
      MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    //退出测试模式
   
      if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
      {
         return 1;
      }
      else
      {
         return 0;
      }
   
 }


 


// u8 hirc_adj()
// {
//     u8 i;

//     MTP_Write_Word(TEST_MODE,RW_TYPE_CTL);
//     for(i=0;i<3;i++)
//         MTP_Write_Word(0x0000,RW_TYPE_CTL); //3 NOP
//     //write reg-option
//     for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
//     {
//         //处理固定校准条件相关值 关闭看门狗；关闭外部复位；振荡器模式OSCM选择内部高频HIRC振荡器； 
//         MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i] &= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
//         TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
//         TestModeRegisterValue_H_L[i] |= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i];
        
//         //取上位机取界面配置的IRC校准相关的option值 (内部RC振荡器频率选择位FAS(16M/8M/4M/2M/1M/455K);VDSEL
//         //TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
//         //TestModeRegisterValue_H_L[i] |= WRITE_DATA_xx.option_byte_H_L[i] & MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
        
//         MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],funCmd_BITSIZE,0);      
//         MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i],funCmd_BITSIZE,0);               
//     }   

//     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + MCU_Config_xx.IRC_CONFIG_xx.TMODE_value ,funCmd_BITSIZE,0);      
//     MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr,funCmd_BITSIZE,0);            


//     MTP_Write_Word(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0);                     /*无效运行指令*/
//     MTP_Write_Word(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0);  

//     osccal_mid=(MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit+1)/2;   
    
//     for(i=0;i<MCU_Config_xx.IRC_CONFIG_xx.TadjNum;i++)
//     {   
//         osccal=osccal_mid;
//         for(j=0;j<MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit_num;j++)   
//         {
//             MC32P5232FT(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);
//             if (IRC_VALUE == IRC_FreqType)
//             {
//                 break;
//             }
//             if (IRC_VALUE<IRC_FreqType)
//             {
//                 osccal |=osccal_mid >>j;
//             } 
//             else
//             {
//                 osccal &=~(osccal_mid>>j);
//             }
//             osccal |=(osccal_mid>>1)>>j;
//         } 

//         MC32P5232FT(osccal+1,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);
//         temp1_FT=IRC_VALUE;
//         ; 
//         MC32P5232FT(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);
//         temp0_FT=IRC_VALUE;

//         if(temp1_FT>IRC_FreqType)
//         {
//             dif1=temp1_FT-IRC_FreqType;
//         }
//         else
//         {
//             dif1=IRC_FreqType-temp1_FT;
//         }

//         if(temp0_FT>IRC_FreqType)
//         {
//             dif0=temp0_FT-IRC_FreqType;
//         }
//         else
//         {
//             dif0=IRC_FreqType-temp0_FT;
//         }    

//         if(dif1<dif0)
//         {
//             osccal=osccal+1;
//             IRC_VALUE=temp1_FT;
//         }

//         if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
//         {
//             OSCCAL=osccal;
//             TADJ=MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i];
//             MC32P5232FT(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[i]);//通过调用这个函数，可以处理、记录 频率校准值 和 温度校准值   
//             if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
//             {
//                 return 1;
//             }
//         }     
//     }
//     OSCCAL=MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit;
//     TADJ=MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit;
//     return 0;

// }
/**
 * @brief У??Option CRC?
 * @return u8 У??????1-?????0-???
 */
u8 CheckOptionCRC(void)
{
    u16 temp;
    u16 RomCrcResultOpt;
    u16 OptionCS;
    
    // ??????λ?? option crc ?????????crc???????
    // option crc check
    FMReadOne(0x01F084); // option crc???
    temp = Rxdata;
    FMReadOne(0x01F085);
    RomCrcResultOpt = Rxdata * 0x100 + temp;
    OptionCS = CalculateOptionCRCStruct(0xff);
    
    if(OptionCS != RomCrcResultOpt)
    {
        
        return 0;
    }
    
    return 1;
}
u8 CheckRomCRC(void)
{
    u16 temp;
    u16 RomCrcResultOpt;
    u16 RomCS;
    
    // ??????λ?? option crc ?????????crc???????
    // option crc check
        FMReadOne(0x01F902);//option crc???
        temp=Rxdata;
        FMReadOne(0x01F903);
        RomCrcResultOpt = Rxdata*0x100 +temp;
    RomCrcResultOpt = Rxdata * 0x100 + temp;
    RomCS = CalculateCRCStruct(0xff);
    
    if(RomCS != RomCrcResultOpt)
    {
        
        return 0;
    }
    
    return 1;
}