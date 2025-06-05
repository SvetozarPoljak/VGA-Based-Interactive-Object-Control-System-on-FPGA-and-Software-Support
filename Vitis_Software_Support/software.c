#include <stdio.h>
#include "xparameters.h"
#include "platform.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "xbasic_types.h"
#include "sleep.h"

/*******************************TIMER_DODATAK**********************************************/
#include "xil_exception.h"
#include "xscugic.h"
/*****************************************************************************************/

/**************************************DIMENZIJE*******************************************/

#define A 20 //dimenzije kvadrata - Pek-Mena
//Neka se inicijalno preko VGA kontrolera iscrtavaju 2 pravougaonika, cije su dimenzije:
#define A_PRAV1 50
#define B_PRAV1 80
#define A_PRAV2 40
#define B_PRAV2 80
/*******************************************************************************************/

/*********************************POCETNE_KOORDINATE*****************************************/
//Neka je njihov polozaj odredjen sa:
#define X_PAC 0
#define Y_PAC 0

#define X_PRAV1 50
#define Y_PRAV1 50

#define X_PRAV2 200
#define Y_PRAV2 50
/*******************************************************************************************/

/************************************GPIO**************************************************/
#define SWITCH_BASEADDR 0x41200000
#define BUTTON_BASEADDR 0x41210000
#define LED_BASEADDR 0x41220000
/******************************************************************************************/

/*******************************TIMER_DODATAK_ID*******************************************/
#define TIMER_BASEADDR XPAR_AXI_TIMER_0_BASEADDR
#define TIMER_INTERRUPT_ID XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR

// Procesorsko jezgro takodje ima svoj ID koji se koristi da omoguci prekide
#define INTC_DEVICE_ID XPAR_PS7_SCUGIC_0_DEVICE_ID

// Konstante za registre koriscene da adresiraju tacne registre unutar AXI TIMER IP-a
#define XIL_AXI_TIMER_TCSR_OFFSET 0x0
#define XIL_AXI_TIMER_TLR_OFFSET 0x4
#define XIL_AXI_TIMER_TCR_OFFSET 0x8

// Konstante koriscene da setuju AXI Timer registre
#define XIL_AXI_TIMER_CSR_ENABLE_ALL_MASK 0x00000400
#define XIL_AXI_TIMER_CSR_ENABLE_PWM_MASK 0x00000200
#define XIL_AXI_TIMER_CSR_INT_OCCURED_MASK 0x00000100 // 0000 0000 0000 0000 0000 0001 0000 0000
#define XIL_AXI_TIMER_CSR_ENABLE_TMR_MASK 0x00000080
#define XIL_AXI_TIMER_CSR_ENABLE_INT_MASK 0x00000040
#define XIL_AXI_TIMER_CSR_LOAD_MASK 0x00000020
#define XIL_AXI_TIMER_CSR_AUTO_RELOAD_MASK 0x00000010
#define XIL_AXI_TIMER_CSR_EXT_GENERATE_MASK 0x00000004
#define XIL_AXI_TIMER_CSR_DOWN_COUNT_MASK 0x00000002
/******************************************************************************************/

/****************************************VGA***********************************************/
#define CRVENA 0xF800
#define PLAVA 0x1F
#define ZELENA 0x7E0
/*******************************************************************************************/

/*
 * opis:	Fja bez povratne vrednosti koja printuje na VGA Pek-Mena
 *
 * params:	x i y koordinata Pek-Mena i njegova boja
 *
 * retVal:	Nema
 * */
void BojajPekMena(volatile u16 x, volatile u16 y, u16 boja);

/*
 * opis:	Fja bez povratne vrednosti koja printuje na VGA poligon crveni pravougaonik po kome se
 * 			Pek ne sme kretati
 *
 * params:	x i y koordinata pravougaonika, njegova boja i njegove dimenzije
 *
 * retVal:	Nema
 * */
