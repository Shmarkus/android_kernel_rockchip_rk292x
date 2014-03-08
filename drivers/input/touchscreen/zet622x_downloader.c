#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/slab.h>
//#include "zet6221_fw.h"
//#include "zet6223_fw_de_cyz_bsr.h"
//#include "zet6223_fw_de_cyz_bsr_v2.h"
//#include "zet6223_fw_donghao.h"
//#include "zet6223_fw_donghao_v2.h"
//#include "zet6223_fw_feichu_20130625.h"
//#include "zet6223_fw_feichu_20130709.h"
#include "ZET6223_fw_with_sig3.h"


#define FEATURE_FW_SIGNATURE 
#define ZET6221_DOWNLOADER_NAME "zet6221_downloader"

///=========================================================================================///
///  Signature
///=========================================================================================///
#ifdef FEATURE_FW_SIGNATURE
#define SIG_PAGE_ID             (255)   ///< ?章所在的?
#define SIG_DATA_LEN            (4)     ///< ?章所在的?
#define SIG_DATA_ADDR           (128  - SIG_DATA_LEN)   ///< ?章所在的位址
static const u8 sig_data[SIG_DATA_LEN] = {'Z', 'e', 'i', 'T'};
#endif ///< for FEATURE_FW_SIGNATURE

///=========================================================================================
#define TRUE 1
#define FALSE 0

#define MODEL_ZET6221 0
#define FLASH_PAGE_LEN 128

static u8 zet_tx_data[131] __initdata;
static u8 zet_rx_data[131] __initdata;


///==========================================================================================
#define TS_INT_GPIO	RK2928_PIN3_PC7
#define TS_RST_GPIO	RK2928_PIN3_PC3


#define RSTPIN_ENABLE       1                    
                       
#define GPIO_LOW 	0
#define GPIO_HIGH 	1
                
static u8 fw_version0;
static u8 fw_version1;
                                                                       
#define debug_mode 1
#define DPRINTK(fmt,args...)	do { if (debug_mode) printk(KERN_EMERG "[%s][%d] "fmt"\n", __FUNCTION__, __LINE__, ##args);} while(0)

static unsigned char zeitec_zet622x_page[131] __initdata;
static unsigned char zeitec_zet622x_page_in[131] __initdata;
//static u16 fb[8] = {0x3EEA,0x3EED,0x3EF0,0x3EF3,0x3EF6,0x3EF9,0x3EFC,0x3EFF};
static u16 fb[8] = {0x3DF1,0x3DF4,0x3DF7,0x3DFA,0x3EF6,0x3EF9,0x3EFC,0x3EFF};
static u16 fb21[8] = {0x3DF1,0x3DF4,0x3DF7,0x3DFA,0x3EF6,0x3EF9,0x3EFC,0x3EFF}; 
static u16 fb23[8] = {0x7BFC,0x7BFD,0x7BFE,0x7BFF,0x7C04,0x7C05,0x7C06,0x7C07};
static u8 ic_model = 0;	// 0:6221 / 1:6223

extern s32 zet6221_i2c_write_tsdata(struct i2c_client *client, u8 *data, u8 length);
extern s32 zet6221_i2c_read_tsdata(struct i2c_client *client, u8 *data, u8 length);
extern u8 pc[];

//#define I2C_CTPM_ADDRESS        (0x76)

u8 *flash_buffer = NULL;
struct inode *inode = NULL;
mm_segment_t old_fs;
#define MAX_FLASH_BUF_SIZE			(0x10000)

u8 zet622x_ts_sndpwd(struct i2c_client *client)
{
	u8 ts_sndpwd_cmd[3] = {0x20,0xC5,0x9D};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_sndpwd_cmd, 3);

	return ret;
}

u8 zet622x_ts_sndpwd_1k(struct i2c_client *client)
{
	u8 ts_sndpwd_cmd[3] = {0x20,0xB9,0xA3};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_sndpwd_cmd, 3);

	return ret;
}

