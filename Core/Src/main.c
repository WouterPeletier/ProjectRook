 #include <stdint.h>
#include <stdio.h>
#include <stm32f4xx.h>
#include <stdbool.h>

#include "platform.h"
#include "gpio.h"
#include "pwm.h"
#include "interrupts.h"
#include "user_interface.h"
#include "IR.h"
#include "fonts.h"
#include "ssd1306.h"

#ifdef SEMIHOSTING
   extern void initialise_monitor_handles(void);
#endif

#ifdef DEBUG
 #define DEBUGLOG(...) printf(__VA_ARGS__)
#else
 #define DEBUGLOG(...)
#endif


#define Receiving
int32_t Address = 1; //Address tussen 0 en 32
int32_t PDLC_intensity = 0;
bool transmit = true;

IRMode IRSendReceive = Send;
IRPacket * IRMsg = {0};
uint32_t bitCycles = 4800; //De tijd van de bit in clock cycles = 600us/(1/F_CPU (8000000))
uint32_t sendIntervalms = 1000; //De tijd in ms hoelang het duurt tot de volgende IR send command


uint16_t receivedIR;
bool receiveFlag = false;
bool sendFlag = false;


void beacon_main(void);
void bril_main(void);

int main(void){

    #ifdef SEMIHOSTING
        initialise_monitor_handles();
    #endif  

  
    init_platform();
    //DEBUGLOG("Platform Initiated\r\n");

    beacon_main();
}

void bril_main(void)
{
    DEBUGLOG("Running Bril Code");
	init_gpio(GPIOB, GPIO_2, GPIO_MODER_INPUT, GPIO_ALTFUNC_0, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);

    RCC->AHB1ENR = RCC_AHB1ENR_GPIODEN;
    // Set all LEDs output
    GPIOD->MODER |= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER11_0 | GPIO_MODER_MODER9_0;

    // Turn all LEDs Off
    GPIOD->ODR &= ~(GPIO_ODR_OD12 | GPIO_ODR_OD14 | GPIO_ODR_OD11 | GPIO_ODR_OD13 | GPIO_ODR_OD9);
    init_gpio(GPIOA, GPIO_4, GPIO_MODER_OUTPUT, GPIO_ALTFUNC_0, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);
    gpio_write(GPIOA, GPIO_4, 1);
    init_gpio(GPIOA, GPIO_0, GPIO_MODER_OUTPUT, GPIO_ALTFUNC_0, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);
    gpio_write(GPIOA, GPIO_0, 1);
    initTimPDLC2(50, 100);
    receive();

    // uint8_t PD13 = 0;
    while(1)
    {
        __WFI();

    }
}

void beacon_main(void) {
	//DEBUGLOG("Running Beacon Code");
	IRMsg->IRAddress = Address;

	//DEBUGLOG("initializing IR LED GPIO");
	init_gpio(GPIOA, GPIO_0, GPIO_MODER_ALT, GPIO_ALTFUNC_0, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);

	//DEBUGLOG("initializing UI button GPIO's");
	//init_gpio(inputPort, ROTA_pin, GPIO_MODER_INPUT, GPIO_ALTFUNC_0, GPIO_OTYPER_PUSHPULL, GPIO_PULL_DOWN, GPIO_OSPEEDR_LOW);
	set_interrupt(inputPort, ROTA_pin, INT_RISING_EDGE, GPIO_PULL_DOWN);
	set_interrupt(inputPort, PB_pin, INT_RISING_EDGE, GPIO_PULL_DOWN);
	set_interrupt(inputPort, ROTB_pin, INT_RISING_EDGE, GPIO_PULL_DOWN);

	//DEBUGLOG("initializing I2C GPIO's");
	init_gpio(GPIOB, GPIO_6, GPIO_MODER_ALT, GPIO_ALTFUNC_4, GPIO_OTYPER_OPENDRAIN, GPIO_PULL_UP, GPIO_OSPEEDR_HIGH); // scl
	init_gpio(GPIOB, GPIO_7, GPIO_MODER_ALT, GPIO_ALTFUNC_4, GPIO_OTYPER_OPENDRAIN, GPIO_PULL_UP, GPIO_OSPEEDR_HIGH); // sda

	//DEBUGLOG("initializing I2C and display driver");
	SSD1306_Init();
	init_settings();

	gpio_toggle(GPIOA, GPIO_0);
	initTIMIRS(50, 38000);
	IRInit(0b00110, Send);

	while(1){
		if (transmit)
		{
			for(volatile uint32_t j = 0; j<400000; j++);
			IRSend(((Address << 4) & 0b11110000) | (PDLC_intensity & 0b00001111));
		}
		else
		{
			__WFI();
		}
	}
        //
}

