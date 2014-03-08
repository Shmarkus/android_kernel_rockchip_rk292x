#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include "zet6223_fw.h"

#include <mach/iomux.h>
//#include <mach/gpio.h>
#include <mach/board.h>

//#define ZET6221_FW
#define ZET6223_FW

#define ZET6221_DOWNLOADER_NAME "zet6221_downloader"

#define TS_INT_GPIO		RK2928_PIN3_PC7//S3C64XX_GPN(9)   /*s3c6410*/
#define TS_RST_GPIO		RK2928_PIN3_PC3//S3C64XX_GPN(10)  /*s3c6410*/
#define RSTPIN_ENABLE

#define GPIO_LOW 	0
#define GPIO_HIGH 	1

static u8 fw_version0;
static u8 fw_version1;

#define debug_mode 0
#define DPRINTK(fmt,args...)	do { if (debug_mode) printk(KERN_EMERG "[%s][%d] "fmt"\n", __FUNCTION__, __LINE__, ##args);} while(0)


#if defined(ZET6221_FW)
static unsigned char zeitec_zet6221_page[130] __initdata;
static unsigned char zeitec_zet6221_page_in[130] __initdata;
static u16 fb[8] = {0x3DF1,0x3DF4,0x3DF7,0x3DFA,0x3EF6,0x3EF9,0x3EFC,0x3EFF};
#elif defined(ZET6223_FW)
static unsigned char zeitec_zet6223_page[131] __initdata;
static unsigned char zeitec_zet6223_page_in[131] __initdata;
static u16 fb[8] = {0x7BFC,0x7BFD,0x7BFE,0x7BFF,0x7C04,0x7C05,0x7C06,0x7C07};
#endif
//static u16 fb[8] = {0x3EEA,0x3EED,0x3EF0,0x3EF3,0x3EF6,0x3EF9,0x3EFC,0x3EFF};
//static u16 fb[8] = {0x3DF1,0x3DF4,0x3DF7,0x3DFA,0x3EF6,0x3EF9,0x3EFC,0x3EFF};

extern s32 zet6221_i2c_write_tsdata(struct i2c_client *client, u8 *data, u8 length);
extern s32 zet6221_i2c_read_tsdata(struct i2c_client *client, u8 *data, u8 length);
extern u8 pc[];

//#define I2C_CTPM_ADDRESS        (0x76)

/***********************************************************************
[function]: 
    callback: write data to ctpm by i2c interface,implemented by special user;
[parameters]:
 	client[in]			:i2c client structure;
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[in]         :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    1    :success;
    0    :fail;
************************************************************************/
int i2c_write_interface(struct i2c_client *client, u8 bt_ctpm_addr, u8* pbt_buf, u16 dw_lenth)
{
	struct i2c_msg msg;
	msg.addr = bt_ctpm_addr;
	msg.flags = 0;
	msg.len = dw_lenth;
	msg.buf = pbt_buf;
	msg.scl_rate= 300*1000;
	return i2c_transfer(client->adapter,&msg, 1);
}

/***********************************************************************
[function]: 
    callback: read data from ctpm by i2c interface,implemented by special user;
[parameters]:
 	client[in]			:i2c client structure;
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[out]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    1    :success;
    0    :fail;
************************************************************************/
int i2c_read_interface(struct i2c_client *client, u8 bt_ctpm_addr, u8* pbt_buf, u16 dw_lenth)
{
	struct i2c_msg msg;
	msg.addr = bt_ctpm_addr;
	msg.flags = I2C_M_RD;
	msg.len = dw_lenth;
	msg.buf = pbt_buf;
	msg.scl_rate= 300*1000;
	return i2c_transfer(client->adapter,&msg, 1);
}

