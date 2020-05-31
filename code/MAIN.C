/*********************************************************************
    Project:MG82F6D17-DEMO
    Author:LZD
			MG82F6D17 SSOP20_V10 EV Board (TH194A)
			CpuCLK=32MHz, SysCLK=32MHz
	Description:
			CPUCLK=SYSCLK (32M)
			PCA时钟: SYSCLK
			CH0(P22),CH1(P33)
			边沿对齐,分辨率为1000
			PWM频率: PCAClk/(1000)=32MHz/1000=32KHz
	Note:

    Creat time::
    Modify::
    
*********************************************************************/
#define _MAIN_C

#include <Intrins.h>
#include <Absacc.h>

#include <Stdio.h>  // for printf
#include <string.h>
#include <stdlib.h>


#include ".\include\REG_MG82F6D17.H"
#include ".\include\Type.h"
#include ".\include\API_Macro_MG82F6D17.H"
#include ".\include\API_Uart_BRGRL_MG82F6D17.H"


/*************************************************
Set SysClk (MAX.50MHz) (MAX.50MHz)
Selection: 
	11059200,12000000,
	22118400,24000000,
	29491200,32000000,
	44236800,48000000
*************************************************/
#define MCU_SYSCLK		12000000//32000000
/*************************************************/
/*************************************************
set CpuClk (MAX.36MHz)
	1) CpuCLK=SysCLK
	2) CpuClk=SysClk/2
*************************************************/
#define MCU_CPUCLK		(MCU_SYSCLK)
//#define MCU_CPUCLK		(MCU_SYSCLK/2)

#define TIMER_1T_1ms_TH	((65536-(u16)(float)(1000*((float)(MCU_SYSCLK)/(float)(1000000)))) /256) 			
#define TIMER_1T_1ms_TL	((65536-(u16)(float)(1000*((float)(MCU_SYSCLK)/(float)(1000000)))) %256)

#define TIMER_12T_1ms_TH	((65536-(u16)(float)(1000*((float)(MCU_SYSCLK)/(float)(12000000)))) /256) 			
#define TIMER_12T_1ms_TL	((65536-(u16)(float)(1000*((float)(MCU_SYSCLK)/(float)(12000000)))) %256)

#define LED_G_0		P33
#define LED_R		P34
#define LED_G_1		P35


#define PWM_MIN			(0*1)
#define PWM_MAX			(1000*1)
#define PWM_3_4			(750*1)
#define PWM_2_4			(500*1)
#define PWM_1_4			(250*1)
#define PWM_LOW			(40*1)
#define PWM_HINT		(50*1)

#define PCA_RELOAD		(PWM_MAX)

#define PCA_C           (65536)       	

#define PCA_CL(x)		(u8)((65536-(x))%256) 
#define PCA_CH(x)     	(u8)((65536-(x))/256)          

#define SFR_Page_(x)		SFRPI = x;

#define UNUSED(var) (void)((var) = (var))



idata WordTypeDef wDuty[2];
bit bDutyChange;
u8 DutyFlag;



#define UART1_RX_BUFF_SIZE   32   		
//#define UART1_TX_BUFF_SIZE   32   	
char acRecvBuf[UART1_RX_BUFF_SIZE]={0};
xdata u8 RcvBuf[UART1_RX_BUFF_SIZE];
u8 Uart1RxIn =0;
//u8 Uart1RxOut =0;
//xdata u8 TxBuf[UART1_TX_BUFF_SIZE];
//u8 Uart1TxIn =0;
//u8 Uart1TxOut =0;
//bit bUart1TxFlag=0;

u8 LedTime;

//-------------------------------------------
//
//-------------------------------------------


void s_pwm(char* para);
void Uart1SendStr(u8* PStr);
void DelayXus(u8 xUs);



//------------------------------------------
struct command {
  char *name;
  void (*function)(char *str);
  char *syntax;
  //char state_needed;
  //char showinfeat;
};

enum
{
   RCV_EMPTY,                              // No data packet
   RCV_READY                               // Data packet received
};