u8 zet622x_ts_option(struct i2c_client *client)
{

	u8 ts_cmd[1] = {0x27};
	u8 ts_cmd_erase[1] = {0x28};
	u8 ts_in_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_out_data[18] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int ret;
	u16 model;
	int i;
	
	printk("\noption write : "); 

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	msleep(1);
	
	printk("%02x ",ts_cmd[0]); 
	
	printk("\nread : "); 

	ret=zet6221_i2c_read_tsdata(client, ts_in_data, 16);

	msleep(1);

	for(i=0;i<16;i++)
	{
		printk("%02x ",ts_in_data[i]); 
	}
	printk("\n"); 

	model = 0x0;
	model = ts_in_data[7];
	model = (model << 8) | ts_in_data[6];
//	model = 0x6223;
	
	switch(model) { 
        case 0xFFFF: 
        	ret = 1;
            ic_model = 0;
			for(i=0;i<8;i++)
			{
				fb[i]=fb21[i];
			}
			
#if defined(High_Impendence_Mode)

			if(ts_in_data[2] != 0xf1)
			{
				if(zet622x_ts_sfr(client)==0)
				{
					return 0;
				}
			
				ret=zet6221_i2c_write_tsdata(client, ts_cmd_erase, 1);
				msleep(1);
			
				for(i=2;i<18;i++)
				{
					ts_out_data[i]=ts_in_data[i-2];
				}
				ts_out_data[0] = 0x21;
				ts_out_data[1] = 0xc5;
				ts_out_data[4] = 0xf1;
			
				ret=zet6221_i2c_write_tsdata(client, ts_out_data, 18);
				msleep(1);
			
				//
				ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

				msleep(1);
	
				printk("%02x ",ts_cmd[0]); 
	
				printk("\n(2)read : "); 

				ret=zet6221_i2c_read_tsdata(client, ts_in_data, 16);

				msleep(1);

				for(i=0;i<16;i++)
				{
					printk("%02x ",ts_in_data[i]); 
				}
				printk("\n"); 
				
			}
									
#endif					
    	break; 
    case 0x6223:
      ret = 1;
			ic_model = 1;
			for(i=0;i<8;i++)
			{
				fb[i]=fb23[i];
			}
      break; 
    default: 
      ret=0; 
  } 

	return ret;
}

u8 zet622x_ts_sfr(struct i2c_client *client)
{

	u8 ts_cmd[1] = {0x2C};
	u8 ts_in_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u8 ts_cmd17[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
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
	}
	printk("\n"); 

#if 1
	if(ts_in_data[14]!=0x3D && ts_in_data[14]!=0x7D)
	{
		return 0;
	}
#endif

	if(ts_in_data[14]!=0x3D)
	{
		ts_cmd17[15]=0x3D;
		
		ts_cmd17[0]=0x2B;	
		
		ret=zet6221_i2c_write_tsdata(client, ts_cmd17, 17);
	}
	
	return 1;
}

u8 zet622x_ts_masserase(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x24};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet622x_ts_pageerase(struct i2c_client *client,int npage)
{
	u8 ts_cmd[3] = {0x23,0x00,0x00};
	u8 len=0;
	int ret;

	switch(ic_model)
	{
		case 0: //6221
			ts_cmd[1]=npage;
			len=2;
			break;
		case 1: //6223
			ts_cmd[1]=npage & 0xff;
			ts_cmd[2]=npage >> 8;
			len=3;
			break;
	}

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, len);

	return 1;
}

u8 zet622x_ts_resetmcu(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0x29};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet622x_ts_hwcmd(struct i2c_client *client)
{
	u8 ts_cmd[1] = {0xB9};
	
	int ret;

	ret=zet6221_i2c_write_tsdata(client, ts_cmd, 1);

	return 1;
}

u8 zet622x_ts_version()
{	
	int i;
		
	printk("pc: ");
	for(i=0;i<8;i++)
		printk("%02x ",pc[i]);
	printk("\n");
	
	printk("src: ");
	for(i=0;i<8;i++)
		printk("%02x ",zeitec_zet622x_firmware[fb[i]]);
	printk("\n");
	
	for(i=0;i<8;i++)
		if(pc[i]!=zeitec_zet622x_firmware[fb[i]])
			return 0;
			
	return 1;
}
#ifdef FEATURE_FW_SIGNATURE
///************************************************************************
///   [function]:  zet622x_ts_sig_check
///   [parameters]: client
///   [return]: void
///************************************************************************
///************************************************************************
///   [function]:  zet_fw_init
///   [parameters]: void
///   [return]: void
///************************************************************************
void zet_fw_init(void)
{
	int i;
	
	flash_buffer = kmalloc(MAX_FLASH_BUF_SIZE, GFP_KERNEL);	

	printk("[ZET]: Load from header\n");
	for(i = 0 ; i < sizeof(zeitec_zet622x_firmware); i++)
	{
		flash_buffer[i] = zeitec_zet622x_firmware[i];
	}
	/// Load firmware from bin file
//	zet_fw_load(FW_FILE_NAME);
}