/*************************6221_FW*******************************/
#if defined(ZET6221_FW)
/***********************************************************************
    [function]: 
		        callback: check version;
    [parameters]:
    			void

    [return]:
			    0: different 1: same;
************************************************************************/
u8 zet6221_ts_version()
{	
	int i;
	
#if 1
	printk("pc: ");
	for(i=0;i<8;i++)
		printk("%02x ",pc[i]);
	printk("\n");
	
	printk("src: ");
	for(i=0;i<8;i++)
		printk("%02x ",zeitec_zet6221_firmware[fb[i]]);
	printk("\n");
#endif
	
	for(i=0;i<8;i++)
		if(pc[i]!=zeitec_zet6221_firmware[fb[i]])
			return 0;
			
	return 1;
}


/***********************************************************************
    [function]: 
		        callback: send password;
    [parameters]:
    			client[in]:  struct i2c_client — represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_sndpwd(struct i2c_client *client)
{
	u8 ts_sndpwd_cmd[3] = {0x20,0xC5,0x9D};
	
	int ret;

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_sndpwd_cmd, 3);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_sndpwd_cmd, 3);
#endif
	
	return ret;
}

/***********************************************************************
    [function]: 
		        callback: set/check sfr information;
    [parameters]:
    			client[in]:  struct i2c_client — represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_sfr(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x2C};
	u8 ts_in_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_cmd17[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_sfr_data[16] = {0x18,0x76,0x27,0x27,0xFF,0x03,0x8E,0x14,0x00,0x38,0x82,0xEC,0x00,0x00,0x7d,0x03};
	int ret;
	int i;
	
	printk("\nsfr cmd : "); 
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 1);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
#endif
	msleep(1);

	printk("%02x \nsfr rcv : ",ts_cmd[0]); 
	
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, ts_in_data, 16);
#else
	ret=zet6221_i2c_read_tsdata(client, ts_in_data, 16);
#endif
	msleep(1);

	for(i=0;i<16;i++)
	{
		ts_cmd17[i+1]=ts_in_data[i];
		printk("%02x ",ts_in_data[i]); 
		
#if 1
		if(i>1 && i<8)
		{
			if(ts_in_data[i]!=ts_sfr_data[i])
				return 0;
		}
#endif

	}
	printk("\n"); 

	if(ts_in_data[14]!=0x3D)
	{
		ts_cmd17[15]=0x3D;
		
		ts_cmd17[0]=0x2B;	
		
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd17, 17);
#else
		ret=zet6221_i2c_write_tsdata(client, ts_cmd17, 17);
#endif

		if(ret<0)
		{
			printk("enable sfr(0x3D) failed!\n"); 
			return 0;
		}

	}
	
	return 1;
}

/***********************************************************************
    [function]: 
		        callback: mass erase flash;
    [parameters]:
    			client[in]:  struct i2c_client — represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_masserase(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x24};
	
	int ret;

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 1);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
#endif
	
	return 1;
}

/***********************************************************************
    [function]: 
		        callback: erase flash by page;
    [parameters]:
    			client[in]:  struct i2c_client — represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_pageerase(struct i2c_client *client,int npage)
{
	u8 ts_cmd[2] = {0x23,0x00};
	
	int ret;

	ts_cmd[1]=npage;
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 2);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 2);
#endif
	
	return 1;
}

/***********************************************************************
    [function]: 
		        callback: reset mcu;
    [parameters]:
    			client[in]:  struct i2c_client — represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_resetmcu(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x29};
	
	int ret;

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 1);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
#endif
	
	return 1;
}

/***********************************************************************
    [function]: 
		        callback: start HW function;
    [parameters]:
    			client[in]:  struct i2c_client — represent an I2C slave device;

    [return]:
			    1;
************************************************************************/
u8 zet6221_ts_hwcmd(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0xB9};
	
	int ret;

#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, ts_cmd, 1);
#else
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
#endif
	
	return 1;
}