u8 gB_RcvStatus=RCV_EMPTY;
//BYTE gB_RcvComplete=0;
u8 gB_dummy =0 ;


void sendOK(void)
{
	Uart1SendStr("OK\r\n");	 

}
void sendERR(void){ // para error
	Uart1SendStr("ERR\r\n");	 

}
void sendEmpty(void){
	Uart1SendStr("\r\n");	 

}

void s_pwm(char* para)
{
	UNUSED(para);//= NULL;
    sendOK();

}

const struct command commands[] = {


  {"s_pwm", s_pwm, "s_pwm \r\n"},


   {NULL, NULL, NULL}
	
};




void cutto(char *str, int len , int buf_size)

{
	int size;// = -len + 1;
	
		size = buf_size - len + 1;
		memmove(str, str + len,  size);
		memset(str+size-1 , '\0' , len);

}
char* trimgarbage(char*str)
{
	char *p;  /* strlen("ALLOWCOMMAND_XXXX") + 1 == 18 */
	p = str;			/* Remove garbage in the string */
	
	while (*p)
	{
		if ((unsigned char) *p <= 32 || (unsigned char) *p ==0x7F) p++;
		else{
			break;
		}
	}	

    return p;
}

void remove_trim(char*str , int buf_size)

{
		char* p;
		int size;
		p = str;
	
	
		while (((*p == ' ') || (*p == '\t')))//while ((*p) && ((*p == ' ') || (*p == '\t')))
		{
			p++;
		}
		size = buf_size - (p - str) + 1;
	//	memmove(str, p, strlen(str) - (p - str) + 1);
		memmove(str, p, size);
		memset(str + (size - 1), '\0', (p - str));


}
u8 parsecmd(char *str)
{
		 int i;
	 //  char*p;
		 
		 str=trimgarbage(str);
		 //str = strtok(str,"\r\n");
		 
		 for (i = 0; commands[i].name; i++) 
			{	 /* Parse command */
			 if (!strncmp(str, commands[i].name, strlen(commands[i].name))) 
			 {
						
				 cutto(str, strlen(commands[i].name),UART1_RX_BUFF_SIZE);
				 remove_trim(str,UART1_RX_BUFF_SIZE);
				 commands[i].function(str); // pass parameter to command
				 return TRUE ;
				 
			 }
			 else // handle unsupported command
			 {
	 
			 }
		 }
			 
		if(!strlen(str))
		 { 
		   // handle enter key only
		   sendEmpty();
		   return TRUE ;
		 }	 
	   
	   return FALSE ;


}


void StopRcvMsg(void)
{
   gB_RcvStatus = RCV_READY;						// stop receiving messages

}


void StartRcvMsg(void)
{
   gB_RcvStatus = RCV_EMPTY;						// start receiving messages

}
static bit is_uart_message_complete(void)
{
  return gB_RcvStatus;

}

void UartHandler(void)
{
   
    if(is_uart_message_complete())
    { 
 
       if(!parsecmd(acRecvBuf))// parse command
       {
            sendERR();
       }

	   memset(acRecvBuf , 0 ,UART1_RX_BUFF_SIZE);
       StartRcvMsg();
    }	

 
}