void BojajPravougaonike(u16 x, u16 y, u16 boja, u16 dimA, u16 dimB);

/*
 * opis:	Fja bez povratne vrednosti koja printuje na VGA poligon bele tacke koje Pek jede
 * 			ukoliko je tacka pojedena (detekcijom sudara Pek-a i tacke) najpre je u globalnom nizu
 * 			tacka_je_prebrisana[] setuje pozicija rezervisana za tacku na 1, te se preko prostora gde je do
 * 			tada bila ista ispisuje na VGA crni kvadrat.
 * 			Setovanje se vrsi kako bi se ispis crnog kvadrata desio samo jednom (inace bi Pek trepereo svaki
 * 			put kad predje preko pozicije dotadasnje bele tacke)
 *
 * params:	x i y koordinata bele tacke, u16 x i u16 y
 * 			u8 pojedena - pri pozivu se prosledjuje neka vrednost niza status_pojedenosti[] (element tu ima vrednost
 * 						  0 ako nije pojedena bela tacka i 1 ako jeste) koja kaze ako je pojedena bela tacka ne ispisuj
 * 						  je vec proveri da li je preko nje vec nacrtan crni kvadrat (da li je vec prebrisana
 * 						  tacka_je_prebrisana[neka_pozicija] = 1) ako jeste ne radi nista, a ako nije iscrtaj ga.
 * 						  Ako tacka uopste nije pojedena, iscrtaj je.
 *
 * retVal:	Nema
 * */
void BojajTacke(u16 x, u16 y, u8 *pojedena, u8 *vec_si_je_pojeo);

/* opis : 	Da li se kvadrat koordinate (x_k, y_k) svog gornjeg levog coska, dimenzije a_k preklapa sa
 * 			pravougaonikom dimenzija a_p i b_p, cija su koordinate gornjeg levog coska (x_p, y_p)
 *
 * retVal :	1 - da, preklapa se
 * 			0 - ne, ne preklapa se
 *
 * params : u16 x_k i u16 y_k - trenutna x i y koordinata kvadrata (Pek-Mena)
 * 			u16 a_k 		  - dimenzija kvadrata (Pek-Mena)
 * 			u16 x_p i u16 y_p - trenutna x i y koordinata pravougaonika (kasnije koriscena fja i za bele tacke
 * 								tako da u sustini x i y koordinata objeta kome detektujemo sudar sa Pek-Men-om)
 * 			u16 a_p i u16 b_p - dimenzije drugog objekta
 * */

u8 Sudar(u16 x_k, u16 y_k, u16 a_k, u16 x_p, u16 y_p, u16 a_p, u16 b_p);

/*
 * opis :	Dodatna for petlja koja prolazi kroz 15 iteracija i ispisuje sve tacke pozivom fje BojajTacke()
 *
 * params :	Nema
 *
 * retVal : Nema
 * */
void IspisiSveTacke();

// Preuzeta funkcije sa vezbi, koje hendluju tajmer IP,
// prekidna rutina opsluzuje zadatak VGA igrice
u32 Setup_Interrupt(u32 DeviceId, Xil_InterruptHandler Handler, u32 interrupt_ID);
void Timer_Interrupt_Handler();
static void Setup_And_Start_Timer(unsigned int milliseconds);

int switches = 0;
int buttons = 0;
int leds = 0;