/***********************************************************************
update FW
************************************************************************/
int __init zet6221_downloader( struct i2c_client *client )
{
	int BufLen=0;
	int BufPage=0;
	int BufIndex=0;
	int ret;
	int i;
	
	int nowBufLen=0;
	int nowBufPage=0;
	int nowBufIndex=0;
	int retryCount=0;
	
begin_download:
	
#if defined(RSTPIN_ENABLE)
	//reset mcu
	gpio_direction_output(TS_RST_GPIO, GPIO_LOW);
	msleep(5);
#else
	zet6221_ts_hwcmd(client);
	msleep(200);
#endif
	//send password
	//send password
	ret=zet6221_ts_sndpwd(client);
	msleep(100);
	
	if(ret<=0)
		return ret;
	
/*****compare version*******/

	//0~3
	memset(zeitec_zet6221_page_in,0x00,130);
	zeitec_zet6221_page_in[0]=0x25;
	zeitec_zet6221_page_in[1]=(fb[0] >> 7);      //(fb[0]/128);
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 2);
#else
		ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page_in, 2);
#endif

	if(ret<=0)
		return ret;	
	
	zeitec_zet6221_page_in[0]=0x0;
	zeitec_zet6221_page_in[1]=0x0;
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 128);
#else
	ret=zet6221_i2c_read_tsdata(client, zeitec_zet6221_page_in, 128);
#endif

	if(ret<=0)
		return ret;
		
	//printk("page=%d ",(fb[0] >> 7));             //(fb[0]/128));
	for(i=0;i<4;i++)
	{
		pc[i]=zeitec_zet6221_page_in[(fb[i] & 0x7f)];     //[(fb[i]%128)];
		//printk("offset[%d]=%d ",i,(fb[i] & 0x7f));        //(fb[i]%128));
	}
	//printk("\n");
	
	
	//4~7
	memset(zeitec_zet6221_page_in,0x00,130);
	zeitec_zet6221_page_in[0]=0x25;
	zeitec_zet6221_page_in[1]=(fb[4] >> 7);			//(fb[4]/128);
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 2);
#else
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page_in, 2);
#endif

	if(ret<=0)
		return ret;
	
	zeitec_zet6221_page_in[0]=0x0;
	zeitec_zet6221_page_in[1]=0x0;
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 128);
#else
		ret=zet6221_i2c_read_tsdata(client, zeitec_zet6221_page_in, 128);
#endif

	if(ret<=0)
		return ret;

	//printk("page=%d ",(fb[4] >> 7)); //(fb[4]/128));
	for(i=4;i<8;i++)
	{
		pc[i]=zeitec_zet6221_page_in[(fb[i] & 0x7f)]; 		//[(fb[i]%128)];
		//printk("offset[%d]=%d ",i,(fb[i] & 0x7f));  		//(fb[i]%128));
	}
	//printk("\n");
	
	//page 127
	memset(zeitec_zet6221_page_in,0x00,130);
	zeitec_zet6221_page_in[0]=0x25;
	zeitec_zet6221_page_in[1]=127;
#if defined(I2C_CTPM_ADDRESS)
	ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 2);
#else
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page_in, 2);
#endif

	if(ret<=0)
		return ret;	
	
	zeitec_zet6221_page_in[0]=0x0;
	zeitec_zet6221_page_in[1]=0x0;
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 128);
#else
		ret=zet6221_i2c_read_tsdata(client, zeitec_zet6221_page_in, 128);
#endif

	if(ret<=0)
		return ret;

	for(i=0;i<128;i++)
	{
		if(0x3F80+i < sizeof(zeitec_zet6221_firmware)/sizeof(char))
		{
			if(zeitec_zet6221_page_in[i]!=zeitec_zet6221_firmware[0x3F80+i])
			{
				printk("page 127 [%d] doesn't match! continue to download!\n",i);
				goto proc_sfr;
			}
		}
	}

	if(zet6221_ts_version()!=0)
		goto exit_download;
		
/*****compare version*******/

