2018.3.5
	1、优化IRC校准相关代码，通过配置文件适用于A600 A670之外的任意型号校准，特别是温度校准值或频率校准值占用两个option或寄存器地址
2017.3.3
	1、修改6060bug:烧录器上电后烧写完一颗芯片后DeviceConfig_xx.OptionAddr被改成0x8001，导致之后再烧非空芯片时烧错0x8001地址
2017.3.2
	1.修改6060bug:烧写完一颗芯片以后，DeviceConfig_xx.OptionAddr被改成0x8001,之后判断芯片是否被写过不同page项，应该读0x8000地址，而不是DeviceConfig_xx.OptionAddr
	  导致选择1.5V,1.6V,2.7V,2.8V的芯片烧过一次后不能再烧成功，1.8V,3.0V虽然逻辑依然不对，不过在现在6060 1k烧写时不会导致出错
2017.2.22
	1、MC32T8132增加checksum
	2、修改MC32T8132块加密后二次烧写会破坏加密块数据的bug
	3、判断MC30P6070有没有中成测值，无值报错
2016.12.12
	删除 ProgramID=0x01，0x03,0x05(老的10，20系列以及9029 9033)

2016.09.12
	1.修改版本号，使其适合V2.97
2016.05.20
	1.修改MC32P5312IRC校准bug
	2.修改MC30P6060IRC校准bug
2015.11.17
	1.MC30P6060、MC33P78\MC33P74、MC31P11、MC9029校准完成后，将校准值再送入校准一次
	2.MC30P6060校准时，测试模式关闭看门狗；正常驱动；使能端口施密特功能；端口施密特阈值选择1；屏蔽外部复位功能
	3.MC30P6060 OPBIT 的PAGE BIT BIT[13:5]取上位机传的值，之前是写0
	4.MC32P5312提示IRC校准失败
	5.MC33P116去掉4k烧录模式写checksum功能，之前地址会冲突
	6.MC34P01、MC33P78、MC31P11、MC9029、MC33P116、MC30P081、MC32P5312增加第三个温度校准点校准
2015.08.25
	1.MC32P7011正式版本IRC校准
2015.08.18
	1.mc9033 之前版本IRC校准失败报错15，现在v2.1版本改正为03
	2.mc9033 option3之前版本如果读出来不是0xff,就把读出来的数据再写进去，这是不对的，v2.1版本已经改正
2015.05.12
	1.搬移MC32P7022中测、成测值
2015.05.011
	1.按更新的开发手册要求增加MC33P116、MC34P01、MC30P081写checksum的功能
2015.05.08
	1.按更新的开发手册要求增加MC30P6060写checksum的功能
2015.04.22
	1.去掉MC33P116写checksum的功能
2015.04.21
	1.MC30P6060烧写程序，删除操作VPP_IO和VDD_IO的代码
2015.04.16
	1.搬移A040的Vref成测中测值
