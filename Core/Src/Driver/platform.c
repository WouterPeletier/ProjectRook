#include "platform.h"
#include <stdint.h>
#include <stm32f4xx.h>
#include "gpio.h"
/* Global variables */
static volatile uint64_t gSysticks;


/* Function Prototypes */
static void init_clock(void);
static void init_clock_hse(void);
static void init_systick(void);

void init_platform(void)
{
	init_clock_hse();
	init_systick();
}

void delay_ms(uint32_t delay)
{
	uint64_t prev, now;
	prev = now = get_sysTicks();		/* Get current sysTickTime */

	while ((now - prev) < delay) {
    	__WFI();
		now = get_sysTicks();			/* Wait for interrupt, and check if enough time has passed */
	}
}

uint64_t get_sysTicks(void)
{
	return gSysticks;
}

/* This function doesnt work!!! */
static void init_clock(void)
{
	/* Initialise clock to something reasonable */

	/* Init HSE clk */
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY)) {	}

	/* Make sure PLL is disabled */
	RCC->CR &= ~RCC_CR_PLLON;
	while ((RCC->CR & RCC_CR_PLLRDY)) {	}

	/* Reset CFGR */
	RCC->PLLCFGR = RCC_PLLCFGR_RST_VALUE;
	/* Clock source HSE */
	RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC;
	/* Division and multiplication factors */
	RCC->PLLCFGR |= (8<<RCC_PLLCFGR_PLLQ_Pos);	//PLLQ = 8
	RCC->PLLCFGR |= (3<<RCC_PLLCFGR_PLLP_Pos);	//PLLP = 8
	RCC->PLLCFGR |= (192<<RCC_PLLCFGR_PLLN_Pos);//PLLN = 192
	RCC->PLLCFGR |= (4<<RCC_PLLCFGR_PLLM_Pos);	//PLLM = 4
	/* Enable PLL */
	RCC->CR |= RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY)) {	}

	/* Set the other prescalers*/
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV1;			//APB1 /1	
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;			//APB2 /1	
	RCC->CFGR |= RCC_CFGR_HPRE_DIV4;			//AHB  /4	48Mhz/4->12Mhz

	/* Set PLL as system clock */
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	while (!(RCC->CFGR & RCC_CFGR_SWS_PLL)) {	}

	/**** Every clock is now 24MHZ ****/
	RCC->CFGR &= ~(0b11<<RCC_CFGR_MCO2_Pos);	//Sysclk (8MHZ)
	RCC->CFGR |= (0b110<<RCC_CFGR_MCO2PRE_Pos);	//MCO2 Prescaler 4	(MCO2->12MHZ)
}

static void init_clock_hse(void)
{
	/* Init HSE clk */
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY)) {	}

	/* Set HSE as system clock */
	RCC->CFGR |= RCC_CFGR_SW_HSE;
	while (!(RCC->CFGR & RCC_CFGR_SWS_HSE)) {	}

	/**** Every clock is now 8MHZ ****/
	RCC->CFGR &= ~(0b11<<RCC_CFGR_MCO2_Pos);	//Sysclk (8MHZ)
	RCC->CFGR |= (0b110<<RCC_CFGR_MCO2PRE_Pos);	//MCO2 Prescaler 4	(MCO2->2MHZ)
}

static void init_systick(void)
{
	SysTick_Config(F_CPU/1000);		// every ms	//10MHz???? wtf
}

/* Systick handler runs every ms */
uint32_t kanIkTellen = 0;
void SysTick_Handler(void)
{
	
//	gpio_toggle(GPIOD, GPIO_15);
	if(kanIkTellen == 0)
	{
		kanIkTellen = 10;
//		gpio_toggle(GPIOD, GPIO_13);
	}
	kanIkTellen--;
	gSysticks++;
}