///************************************************************************
///   [function]:  zet_fw_exit
///   [parameters]: void
///   [return]: void
///************************************************************************
void zet_fw_exit(void)
{
	kfree(flash_buffer);
	flash_buffer = NULL;
}
int zet622x_ts_sig_check(struct i2c_client *client)
{
	int i;
	int ret = TRUE;
	u8 read_page_cmd[3];
	if(ic_model== MODEL_ZET6221)
	{
		printk("[ZET]: signature check ignored\n");
		return	TRUE;
	}
    /// Read sig page
	
	//ret = zet622x_cmd_readpage(client, SIG_PAGE_ID, &zet_rx_data[0]);

	read_page_cmd[0] = 0x25;
	read_page_cmd[1] = (u8)((SIG_PAGE_ID) & 0xff);
	read_page_cmd[2] = (u8)((SIG_PAGE_ID) >> 8);
	ret=zet6221_i2c_write_tsdata(client, read_page_cmd, 3);
	if(ret<=0)
	{
		printk("[ZET]: Write page command fail");
		return ret;
	}

	ret = zet6221_i2c_read_tsdata(client, zet_rx_data, FLASH_PAGE_LEN);
	if(ret<=0)		
	{
		printk("[ZET]: Read page data fail");
		return ret;
	}

	/// Clear the signature position
	for(i = 0 ; i < SIG_DATA_LEN ; i++)
	{
		flash_buffer[SIG_PAGE_ID * FLASH_PAGE_LEN + SIG_DATA_ADDR + i] = 0xFF;
	}
	/// check signature
	printk("[ZET]: zet_rx_data[] =  ");
    	for(i = 0 ; i < 128 ; i++)
	{
		printk("%02X ", zet_rx_data[i]);
    	}
	printk("\n");
	
	printk("[ZET]: sig_curr[] =  ");
    	for(i = 0 ; i < SIG_DATA_LEN ; i++)
	{
		printk("%02X ", zet_rx_data[i + SIG_DATA_ADDR]);
   	}
	printk("\n");

	printk("[ZET]: sig_data[] =  ");
    	for(i = 0 ; i < SIG_DATA_LEN ; i++)
	{
		printk("%02X ", sig_data[i]);
    	}
	printk("\n");      

    	printk("[ZET]: sig_data[] =  ");
	for(i = 0 ; i < SIG_DATA_LEN ; i++)
	{
		if(zet_rx_data[i + SIG_DATA_ADDR] != sig_data[i])
		{
			printk("[ZET]: signature check : not signatured!!\n");
			return FALSE;
		}
	}
	printk("[ZET]: signature check : signatured\n");
	return  TRUE;
}

///************************************************************************
///   [function]:  zet622x_ts_sig_write
///   [parameters]: client
///   [return]: void
///************************************************************************