proc_sfr:
	//sfr
	if(zet6221_ts_sfr(client)==0)
	{
		//comment out to disable sfr checking loop
		//return 0;

#if 1

#if defined(RSTPIN_ENABLE)
	
		gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
		msleep(20);
		
		gpio_direction_output(TS_RST_GPIO, GPIO_LOW);
		msleep(20);
		
		gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
#else
		zet6221_ts_resetmcu(client);
#endif	
		msleep(20);
		goto begin_download;
		
#endif

	}
	msleep(20);
	
	//comment out to enable download procedure
	//return 1;	

	//erase
	if(BufLen==0)
	{
		//mass erase
		//DPRINTK( "mass erase\n");
		zet6221_ts_masserase(client);
		msleep(200);

		BufLen=sizeof(zeitec_zet6221_firmware)/sizeof(char);
	}else
	{
		zet6221_ts_pageerase(client,BufPage);
		msleep(200);
	}

	
	while(BufLen>0)
	{
download_page:

		memset(zeitec_zet6221_page,0x00,130);
		
		DPRINTK( "Start: write page%d\n",BufPage);
		nowBufIndex=BufIndex;
		nowBufLen=BufLen;
		nowBufPage=BufPage;
		
		if(BufLen>128)
		{
			for(i=0;i<128;i++)
			{
				zeitec_zet6221_page[i+2]=zeitec_zet6221_firmware[BufIndex];
				BufIndex+=1;
			}
			zeitec_zet6221_page[0]=0x22;
			zeitec_zet6221_page[1]=BufPage;
			BufLen-=128;
		}
		else
		{
			for(i=0;i<BufLen;i++)
			{
				zeitec_zet6221_page[i+2]=zeitec_zet6221_firmware[BufIndex];
				BufIndex+=1;
			}
			zeitec_zet6221_page[0]=0x22;
			zeitec_zet6221_page[1]=BufPage;
			BufLen=0;
		}
		
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page, 130);
#else
		ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page, 130);
#endif
		msleep(200);
		
#if 1

		memset(zeitec_zet6221_page_in,0x00,130);
		zeitec_zet6221_page_in[0]=0x25;
		zeitec_zet6221_page_in[1]=BufPage;
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_write_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 2);
#else
		ret=zet6221_i2c_write_tsdata(client, zeitec_zet6221_page_in, 2);
#endif
	
		zeitec_zet6221_page_in[0]=0x0;
		zeitec_zet6221_page_in[1]=0x0;
#if defined(I2C_CTPM_ADDRESS)
		ret=i2c_read_interface(client, I2C_CTPM_ADDRESS, zeitec_zet6221_page_in, 128);
#else
		ret=zet6221_i2c_read_tsdata(client, zeitec_zet6221_page_in, 128);
#endif
		
		for(i=0;i<128;i++)
		{
			if(i < nowBufLen)
			{
				if(zeitec_zet6221_page[i+2]!=zeitec_zet6221_page_in[i])
				{
					BufIndex=nowBufIndex;
					BufLen=nowBufLen;
					BufPage=nowBufPage;
				
					if(retryCount < 5)
					{
						retryCount++;
						goto download_page;
					}else
					{
						//BufIndex=0;
						//BufLen=0;
						//BufPage=0;
						retryCount=0;
						
#if defined(RSTPIN_ENABLE)
						gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
						msleep(20);
		
						gpio_direction_output(TS_RST_GPIO, GPIO_LOW);
						msleep(20);
		
						gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
#else
						zet6221_ts_resetmcu(client);
#endif	
						msleep(20);
						goto begin_download;
					}

				}
			}
		}
		
#endif
		retryCount=0;
		BufPage+=1;
	}
	
exit_download:

#if defined(RSTPIN_ENABLE)
	gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
	msleep(100);
#endif

	zet6221_ts_resetmcu(client);
	msleep(100);

}
#endif

/*************************6221_FW*******************************/

/*************************6223_FW*******************************/
#if defined(ZET6223_FW)

u8 zet6223_ts_sndpwd(struct i2c_client *client)
{
	u8 ts_sndpwd_cmd[3] = {0x20,0xC5,0x9D};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_sndpwd_cmd, 3);
	
	return ret;
}