/***********************************************************************************
Function:   	void INT_UART1(void)
Description:	UART1 Interrupt handler
		 
Input:   
Output:     
*************************************************************************************/
void INT_UART1(void) interrupt INT_VECTOR_UART1
{
	_push_(SFRPI);		   

	SFR_Page_(1);	
	/*
	if(TI1)					
	{
	   TI1 = 0;	   
		if(Uart1TxIn==Uart1TxOut)
		{
			bUart1TxFlag=FALSE;
		}
		else
		{
			S1BUF=TxBuf[Uart1TxOut];
			bUart1TxFlag=TRUE;
			Uart1TxOut++;
			if(Uart1TxOut>=UART1_TX_BUFF_SIZE)
			{
				Uart1TxOut=0;
			}
		}
	}
	*/
	if(RI1)					
	{
		

	  if ((gB_RcvStatus == RCV_EMPTY) && (Uart1RxIn < UART1_RX_BUFF_SIZE))  
	  {

		
		RcvBuf[Uart1RxIn] = S1BUF;
		Uart1RxIn++;

	    if((Uart1RxIn > 1) && RcvBuf[Uart1RxIn-1] == '\n' && RcvBuf[Uart1RxIn-2] == '\r')	
		{
		  StopRcvMsg();
          memcpy(acRecvBuf, RcvBuf, Uart1RxIn);
	      memset(RcvBuf , 0 ,UART1_RX_BUFF_SIZE);
	      Uart1RxIn =0;
	
		}
			 
		if(Uart1RxIn >=UART1_RX_BUFF_SIZE)
		{
			Uart1RxIn =0;
			memset(RcvBuf , 0 ,UART1_RX_BUFF_SIZE);
		}
	  }
	  else
	  {
		  gB_dummy = S1BUF; // clear the buffer
	
	  }

	  
	  RI1 = 0;
		
	}
	_pop_(SFRPI);		  
}
void InitUart1(void)
{
	UART1_SetMode8bitUARTVar();								// UART1 Mode: 8-bit, Variable B.R

	UART1_EnS1BRG();										// Enable S1BRG
	UART1_SetBaudRateX2();									// S1BRG x2
    UART1_SetRxTxP10P11();	//UART1_SetRxTxP34P35();									// UART1 Pin：RX:P34 TX:P35
	UART1_EnReception();									// Enable reception
	UART1_SetS1BRGSelSYSCLK();								// S1BRG clock source：SYSCLK

	// Sets B.R. value
	UART1_SetS1BRGValue(S1BRG_BRGRL_9600_2X_12000000_1T);	
}

void Uart1SendByte(u8 tByte)
{
#if 0
	u8 i;
	
	if(bUart1TxFlag==FALSE)
	{
		Uart1TxOut=0;
		Uart1TxIn=1;
		TxBuf[0]=tByte;
		SFR_Page_(1);
        TI1=1;
        SFR_Page_(0);
	}
	else
	{
		i=Uart1TxIn;
		TxBuf[i]=tByte;
		i++;
		if(i>=UART1_TX_BUFF_SIZE)
		{
			i=0;
		}
		while(i==Uart1TxOut)
		{
		}
		INT_DisUART1();
		Uart1TxIn=i;
		INT_EnUART1();
	}
#else

WORD usTimeOut = 1800;

// Disable Serial Port IRQ
INT_DisUART1();//ES = _DISABLE;

// Clear Flag
//TI = 0;
  SFR_Page_(1);
     TI1=0;
  SFR_Page_(0);

// Load Data to Serial Port Buffer
S1BUF = tByte;

do
{
	DelayXus(5);
}
while((TI1 == 0) && (--usTimeOut != 0));

// Enable Serial Port IRQ
 INT_EnUART1();//ES = _ENABLE;

//return ((TI != 0) && (usTimeOut != 0)) ? _TRUE : _FALSE;

	
#endif	
}



/***********************************************************************************
Function:		void Uart1SendStr(BYTE* PStr)
Description:	Uart1 send string
Input: 			u8* PStr:the string to be send
Output:     
*************************************************************************************/

void Uart1SendStr(u8* PStr)
{

	while(*PStr != 0)
	{
		Uart1SendByte(*PStr);
		PStr ++;
	}
	
}


/***********************************************************************************
Function:   void INT_PCA(void)
Description:PCA Interrupt handler
		 
Input:   
Output:     
*************************************************************************************/
void INT_PCA(void) interrupt INT_VECTOR_PCA
{
	WordTypeDef duty;
	_push_(SFRPI);
	SFRPI=0;
	if(CF)
	{
		LED_R=!LED_R;
		CF=0;
		// Todo...
		// ......
		if(bDutyChange)
		{
			duty.W=PCA_C-wDuty[0].W;
			PCA_CH0_SetValue(duty.B.BHigh,duty.B.BLow);
			PCA_CH2_SetValue(duty.B.BHigh,duty.B.BLow); //add
			duty.W=PCA_C-wDuty[1].W;
			PCA_CH1_SetValue(duty.B.BHigh,duty.B.BLow);
				PCA_CH3_SetValue(duty.B.BHigh,duty.B.BLow);//add
			bDutyChange=FALSE;
		}
	}
	_pop_(SFRPI);
}