2015.04.14
	1.搬移A010的Vref成测中测值
	2.针对A010用了外部32K晶振后，进功能测试模式后送指令无效的bug，做了处理，如果用内部RC，在IRC校准前先写option0(FOSCS）
2015.04.10
	1.之前的版本A010 VPP_IO和VDD_IO端口操作导致VDD不能降到地，对此作了改正，不操作VPP_IO和VDD_IO
	2.AO10 IRC校准结束后，不管成功失败都关闭定时器TIMER3和TIMER4
2015.04.09
	1.增加MC32P7311测试版烧录
2015.04.07
	1.add on line write function of mc32t8132
2015.04.03
	1.增加写MC33P116CheckSum,写在0x800f地址
	2.开发MC33P116多次烧写功能
2015.03.30
	1.增加A010测试版烧录
	2.9033正式版烧录中增加7个温度校准值
	3.处理mc32p7510 中测、成测的Vref校准值
	4.mc32p7022增加第三个温度校准值
2015.02.06
	1.该版本放弃MC30P6060再次烧写同一页，用不同内部高频振荡器频率报错提示，因为如果用户并不使用内部振荡器，用外部的，并且两次
	再次配置界面时又选了不同的内部高频振荡器，就会烧写失败，本来内部怎么配置跟使用没关系，这样反而烧不进去。
	特别注意：下次再改版，这样做：根据option的选用内部外部振荡器的位进行判断，用内部才进行IRC校验，并且再次烧录同一页却用不同
	内部振荡器频率值就报错
2015.02.05
	1.修改之前mc30p081(14bit)、mc34p01(13bit)、mc31p11(13bit)一旦使用滚动码就无法烧写成功的bug
2015.02.03
	1.增加mc32p7022正式烧写(IRC校准说明参照MC32P5312)
	2.按VPP、ProTime规格要求修改mc30p6060、MC31P11、MC34P01、MC9033、MC9029、MC32P5312、MC32P7022
2015.01.21
	1.调整mc33p78、mc33p74、mc33p116的VPP=7.5v,ProTime=103us
	2.将mc33p78、mc33p74中心频率改回16M，确认mc33p116中心值为16M
	3.增加了mc30p6060正式烧写
	4.增加了mc32t8132正式烧写
2015.01.14
	1.add mc32p7510
	2.PC send real otp address to read otp data
	3.PC :mc30p6060 write different page option value in same address like other mcu
2014.03.25
	1. add mc33p116

2014.02.25
	1. ADD MC30P081,MC34P01
	2. dynamic-id funtion add max-id
		data[0]=0x0a,data[1]=length
		data[2],3, 4,5  6,7 8,9---addr
		data[10],11,12,13---------current id
		data[14]------------------shift
		data[15],16,17,18---------id_end
		dada[19],20,21.22---------ok counter
		data[23],24,25,26---------min-id
		data[27],28,29,30---------max-id
2014.01.03
	1. fixed mc32pxxx program dynamic-id
2013.11.06 
	1. fixed mc30p011 program wave. debuger for defective more than 0.5%.
2013.10.14
	1. add mc32p821,mc33p78
2013.09.10
	1.reduct funtion
	1.1 delete MC10P02 ,MC20P02 IRC adj model
	1.2 delete HC05 IRC adj model

2013.09.03
	1. add mc31p11 mc9029 mc32p64 
	2. updata mc9029 irc funtion
2013.05.07
	2. fixef mc30p011 dynamic-id verfy
2013.04.11
	1. add mc20p38 verify funtion
	2.fixed mc30p011 dynamic-id, 30p01=opcode+id
2013.03.26
	1. add mc33p94 33p716 bt version
2013.03.08
	1.fixed mc32p21,mc30p01 dynamic-id, 30p01=0x0a+id
2013.02.20
	1. add mc32p21 write program

2013.01.31
	1. fixed test model funtion,add test of eeprom device 
2012.12.21 
	1. fixed  HC05 irc adj funtion no care sdo singal
2012.09.10
    1.add iic R/W funtion for dynamic-ID
    2. 14bit RISC dynami id OTP_ADDR is not the same to 16 bit
	
2012.08.06
    1.add mc30p01 write model

2012.05.16
	增加对校准值的判断：
	   1.选择校准时，烧写器对校准结果进行测试判断。
	   2.当选择搬校准值时，先读校准参数地址值（如0X04），如果为FF/00，则再读向前偏移两字节的地址（如0X02），如
	     果仍为FF/00则 直接报错。 
	   3.当选择不校准，不搬校准值，则直接判断0X0001地址 如果为 FF/00 直接报错。
	   4.不校准，不搬校准值的MCU将一律不允许烧写。需要 紧急出货需另行通知，使用特殊工具。

2012.05.15
	发布版
2012.03.26
	LCD显示版本
	修改PA8 为MCO
	USB接口，通过IO口控制上拉电阻
	
2012.02.27
	增加MC20P38烧写时序,修改校准时序，扩大成15

烧写器主版本。

2011-10-24  修改MC10P02R IRC校准值参数。

2011-08-01  起动。型号，ROM,烧写，查空，读取，下载通过 PC端软件实现。