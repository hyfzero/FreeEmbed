#ifndef erorrNum_h
#define erorrNum_h

//MC20P24B_Program
#define		IRC_ModeIn_false	2	//IRC校准 进入模式失败
#define		IRC_Value_false		3	//IRC校准频率值 超出范围
#define		OTP_ModeIn_false	4	//进入烧写模式失败
#define		VDD_Test_false		5	//VDD电压超出 7.0V~6.2V范围
#define		VPP_Test_false		6	//VPP电压超出13V~~11.5V范围
#define		IRC_Write_false		7	//IRC校准值烧写失败
#define		ROM_Write_false		8	//ROM区数据烧写失败

#define		FM_Read_false		  9	//程序缓存区读取失败
#define		OPTION_Write_false	10	//OPTION值烧写失败
#define		Program_Erorr		11	//烧写异常
#define		IRC_Value_0XFF		12	//校准结果为 0XFF or 0x00
#define		EEPROM_Write_ERROR	13   //写EEPROM 出错
#define		OK_Counter_Full		14	 //烧写数量达到最大限制数
#define		VS_Low				15	 //版本号太低
#define     ADDR_EER            16   //OTP地址与烧写地址不一致

//MC32T8132 擦除错误ID
#define         ERASE_STA_xx0       17
#define         ERASE_STA_011       18
#define         ERASE_STA_101       19
#define         ERASE_STA_111       20


//5312增加错误
#define         HIRC_DIFF       21//芯片被写过，并且本次用的内部高频振荡器频率和之前的不一样

//MC33P116 失败测试用
#define         NOMALL_ERROR           22//普通读错
#define         MAGRIN1_ERROR          23//
#define         OFF_MAGRIN_ERROR       24
#define         CHECK0_ERROR           25

//MC32P5222 MTP烧写错误
#define         MTP_Write_false        26

#define         CHECK_PROGRAM_ERROR     27 //程序空间校验出错    
#define         NO_VREF_ERROR           28//无VREF校准值
#define         NO_LVD_ERROR            29//无LVD校准值    
#define         OPTION_MCU_ERROR        30 //芯片型号与配置不匹配
#define         OPTION_NOMALL_ERROR           31//OPTION普通读错
#define         OPTION_MAGRIN1_ERROR          32//
#define         OPTION_OFF_MAGRIN_ERROR       33
#define         S19_ERROR                     34
#define         OS_Error                      35
#define         CRC_ERROR                     40  
#define         BADDOT_ERROR                  41
#define         SDO_ERROR                     42 //编程通讯 检测SDO数据是否和输入SDI数据一致 不一致报错
#define         LoadOption_ERROR              43//进模式加载option到寄存器出错
#define         Unlock_ERROR                  43 //FLASH 写A5 5A失败
#define         OPTION_NOT_Complemental       44 //FLASH Option 区数据不互补

#define         LVR_ERROR                     48//LVR校准出错
#define         LVR_CHECK_ERROR               49//LVR校准出错

#define         FLASH_ERASE_ERROR             55

#define         CRC_CODE_NOT_MATCH_ERROR               70 //上位机传下来的code CRC 与芯片内算出的code CRC不匹配
#define         CRC_OPT_NOT_MATCH_ERROR           71 //上位机传下来的OPT CRC 与芯片内算出的OPT CRC不匹配
#endif