/*************************************************
Function:     	void DelayXus(u16 xUs)
Description:   	dealy，unit:us
Input:     		u8 Us -> *1us  (1~255)
Output:     
*************************************************/
void DelayXus(u8 xUs)
{
	while(xUs!=0)
	{
#if (MCU_CPUCLK>=11059200)
		_nop_();
#endif
#if (MCU_CPUCLK>=14745600)
		_nop_();
		_nop_();
		_nop_();
		_nop_();
#endif
#if (MCU_CPUCLK>=16000000)
		_nop_();
#endif

#if (MCU_CPUCLK>=22118400)
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
#endif
#if (MCU_CPUCLK>=24000000)
		_nop_();
		_nop_();
#endif		
#if (MCU_CPUCLK>=29491200)
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
#endif
#if (MCU_CPUCLK>=32000000)
		_nop_();
		_nop_();
#endif

		xUs--;
	}
}

/*************************************************
Function:     	void DelayXms(u16 xMs)
Description:    dealy，unit:ms
Input:     		u16 xMs -> *1ms  (1~65535)
Output:     
*************************************************/
void DelayXms(u16 xMs)
{
	while(xMs!=0)
	{
		CLRWDT();
		DelayXus(200);
		DelayXus(200);
		DelayXus(200);
		DelayXus(200);
		DelayXus(200);
		xMs--;
		
	}
}

/***********************************************************************************
Function:   	void InitPort()
Description:	Initialize IO Port
Input:   
Output:   		
*************************************************************************************/
void InitPort(void)
{
	PORT_SetP1PushPull(BIT6|BIT7);						// set P17(CEX4) as push-pull for PWM output
	//PORT_SetP2PushPull(BIT2|BIT4);					// set P22(CEX0),P24(CEX2) as push-pull for PWM output
	PORT_SetP3QuasiBi(BIT0|BIT1|BIT3|BIT4|BIT5);		// set P30,P31,P33,P34,P35 as Quasi-Bidirectional
	PORT_SetP1PushPull(BIT0|BIT1);	

	PORT_SetP6PushPull(BIT0|BIT1);					// set P60(PWM6),P61(PWM7) as push-pull for PWM output
}


/***********************************************************************************
Function:   	void InitPCA_PWM(void)
Description:	Initialize PCA for PWM
Input:   
Output:   		
*************************************************************************************/
void InitPCA_PWM(void)
{
	PCA_SetCLOCK_SYSCLK();			// PCA clock: SysClk
	PCA_SetCLOCK_SYSCLKdiv12();
	PCA_CH0_SetMode_PWM();
	//PCA_CH1_SetMode_PWM();
	PCA_CH2_SetMode_PWM();
	//PCA_CH3_SetMode_PWM();

	PCA_CH0_SetPWM_16Bit();
	//PCA_CH1_SetPWM_16Bit();
	PCA_CH2_SetPWM_16Bit();
	//PCA_CH3_SetPWM_16Bit();



	PCA_SetPWM_EdgeAligned();			// Edge-aligned

	PCA_SetCounter(PCA_C-PCA_RELOAD);
	PCA_SetCounterReload(PCA_C-PCA_RELOAD);

	// Set PWM duty
	PCA_CH0_SetValue(PCA_CH(PWM_MIN),PCA_CL(PWM_MIN));
	//PCA_CH1_SetValue(PCA_CH(PWM_MIN),PCA_CL(PWM_MIN));
	PCA_CH2_SetValue(PCA_CH(PWM_MIN),PCA_CL(PWM_MIN));
	//PCA_CH3_SetValue(PCA_CH(PWM_MIN),PCA_CL(PWM_MIN));

	// Enable PWM output
	//PCA_SetPWM0_EnOutput();					
	//PCA_SetPWM1_EnOutput();

	//PCA_SetCEX0CEX2CEX4_P22P24P17();	// Set CEX0:P22,CEX2:P24,CEX4:P17
	//PCA_SetPWM6PWM7_P60P61();			// Set PWM6:P60,PWM7:P61
	PCA_SetPWM0APWM0B_P16P17();
	PCA_SetPWM2APWM2B_P60P61();			// set PWM2A:P60,PWM2B:P61

	PCA_SetPWM2_DisOutput();			// disable PWM2 output
	PCA_SetPWM2_2nd_EnOutput();			// enable PWM2A output
	PCA_SetPWM2_3rd_EnOutput();			// enable PWM2B output

	PCA_SetPWM0_DisOutput();
	PCA_SetPWM0_2nd_EnOutput();
	PCA_SetPWM0_3rd_EnOutput();

	PCA_CF_EnInterrupt();				// Enable PCA CF interrupt

	
	
	PCA_EnPCACounter();					// Enable PCA counter
		
}



