#ifndef iap_h
#define iap_h

/* 类型声明 -----------------------------------------------------------------*/
typedef  void (*pFunction)(void);

//* 宏 ------------------------------------------------------------------------*/
#define CMD_STRING_SIZE       128

#define DefaultStartAddress	  0x8000000
#define ApplicationAddress    0x8004000
#define FLASH_SIZE            0x20000/* 64 KByte */
#define PAGE_SIZE		  1024 //小空量和中容量为1024，大容量为2048


//计算上传文件大小 
#define FLASH_IMAGE_SIZE                   (uint32_t) (FLASH_SIZE - (ApplicationAddress - DefaultStartAddress))


extern u32 BlockNbr;
extern u32 UserMemoryMask;
extern u8  FlashProtection;
extern pFunction Jump_To_Application;
extern u32 JumpAddress;


extern void FLASH_DisableWriteProtectionPages();
extern void IAP_funtion();
extern uint32_t FLASH_PagesMask( u32 Size);



#endif