u8 zet6223_ts_sndpwd1K(struct i2c_client *client)
{
	u8 ts_sndpwd_cmd[3] = {0x20,0xB9,0xA3};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_sndpwd_cmd, 3);
	
	return ret;
}

u8 zet6223_ts_sfr(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x2C};
	u8 ts_in_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_cmd17[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_sfr_data[16] = {0x18,0x76,0x27,0x27,0xFF,0x03,0x8E,0x14,0x00,0x38,0x82,0xEC,0x00,0x00,0x7d,0x03};
	int ret;
	int i;
	

	printk("\nwrite : "); 
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	msleep(1);
	
	printk("%02x ",ts_cmd[0]); 
	
	printk("\nread : "); 
	ret=zet6221_i2c_read_tsdata(client, ts_in_data, 16);

	msleep(1);

	for(i=0;i<16;i++)
	{
		ts_cmd17[i+1]=ts_in_data[i];
		printk("%02x ",ts_in_data[i]); 

#if 1
		if(i>1 && i<8)
		{
			if(ts_in_data[i]!=ts_sfr_data[i])
				return 0;
		}
#endif

	}
	printk("\n"); 


	if(ts_in_data[14]==0x3D)
	{
			
		ts_cmd17[i+1]=ts_in_data[i];
		
	}else
	{
		ts_cmd17[15]=0x3D;
		
		ts_cmd17[0]=0x2B;	
		
		printk("write \n"); 

		ret=zet6221_i2c_write_tsdata(client, ts_cmd17, 17);

	
		printk("ts_in_data[14]!=0x3D\n"); 
		for(i=0;i<17;i++)
		{
			printk("%02x ",ts_cmd17[i]); 
		}
		printk("\n"); 
	}
	
	return 1;
}

u8 zet6223_ts_masserase(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x24};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet6223_ts_pageerase(struct i2c_client *client,int npage)
{
	u8 ts_cmd[3] = {0x23,0x00,0x00};
	
	int ret;

	ts_cmd[1]=npage & 0xff;
	ts_cmd[2]=npage >> 8;
	
	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 3);

	return 1;
}

u8 zet6223_ts_resetmcu(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x29};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet6223_ts_hwcmd(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0xB9};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);
	
	return 1;
}

u8 zet6221_ts_version()
{	
	int i;
	printk("pc: ");
	for(i=0;i<8;i++)
		printk("%02x ",pc[i]);
	printk("\n");
	
	printk("src: ");
	for(i=0;i<8;i++)
		printk("%02x ",zeitec_zet6223_firmware[fb[i]]);
	printk("\n");
	
	for(i=0;i<8;i++)
		if(pc[i]!=zeitec_zet6223_firmware[fb[i]])
			return 0;
			
	return 1;
}