int zet622x_ts_sig_write(struct i2c_client *client)
{
	int i;
	int ret;
	u8 p255_buf[128];
	u8 read_page_cmd[3];
    /// if zet6221, then leaves
	if(ic_model== MODEL_ZET6221)
	{
		printk("[ZET]: signature write ignore\n");
		return	TRUE;
	}
    /// set signature
	for(i=0;i<FLASH_PAGE_LEN;i++)
	{
		zet_tx_data[i] = flash_buffer[SIG_PAGE_ID * FLASH_PAGE_LEN + i];
	}

	printk("[ZET] : old data fw\n");
        for(i=0;i<FLASH_PAGE_LEN;i++)
        {
                printk("%02x ", zet_tx_data[i]);
                if((i%0x10) == 0x0F)
                {
                        printk("\n");
                }
                else if((i%0x08) == 0x07)
                {
                        printk(" - ");
                }
        }

	read_page_cmd[0] = 0x25;
	read_page_cmd[1] = (u8)((SIG_PAGE_ID) & 0xff);
	read_page_cmd[2] = (u8)((SIG_PAGE_ID) >> 8);
	ret=zet6221_i2c_write_tsdata(client, read_page_cmd, 3);
	if(ret<=0)
	{
		printk("[ZET]: Read page command fail");
		return ret;
	}
	msleep(5);

	ret = zet6221_i2c_read_tsdata(client, zet_rx_data, FLASH_PAGE_LEN);
	if(ret<=0)		
	{
		printk("[ZET]: Read page data fail");
		return ret;
	}
	msleep(10);
	printk("[ZET] : old data flash\n");
        for(i=0;i<FLASH_PAGE_LEN;i++)
        {
                printk("%02x ", zet_rx_data[i]);
                if((i%0x10) == 0x0F)
                {
                        printk("\n");
                }
                else if((i%0x08) == 0x07)
                {
                        printk(" - ");
                }
        }
	
	/// set signature
	for(i = 0 ; i < SIG_DATA_LEN ; i++)
	{
			zet_rx_data[ i + SIG_DATA_ADDR] = sig_data[i];
	}

	printk("[ZET] : new data\n");
    for(i=0;i<FLASH_PAGE_LEN;i++)
	{
		printk("%02x ", zet_rx_data[i]);
		if((i%0x10) == 0x0F)
		{
			printk("\n");
		}
		else if((i%0x08) == 0x07)
		{
			printk(" - ");
		}		
	}

    /// write sig page
	/*
	ret = zet622x_cmd_writepage(client, SIG_PAGE_ID, &zet_tx_data[0]);
    if(ret<=0)
    {
		printk("[ZET]: signature write fail\n");
        return FALSE;
	}
	*/

	zet622x_ts_pageerase(client,SIG_PAGE_ID);
	msleep(100);
	
	read_page_cmd[0] = 0x25;
	read_page_cmd[1] = (u8)((SIG_PAGE_ID) & 0xff);
	read_page_cmd[2] = (u8)((SIG_PAGE_ID) >> 8);
	ret=zet6221_i2c_write_tsdata(client, read_page_cmd, 3);
	if(ret<=0)
	{
		printk("[ZET]: Read page command fail");
		return ret;
	}
	msleep(5);

	ret = zet6221_i2c_read_tsdata(client, p255_buf, FLASH_PAGE_LEN);
	if(ret<=0)		
	{
		printk("[ZET]: Read page data fail");
		return ret;
	}
	msleep(10);
	printk("[ZET] : pageerase flash\n");
        for(i=0;i<FLASH_PAGE_LEN;i++)
        {
                printk("%02x ", p255_buf[i]);
                if((i%0x10) == 0x0F)
                {
                        printk("\n");
                }
                else if((i%0x08) == 0x07)
                {
                        printk(" - ");
                }
        }


    	zet_tx_data[0] = 0x22;
	zet_tx_data[1] = (u8)((SIG_PAGE_ID) & 0xff);
	zet_tx_data[2] = (u8)((SIG_PAGE_ID) >> 8);
	for(i=3;i<131;i++)
	{
		zet_tx_data[i] = zet_rx_data[i-3];
	}
	ret = zet6221_i2c_write_tsdata(client, zet_tx_data, 131);
	if(ret <=0)
	{
		printk("[ZET] : write page %d failed!!", SIG_PAGE_ID);
	}

	msleep(50);
	ret = zet622x_ts_sig_check(client);
	if(ret<=0)
        {
		printk("[ZET]: signature write fail\n");
        	return FALSE;
	}
	printk("[ZET]: signature write ok\n");	
	return TRUE;
}


#endif ///< for FEATURE_FW_SIGNATURE

//support compatible
int __init zet622x_downloader( struct i2c_client *client )
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
	
	int i2cLength=0;
	int bufOffset=0;
	u8 mass_erase = 1;
	
	u8 download_ok = 1;
	
begin_download:
	
	//reset mcu
	gpio_direction_output(TS_RST_GPIO, GPIO_LOW);
	msleep(20);


	//send password
	ret=zet622x_ts_sndpwd(client);
	msleep(20);
	
	if(ret<=0)
		return ret;
		
	ret=zet622x_ts_option(client);
	msleep(20);
	
	if(ret<=0)
		return ret;

	/// send password 1k (ZET6223)
	if(ic_model == 1) ///< (ZET6223)
	{
		ret=zet622x_ts_sndpwd_1k(client);
		msleep(20);
		if(ret<=0)
		{
			return ret;
		}
	}
	