/***********************************************************************************
Function:   	void InitInterrupt()
Description:	Initialize Interrupt
Input:   
Output:   		
*************************************************************************************/
void InitInterrupt(void)
{
	INT_EnUART1();						// Enable UART1 interrupt

	INT_EnPCA();						// Enable PCA interrupt
}	


/***********************************************************************************
Function:   	void InitClock()
Description:	Initialize clock
Input:   
Output:   		
*************************************************************************************/
void InitClock(void)
{
#if (MCU_SYSCLK==11059200)
#if (MCU_CPUCLK==MCU_SYSCLK)
	// SysClk=11.0592MHz CpuClk=11.0592MHz
	CLK_SetCKCON0(IHRCO_110592MHz|CPUCLK_SYSCLK_DIV_1|SYSCLK_MCKDO_DIV_1);
	
#else
	// SysClk=11.0592MHz CpuClk=5.5296MHz
	CLK_SetCKCON0(IHRCO_110592MHz|CPUCLK_SYSCLK_DIV_2|SYSCLK_MCKDO_DIV_1);
#endif
#endif

#if (MCU_SYSCLK==12000000)
#if (MCU_CPUCLK==MCU_SYSCLK)
	// SysClk=12MHz CpuClk=12MHz
	//CLK_SetCKCON0(IHRCO_12MHz|CPUCLK_SYSCLK_DIV_1|SYSCLK_MCKDO_DIV_1);
CLK_SetCKCON0(IHRCO_12MHz|CPUCLK_SYSCLK_DIV_1|SYSCLK_MCKDO_DIV_4);

	
#else
	// SysClk=12MHz CpuClk=6MHz
	CLK_SetCKCON0(IHRCO_12MHz|CPUCLK_SYSCLK_DIV_2|SYSCLK_MCKDO_DIV_1);
#endif
#endif

#if (MCU_SYSCLK==22118400)
#if (MCU_CPUCLK==MCU_SYSCLK)
	// SysClk=22.1184MHz CpuClk=22.1184MHz
	CLK_SetCKCON0(IHRCO_110592MHz|CPUCLK_SYSCLK_DIV_1|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx4, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X4|OSCIn_IHRCO);
#else
	// SysClk=22.1184MHz CpuClk=11.0592MHz
	CLK_SetCKCON0(IHRCO_110592MHz|CPUCLK_SYSCLK_DIV_2|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx4, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X4|OSCIn_IHRCO);
#endif
#endif

#if (MCU_SYSCLK==24000000)
#if (MCU_CPUCLK==MCU_SYSCLK)
	// SysClk=24MHz CpuClk=24MHz
	CLK_SetCKCON0(IHRCO_12MHz|CPUCLK_SYSCLK_DIV_1|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx4, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X4|OSCIn_IHRCO);
#else
	// SysClk=24MHz CpuClk=12MHz
	CLK_SetCKCON0(IHRCO_12MHz|CPUCLK_SYSCLK_DIV_2|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx4, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X4|OSCIn_IHRCO);
#endif
#endif

#if (MCU_SYSCLK==29491200)
#if (MCU_CPUCLK==MCU_SYSCLK)
	// SysClk=29.491200MHz CpuClk=29.491200MHz
	CLK_SetCKCON0(IHRCO_110592MHz|CPUCLK_SYSCLK_DIV_1|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx5.33, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X533|OSCIn_IHRCO);
#else
	// SysClk=29.491200MHz CpuClk=14.7456MHz
	CLK_SetCKCON0(IHRCO_110592MHz|CPUCLK_SYSCLK_DIV_2|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx5.33, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X533|OSCIn_IHRCO);
#endif
#endif

#if (MCU_SYSCLK==32000000)
#if (MCU_CPUCLK==MCU_SYSCLK)
	// SysClk=32MHz CpuClk=32MHz
	CLK_SetCKCON0(IHRCO_12MHz|CPUCLK_SYSCLK_DIV_1|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx5.33, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X533|OSCIn_IHRCO);
#else
	// SysClk=32MHz CpuClk=16MHz
	CLK_SetCKCON0(IHRCO_12MHz|CPUCLK_SYSCLK_DIV_2|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx5.33, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X533|OSCIn_IHRCO);
#endif
#endif


#if (MCU_SYSCLK==44236800)
	// SysClk=44.2368MHz CpuClk=22.1184MHz
	CLK_SetCKCON0(IHRCO_110592MHz|CPUCLK_SYSCLK_DIV_1|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx8, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X8|OSCIn_IHRCO);
#endif

#if (MCU_SYSCLK==48000000)
	// SysClk=48MHz CpuClk=24MHz
	CLK_SetCKCON0(IHRCO_12MHz|CPUCLK_SYSCLK_DIV_2|SYSCLK_MCKDO_DIV_1|ENABLE_CKM|CKM_OSCIN_DIV_2);
	DelayXus(100);
	// IHRCO, MCK=CKMIx8, OSCin=IHRCO
	CLK_SetCKCON2(ENABLE_IHRCO|MCK_CKMI_X8|OSCIn_IHRCO);
#endif

	// P60 Output MCK/4
	//CLK_P60OC_MCKDiv4();
}