//support compatible
int __init zet6221_downloader( struct i2c_client *client, unsigned short ver, unsigned char * data )
{
	int BufLen=0;
	int BufPage=0;
	int BufIndex=0;
	int ret;
	int i;
	
	int nowBufLen=0;
	int nowBufPage=0;
	int nowBufIndex=0;
	int retryCount=0;
	
begin_download:
	
#if defined(RSTPIN_ENABLE)
	//reset mcu
	msleep(5);
	gpio_direction_output(TS_RST_GPIO, 0);
	msleep(5);
	gpio_set_value(TS_RST_GPIO,GPIO_LOW);
	//if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup")){
	//		pr_info("%s: err when operate gpio. \n", __func__);
	//}
	msleep(20);
#else
	zet6223_ts_hwcmd(client);
	msleep(200);
#endif

	//send password
//	DPRINTK( "send passwd\n");
	ret=zet6223_ts_sndpwd(client);
	msleep(200);
	ret=zet6223_ts_sndpwd1K(client);
	msleep(200);
	
	if(ret<=0)
		return ret;

/*****compare version*******/

	//0~3
	//printk("-------------------------------1111111111\n");
	memset(zeitec_zet6223_page_in,0x00,131);
	zeitec_zet6223_page_in[0]=0x25;
	zeitec_zet6223_page_in[1]=(fb[0] >> 7) & 0xff; //(fb[0]/128);
	zeitec_zet6223_page_in[2]=(fb[0] >> 7) >> 8; //(fb[0]/128);
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet6223_page_in, 3);

	if(ret<=0)
		return ret;
	//printk("-------------------------------------------2222222\n");

	zeitec_zet6223_page_in[0]=0x0;
	zeitec_zet6223_page_in[1]=0x0;
	zeitec_zet6223_page_in[2]=0x0;
	ret=zet6221_i2c_read_tsdata(client, zeitec_zet6223_page_in, 128);

	if(ret<=0)
		return ret;
	
	printk("page=%d ",(fb[0] >> 7));//(fb[0]/128));
	for(i=0;i<4;i++)
	{
		pc[i]=zeitec_zet6223_page_in[(fb[i] & 0x7f)]; //[(fb[i]%128)];
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f)); //(fb[i]%128));
	}
	printk("\n");

	
	/*
	printk("page=%d ",(fb[0] >> 7));
	for(i=0;i<4;i++)
	{
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));
	}
	printk("\n");
	*/
	
	//4~7
	memset(zeitec_zet6223_page_in,0x00,131);
	zeitec_zet6223_page_in[0]=0x25;
	zeitec_zet6223_page_in[1]=(fb[4] >> 7) & 0xff; //(fb[4]/128);
	zeitec_zet6223_page_in[2]=(fb[4] >> 7) >> 8; //(fb[4]/128);
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet6223_page_in, 3);

	if(ret<=0)
		return ret;
	
	zeitec_zet6223_page_in[0]=0x0;
	zeitec_zet6223_page_in[1]=0x0;
	zeitec_zet6223_page_in[2]=0x0;
	ret=zet6221_i2c_read_tsdata(client, zeitec_zet6223_page_in, 128);

	printk("page=%d ",(fb[4] >> 7)); //(fb[4]/128));
	for(i=4;i<8;i++)
	{
		pc[i]=zeitec_zet6223_page_in[(fb[i] & 0x7f)]; //[(fb[i]%128)];
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));  //(fb[i]%128));
	}
	printk("\n");
	
	if(ret<=0)
		return ret;
	

	if(zet6221_ts_version()!=0)
		goto exit_download;
		
/*****compare version*******/

proc_sfr:
	//sfr