// Maska bele tacke - crna(0x0000) & bela(xFFFF) = crna(0x0000)
//					  bela(0xFFFF) & bela(xFFFF) = bela(0xFFFF)
u16 bela_tacka[12][12] = {
						{0x0000,0x0000,0x0000,0x0000,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x0000,0x0000,0x0000,0x0000},
						{0x0000,0x0000,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x0000,0x0000},
						{0x0000,0x0000,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x0000},
						{0x0000,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0X0000},
						{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
						{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
						{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
						{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF},
						{0x0000,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x0000},
						{0x0000,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x0000},
						{0x0000,0x0000,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x0000,0x0000},
						{0x0000,0x0000,0x0000,0x0000,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x0000,0x0000,0x0000,0x0000},
						};

// x i y koordinate koje reprezentuju predefinisane pozicije belih tacaka na pocetku VGA igrice
u16 pozicije_tacaka[15][2] = {
						{25, 25},
						{32, 145},
						{42, 215},
						{123, 25},
						{132, 145},
						{142, 215},
						{260, 29},
						{280, 134},
						{290, 200},
						{200, 169},
						{120, 189},
						{220, 200},
						{200, 10},
						{60, 20},
						{220, 30}
						};

/* pod statusom se misli da li je tacka pojedena ili ne:
 * 1 - da, pojedena je
 * 0 - ne, nije pojedena
 */
/*volatile*/ u8 status_tacaka[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* tacka je prebrisana ukoliko je Pek-Men dosao do nje i ona je postala "crni kvadrat"
 * da se ne bi u ISR stalno smenjivao ispis crnog kvadrata i Pek-Mena preko njega
 * uvodimo globalni statusni niz prebrisanosti gde 0 signalizira da jos nije tacka pojedena
 * i da jos nije prebrisana (takodje hvata situaciju koja nas je i motivisala da napravimo ovaj niz, a to je
 * slucaj kada je pojedena tacka i jos uvek nismo belu tacku prebrisali crnim kvadratom, a to treba samo jednom
 * uraditi jer u suprotnom imamo gore navedeni problem. Kada se to obavi u ovaj niz na poziciju rezervisanu za tu
 * tacku (mesto u nizu 0-14) se upisuje 1 i vise se ne razmatra ispis crnog ekrana na tu poziciju
 */
u8 tacka_je_prebrisana[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

volatile u16 y = Y_PAC;
volatile u16 x = X_PAC;
volatile u8 broj_pojedenih_tacaka = 0;

int main()
{
	int status;
	int data;
	init_platform();

	print("VGA pokrenut\n\r");
	Xil_DCacheDisable();
	Xil_ICacheDisable();
	status = Setup_Interrupt(INTC_DEVICE_ID, (Xil_InterruptHandler)Timer_Interrupt_Handler, TIMER_INTERRUPT_ID);

	while(1)
	{

		Setup_And_Start_Timer(5);

		//Klir interapt flega
		data = Xil_In32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET);
		Xil_Out32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET, (data | XIL_AXI_TIMER_CSR_INT_OCCURED_MASK));

		// Onesposobi tajmer
		//data = Xil_In32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET);
		//Xil_Out32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET, data & ~(XIL_AXI_TIMER_CSR_ENABLE_TMR_MASK));

		usleep(50000);
	}
	cleanup_platform();

	return 0;
}

void BojajPekMena(volatile u16 x, volatile u16 y, u16 boja)
{
	for(volatile u16 j = y; j < y + A; j++)
	{
		for(volatile u16 i = x; i < x + A; i++)
		{
			Xil_Out16(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR + (j*320+i)*2, (u16)boja);
		}
	}
}

void BojajPravougaonike(u16 x, u16 y, u16 boja, u16 dimA, u16 dimB)
{
	for(u16 j = y; j < y + dimB; j++)
	{
		for(u16 i = x; i < x + dimA; i++)
		{
			Xil_Out16(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR + (j*320+i)*2, CRVENA);
		}
	}
}

void BojajTacke(u16 x, u16 y, u8 *pojedena, u8 *vec_si_je_pojeo)
{
	if(!*(pojedena)){
		for(u16 j = y; j < y + 12; j++)
		{
			for(u16 i = x; i < x + 12; i++)
			{
				Xil_Out16(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR + (j*320+i)*2, (bela_tacka[i - x][j - y] & (CRVENA | PLAVA | ZELENA)));
			}
		}
	}
	else
	{
		if(!*(vec_si_je_pojeo))
		{
			*vec_si_je_pojeo = 1;
			for(u16 j = y; j < y + 12; j++)
			{
				for(u16 i = x; i < x + 12; i++)
				{
					Xil_Out16(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR + (j*320+i)*2, 0);
				}
			}
		}
	}
}

void IspisiSveTacke()
{
	u8 i;
	for(i = 0; i < 15; i++)
	{
		BojajTacke(pozicije_tacaka[i][0], pozicije_tacaka[i][1], status_tacaka + i, tacka_je_prebrisana + i);
	}
}

u8 Sudar(u16 x_k, u16 y_k, u16 a_k, u16 x_p, u16 y_p, u16 a_p, u16 b_p)
{
	u8 ima_sudara = 0;
	if(((x_k > x_p - a_k) && (x_k < x_p + a_p)) && ((y_k > y_p - a_k) && (y_k < y_p + b_p)))
	{
		ima_sudara = 1;
	}

	return ima_sudara;
}

void DodirTacke(u16 x_k, u16 y_k, u16 a_k, u16 a_p, u16 b_p)
{
	u8 i;
	for(i = 0; i < 15; i++)
	{
		if(Sudar(x_k, y_k, a_k, pozicije_tacaka[i][0], pozicije_tacaka[i][1], a_p, b_p))
		{
			if(!*(status_tacaka + i)){
				*(status_tacaka + i) = 1;
				broj_pojedenih_tacaka++;
			}
		}
	}
}


u32 Setup_Interrupt(u32 DeviceId, Xil_InterruptHandler Handler, u32 interrupt_ID)
{
	XScuGic_Config *IntcConfig;
	XScuGic INTCInst;
	int status;
	// Extracts informations about processor core based on its ID, and they are used to setup interrupts
	IntcConfig = XScuGic_LookupConfig(DeviceId);
	// Initializes processor registers using information extracted in the previous step
	status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
	if(status != XST_SUCCESS) return XST_FAILURE;
	status = XScuGic_SelfTest(&INTCInst);
	if (status != XST_SUCCESS) return XST_FAILURE;
	// Connect Timer Handler And Enable Interrupt
	// The processor can have multiple interrupt sources, and we must setup trigger and priority
	// for the our interrupt. For this we are using interrupt ID.
	XScuGic_SetPriorityTriggerType(&INTCInst, interrupt_ID, 0xA8, 3);
	// Connects out interrupt with the appropriate ISR (Handler)
	status = XScuGic_Connect(&INTCInst, interrupt_ID, Handler, (void *)&INTCInst);
	if(status != XST_SUCCESS) return XST_FAILURE;
	// Enable interrupt for out device
	XScuGic_Enable(&INTCInst, interrupt_ID);
	//Two lines bellow enable exeptions
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler,&INTCInst);
	Xil_ExceptionEnable();
	return XST_SUCCESS;
}



// Prekidna rutima tajmera
void Timer_Interrupt_Handler()
{
	u16 boja = 0;
	switches = Xil_In32(SWITCH_BASEADDR);

	{
		if(switches&0x1)
		{
			boja |= CRVENA;
			BojajPekMena(x, y, boja);
		}else
		{
			boja &= ~CRVENA;
			BojajPekMena(x, y, boja);
		}

		if(switches&0x2)
		{
			boja |= ZELENA;
			BojajPekMena(x, y, boja);
		}
		else
		{
			boja &= ~ZELENA;
			BojajPekMena(x, y, boja);
		}

		if(switches&0x4)
		{
			boja |= PLAVA;
			BojajPekMena(x, y, boja);
		}
		else
		{
			boja &= ~PLAVA;
			BojajPekMena(x, y, boja);
		}
	}

	BojajPravougaonike(X_PRAV1, Y_PRAV1, CRVENA, A_PRAV1, B_PRAV1);
	BojajPravougaonike(X_PRAV2, Y_PRAV2, CRVENA, A_PRAV2, B_PRAV2);

	IspisiSveTacke();

	Xil_Out32(LED_BASEADDR, broj_pojedenih_tacaka);

	buttons = Xil_In32(BUTTON_BASEADDR);
	{
			if(buttons&0x1)
			{
				if(x <= (319 - A - 1))
				{
					if((!Sudar(x + 1, y, A, X_PRAV1, Y_PRAV1, A_PRAV1, B_PRAV1)) && (!Sudar(x + 1, y, A, X_PRAV2, Y_PRAV2, A_PRAV2, B_PRAV2)))
					{
						BojajPekMena(x, y, 0);
						x+=1;
						DodirTacke(x, y, A, 12, 12);
						BojajPekMena(x, y, boja);
					}
				}
			}

			if(buttons&0x2)
			{
				if(y >= 1)
				{
					if((!Sudar(x, y - 1, A, X_PRAV1, Y_PRAV1, A_PRAV1, B_PRAV1)) && (!Sudar(x, y - 1, A, X_PRAV2, Y_PRAV2, A_PRAV2, B_PRAV2)))
					{
						BojajPekMena(x, y, 0);
						y-=1;
						DodirTacke(x, y, A, 12, 12);
						BojajPekMena(x, y, boja);
					}
				}
			}

			if(buttons&0x4)
			{
				if(y <= (239 - A - 1))
				{
					if((!Sudar(x, y + 1, A, X_PRAV1, Y_PRAV1, A_PRAV1, B_PRAV1)) && (!Sudar(x, y + 1, A, X_PRAV2, Y_PRAV2, A_PRAV2, B_PRAV2)))
					{
						BojajPekMena(x, y, 0);
						y+=1;
						DodirTacke(x, y, A, 12, 12);
						BojajPekMena(x, y, boja);
					}
				}
			}

			if(buttons&0x8)
			{
				if(x >= 1)
				{
					if((!Sudar(x - 1, y, A, X_PRAV1, Y_PRAV1, A_PRAV1, B_PRAV1)) && (!Sudar(x - 1, y, A, X_PRAV2, Y_PRAV2, A_PRAV2, B_PRAV2)))
					{
						BojajPekMena(x, y, 0);
						x--;
						DodirTacke(x, y, A, 12, 12);
						BojajPekMena(x, y, boja);
					}
				}
			}
		}

	return;
}

static void Setup_And_Start_Timer(unsigned int milliseconds)
{
	// Disable Timer Counter
	unsigned int timer_load;
	unsigned int zero = 0;
	unsigned int data = 0;
	// Line bellow is true if timer is working on 100MHz
	timer_load = zero - milliseconds*100000;
	// Disable timer/counter while configuration is in progress
	data = Xil_In32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET);
	Xil_Out32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET, (data & ~(XIL_AXI_TIMER_CSR_ENABLE_TMR_MASK)));
	// Set initial value in load register
	Xil_Out32(TIMER_BASEADDR + XIL_AXI_TIMER_TLR_OFFSET, timer_load);
	// Load initial value into counter from load register
	data = Xil_In32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET);
	Xil_Out32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET, (data | XIL_AXI_TIMER_CSR_LOAD_MASK));
	// Set LOAD0 bit from the previous step to zero
	data = Xil_In32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET);
	Xil_Out32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET, (data & ~(XIL_AXI_TIMER_CSR_LOAD_MASK)));
	// Enable interrupts and autoreload, reset should be zero
	Xil_Out32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET, (XIL_AXI_TIMER_CSR_ENABLE_INT_MASK | XIL_AXI_TIMER_CSR_AUTO_RELOAD_MASK));
	// Start Timer by setting enable signal
	data = Xil_In32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET);
	Xil_Out32(TIMER_BASEADDR + XIL_AXI_TIMER_TCSR_OFFSET, (data | XIL_AXI_TIMER_CSR_ENABLE_TMR_MASK));
}