/*****compare version*******/

	//0~3
	memset(zeitec_zet622x_page_in,0x00,131);
	
	switch(ic_model)
	{
		case 0: //6221
			zeitec_zet622x_page_in[0]=0x25;
			zeitec_zet622x_page_in[1]=(fb[0] >> 7);//(fb[0]/128);
			
			i2cLength=2;
			break;
		case 1: //6223
			zeitec_zet622x_page_in[0]=0x25;
			zeitec_zet622x_page_in[1]=(fb[0] >> 7) & 0xff; //(fb[0]/128);
			zeitec_zet622x_page_in[2]=(fb[0] >> 7) >> 8; //(fb[0]/128);
			
			i2cLength=3;
			break;
	}
	
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet622x_page_in, i2cLength);

	if(ret<=0)
		return ret;
	
	zeitec_zet622x_page_in[0]=0x0;
	zeitec_zet622x_page_in[1]=0x0;
	zeitec_zet622x_page_in[2]=0x0;

	ret=zet6221_i2c_read_tsdata(client, zeitec_zet622x_page_in, 128);
	
/**************************************************
Add By wangke(gary)
**************************************************/
	for(i=0;i<FLASH_PAGE_LEN;i++)
	{
		printk("%02x ", zeitec_zet622x_page_in[i]);
		if((i%0x10) == 0x0F)
		{
			printk("\n");
		}
		else if((i%0x08) == 0x07)
		{
			printk(" - ");
		}		
	}
/**************************************************/

	if(ret<=0)
		return ret;
	
	printk("page=%d ",(fb[0] >> 7));//(fb[0]/128));
	for(i=0;i<4;i++)
	{
		pc[i]=zeitec_zet622x_page_in[(fb[i] & 0x7f)];//[(fb[i]%128)];
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));//(fb[i]%128));
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
	memset(zeitec_zet622x_page_in,0x00,131);
	
	switch(ic_model)
	{
		case 0: //6221
			zeitec_zet622x_page_in[0]=0x25;
			zeitec_zet622x_page_in[1]=(fb[4] >> 7);//(fb[4]/128);
			
			i2cLength=2;
			break;
		case 1: //6223
			zeitec_zet622x_page_in[0]=0x25;
			zeitec_zet622x_page_in[1]=(fb[4] >> 7) & 0xff; //(fb[4]/128);
			zeitec_zet622x_page_in[2]=(fb[4] >> 7) >> 8; //(fb[4]/128);
			
			i2cLength=3;
			break;
	}
	
	ret=zet6221_i2c_write_tsdata(client, zeitec_zet622x_page_in, i2cLength);

	if(ret<=0)
		return ret;
	
	zeitec_zet622x_page_in[0]=0x0;
	zeitec_zet622x_page_in[1]=0x0;
	zeitec_zet622x_page_in[2]=0x0;

	ret=zet6221_i2c_read_tsdata(client, zeitec_zet622x_page_in, 128);


	printk("page=%d ",(fb[4] >> 7)); //(fb[4]/128));
	for(i=4;i<8;i++)
	{
		pc[i]=zeitec_zet622x_page_in[(fb[i] & 0x7f)]; //[(fb[i]%128)];
		printk("offset[%d]=%d ",i,(fb[i] & 0x7f));  //(fb[i]%128));
	}
	printk("\n");
	
	if(ret<=0)
		return ret;
	

	///-----------------------------------------///
	/// 6.Check Version. If yes, Exit.
	///-----------------------------------------///
#ifdef FEATURE_FW_SIGNATURE
  if(zet622x_ts_sig_check(client)==TRUE)
  {
		///----------------------------------------///
		/// Check the data flash version
		///----------------------------------------///
		if(zet622x_ts_version() != 0)
		{
			goto exit_download;
		}
  }
#else
	if(zet622x_ts_version()!=0)
	{
		goto exit_download;
	}
#endif
		
/*****compare version*******/

proc_sfr:
	//sfr
	if(zet622x_ts_sfr(client)==0)
	{
		//comment out to disable sfr checking loop
		return 0;
#if 0

		gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
		msleep(20);
		
		gpio_direction_output(TS_RST_GPIO, GPIO_LOW);
		msleep(20);
		
		gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);

		msleep(20);
		goto begin_download;
		