/***********************************************************************************
Function:       void InitSystem(void)
Description:    Initialize MCU
Input:   
Output:     
*************************************************************************************/
void InitSystem(void)
{
	InitClock();
	InitPort();
	InitUart1();//-----------
	InitPCA_PWM();
	InitInterrupt();

}


void main()
{
	u8 i,x;
	
    InitSystem();

	LED_G_1=0;LED_R=0;
	DelayXms(1000);
	LED_G_1=1;LED_R=1;
	
	INT_EnAll();						// Enable global interrupt

	Uart1SendStr("power on init ....\r\n");

	wDuty[0].W=PWM_MIN;
	wDuty[1].W=PWM_LOW;
	DutyFlag=0x00;

	
	while(1)
    {

#if 1
	
    	DelayXms(200);
    	LED_G_1=!LED_G_1;
    	x=0x01;
    	for(i=0;i<2;i++)
    	{
			if((DutyFlag&x)==0)
			{
				wDuty[i].W=wDuty[i].W+20;
				if(wDuty[i].W >= PWM_MAX)
				{
					wDuty[i].W = PWM_MAX;
					DutyFlag=DutyFlag|x;
				}
			}
			else
			{
				if(wDuty[i].W < 21)
				{
					wDuty[i].W = PWM_MIN;
					DutyFlag=DutyFlag&(~x);
				}
				else
				{
					wDuty[i].W=wDuty[i].W-20;
				}
			}
			x=x<<1;
    	}
    	bDutyChange=TRUE;
//#else

   UartHandler();

#endif
    }
}