//	DPRINTK( "check sfr\n");
	if(zet6223_ts_sfr(client)==0)
	{
		//comment out to disable sfr checking loop
		//return 0;
#if 1

#if defined(RSTPIN_ENABLE)
	gpio_direction_output(TS_RST_GPIO, 0);
	msleep(5);
	gpio_set_value(TS_RST_GPIO,GPIO_HIGH);
	msleep(20);
	gpio_set_value(TS_RST_GPIO,GPIO_LOW);
	msleep(20);
	gpio_set_value(TS_RST_GPIO,GPIO_HIGH);

/*		if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup")){
				pr_info("%s: err when operate gpio. \n", __func__);
		}
		msleep(20);
		
		if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup")){
			pr_info("%s: err when operate gpio. \n", __func__);
		}
		msleep(20);
		
		if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup")){
				pr_info("%s: err when operate gpio. \n", __func__);
		}*/
#else
		zet6223_ts_resetmcu(client);
#endif	
		msleep(20);
		goto begin_download;
		
#endif

	}
	msleep(20);
	
	//comment out to enable download procedure
	//return 1;
	
	//erase
	if(BufLen==0)
	{
		//mass erase
		//DPRINTK( "mass erase\n");
		zet6223_ts_masserase(client);
		msleep(200);

		BufLen=sizeof(zeitec_zet6223_firmware)/sizeof(char);
	}else
	{
		zet6223_ts_pageerase(client,BufPage);
		msleep(200);
	}
	
	
	while(BufLen>0)
	{
download_page:

		memset(zeitec_zet6223_page,0x00,131);
		
		//DPRINTK( "Start: write page%d\n",BufPage);
		printk( "Start: write page%d\n",BufPage);
		nowBufIndex=BufIndex;
		nowBufLen=BufLen;
		nowBufPage=BufPage;
		
		if(BufLen>128)
		{
			for(i=0;i<128;i++)
			{
				zeitec_zet6223_page[i+3]=zeitec_zet6223_firmware[BufIndex];
				BufIndex+=1;
			}
			zeitec_zet6223_page[0]=0x22;
			zeitec_zet6223_page[1]=BufPage & 0xff;
			zeitec_zet6223_page[2]=BufPage >> 8;
			BufLen-=128;
		}
		else
		{
			for(i=0;i<BufLen;i++)
			{
				zeitec_zet6223_page[i+3]=zeitec_zet6223_firmware[BufIndex];
				BufIndex+=1;
			}
			zeitec_zet6223_page[0]=0x22;
			zeitec_zet6223_page[1]=BufPage & 0xff;
			zeitec_zet6223_page[2]=BufPage >> 8;
			BufLen=0;
		}
//		DPRINTK( "End: write page%d\n",BufPage);

		ret=zet6221_i2c_write_tsdata(client, zeitec_zet6223_page, 131);

		msleep(200);
		
#if 1

		memset(zeitec_zet6223_page_in,0x00,131);
		zeitec_zet6223_page_in[0]=0x25;
		zeitec_zet6223_page_in[1]=BufPage & 0xff;
		zeitec_zet6223_page_in[2]=BufPage >> 8;
		ret=zet6221_i2c_write_tsdata(client, zeitec_zet6223_page_in, 3);

	
		zeitec_zet6223_page_in[0]=0x0;
		zeitec_zet6223_page_in[1]=0x0;
		zeitec_zet6223_page_in[2]=0x0;
		ret=zet6221_i2c_read_tsdata(client, zeitec_zet6223_page_in, 128);
		
		for(i=0;i<128;i++)
		{
			if(i < nowBufLen)
			{
				if(zeitec_zet6223_page[i+3]!=zeitec_zet6223_page_in[i])
				{
					BufIndex=nowBufIndex;
					BufLen=nowBufLen;
					BufPage=nowBufPage;
				
					if(retryCount < 5)
					{
						retryCount++;
						goto download_page;
					}else
					{
						//BufIndex=0;
						//BufLen=0;
						//BufPage=0;
						retryCount=0;
						
#if defined(RSTPIN_ENABLE)

	gpio_direction_output(TS_RST_GPIO, 0);
	msleep(5);
	gpio_set_value(TS_RST_GPIO,GPIO_HIGH);
	msleep(20);
	gpio_set_value(TS_RST_GPIO,GPIO_LOW);
	msleep(20);
	gpio_set_value(TS_RST_GPIO,GPIO_HIGH);
				/*
						if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup")){
							pr_info("%s: err when operate gpio. \n", __func__);
						}
						msleep(20);
		
						if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup")){
							pr_info("%s: err when operate gpio. \n", __func__);
						}
						msleep(20);
		
						if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup")){
							pr_info("%s: err when operate gpio. \n", __func__);
						}*/
#else
						zet6223_ts_resetmcu(client);
#endif	
						msleep(20);
						goto begin_download;
					}

				}
			}
		}
		
#endif
		retryCount=0;
		BufPage+=1;
	}

exit_download:

	zet6223_ts_resetmcu(client);
	msleep(100);

#if defined(RSTPIN_ENABLE)
	gpio_direction_output(TS_RST_GPIO, 0);
	msleep(5);
	gpio_set_value(TS_RST_GPIO,GPIO_HIGH);
/*	if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup")){
			pr_info("%s: err when operate gpio. \n", __func__);
	}*/
	msleep(200);

#endif


	return 1;

}


#endif
/*************************6223_FW*******************************/