#endif

	}
	msleep(20);
	
	//comment out to enable download procedure
	//return 1;
	
	//erase
	//if(BufLen==0)
	//{
		//mass erase
		//DPRINTK( "mass erase\n");
		zet622x_ts_masserase(client);
		msleep(50);

		BufLen=sizeof(zeitec_zet622x_firmware)/sizeof(char);
		printk("---[leo]---BufLen %d ---\n",BufLen);
	//}
	//else
	//{
	//	zet622x_ts_pageerase(client,BufPage);
	//	msleep(200);
	//}
	
	
	while(BufLen>0)
	{
download_page:
	
		if(mass_erase ==  0 )
		{
			zet622x_ts_pageerase(client,BufPage);
			msleep(30);			
		}

		memset(zeitec_zet622x_page,0x00,131);
		
		DPRINTK( "Start: write page%d\n",BufPage);
/**************************************************
Add By wangke(gary)
**************************************************/
		printk("---[gary]---BufLen %d ---\n",BufLen);
		printk("---[gary]---BufIndex %d ---\n",BufIndex);
/**************************************************/		
		nowBufIndex=BufIndex;
		nowBufLen=BufLen;
		nowBufPage=BufPage;
		
		switch(ic_model)
		{
			case 0: //6221
				bufOffset = 2;
				i2cLength=130;
				
				zeitec_zet622x_page[0]=0x22;
				zeitec_zet622x_page[1]=BufPage;				
				break;
			case 1: //6223
				bufOffset = 3;
				i2cLength=131;
				
				zeitec_zet622x_page[0]=0x22;
				zeitec_zet622x_page[1]=BufPage & 0xff;
				zeitec_zet622x_page[2]=BufPage >> 8;
				break;
		}
		
		if(BufLen>128)
		{
			for(i=0;i<128;i++)
			{
				//zeitec_zet622x_page[i+bufOffset]=zeitec_zet622x_firmware[BufIndex];
				zeitec_zet622x_page[i+bufOffset] = flash_buffer[BufIndex];
				BufIndex+=1;
			}

			BufLen-=128;
		}
		else
		{
			for(i=0;i<BufLen;i++)
			{
				//zeitec_zet622x_page[i+bufOffset]=zeitec_zet622x_firmware[BufIndex];
				zeitec_zet622x_page[i+bufOffset] = flash_buffer[BufIndex];
				BufIndex+=1;
			}

			BufLen=0;
		}
//		DPRINTK( "End: write page%d\n",BufPage);

		ret=zet6221_i2c_write_tsdata(client, zeitec_zet622x_page, i2cLength);

		msleep(3);
		
#if 1

		memset(zeitec_zet622x_page_in,0x00,131);
		switch(ic_model)
		{
			case 0: //6221
				zeitec_zet622x_page_in[0]=0x25;
				zeitec_zet622x_page_in[1]=BufPage;
				
				i2cLength=2;
				break;
			case 1: //6223
				zeitec_zet622x_page_in[0]=0x25;
				zeitec_zet622x_page_in[1]=BufPage & 0xff;
				zeitec_zet622x_page_in[2]=BufPage >> 8;

				i2cLength=3;
				break;
		}		
		
		ret=zet6221_i2c_write_tsdata(client, zeitec_zet622x_page_in, i2cLength);

		zeitec_zet622x_page_in[0]=0x0;
		zeitec_zet622x_page_in[1]=0x0;
		zeitec_zet622x_page_in[2]=0x0;
		
		ret=zet6221_i2c_read_tsdata(client, zeitec_zet622x_page_in, 128);

		
		for(i=0;i<128;i++)
		{
/**************************************************
Add By wangke(gary)
**************************************************/
		printk("---[gary]---nowBufLen %d ---\n",nowBufLen);
/**************************************************/			
			if(i < nowBufLen)
			{
				if(zeitec_zet622x_page[i+bufOffset]!=zeitec_zet622x_page_in[i])
				{
					BufIndex=nowBufIndex;
					BufLen=nowBufLen;
					BufPage=nowBufPage;
				
					if(retryCount < 5)
					{
						retryCount++;
						mass_erase = 0;
						goto download_page;
					}
					else
					{
						//BufIndex=0;
						//BufLen=0;
						//BufPage=0;
						retryCount=0;
						
						gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
						msleep(20);
		
						gpio_direction_output(TS_RST_GPIO, GPIO_LOW);
						msleep(20);
		
						gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
						
						download_ok = 0;
						
						msleep(20);
						goto exit_download;
					}

				}
			}
		}
		
#endif
		retryCount=0;
		BufPage+=1;
	}
#ifdef FEATURE_FW_SIGNATURE	
	if (zet622x_ts_sig_write(client) == FALSE)
	{
		DPRINTK("[ZET] : download failed!\n");
	}
#endif	

exit_download:
	if(download_ok == 0)
	{
		printk("[ZET] : download failed!\n");
	}

	zet622x_ts_resetmcu(client);
	msleep(100);

	gpio_direction_output(TS_RST_GPIO, GPIO_HIGH);
	msleep(200);

	
	return 1;

}



