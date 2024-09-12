#include "interrupts.h"
#include <stdint.h>
#include <stdio.h>
#include <stm32f4xx.h>
#include "gpio.h"
#include "pwm.h"
#include <stdbool.h>

#include "user_interface.h"

#define messageLength 12
#define DEBUG

#ifdef DEBUG
 #define DEBUGLOG(...) printf(__VA_ARGS__)
#else
 #define DEBUGLOG(...)
#endif

bool start = true;
uint8_t bitIndex = 0;
uint16_t lowDuration;
uint8_t message[messageLength];
uint8_t msgPart = 1;
uint8_t msgFlag = 0;
uint8_t oldPD0 = 0;

extern uint16_t receivedIR;
extern bool receiveFlag;

void set_interrupt(GPIO_TypeDef* port, uint8_t pin, uint8_t edgeRate, uint8_t pull)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    //getest met mode output??
    init_gpio(port, pin, GPIO_MODER_INPUT, GPIO_ALTFUNC_0, GPIO_OTYPER_PUSHPULL, pull, GPIO_OSPEEDR_HIGH);
    GPIOD->MODER &= ~(GPIO_MODER_MODE0);  // Clear bits
    GPIOD->PUPDR &= ~(GPIO_PUPDR_PUPD0);  // No pull-up, no pull-down


    uint8_t reg = pin >> 2;
    //SYSCFG->EXTICR[reg] &= ~(SYSCFG_EXTICR1_EXTI0);  // Clear EXTI selection
    // SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PD;

    if(port == GPIOA) // Only works for pin 0
    {
        SYSCFG->EXTICR[reg] |= SYSCFG_EXTICR1_EXTI0_PA << (4 * (pin % 4));
//        DEBUGLOG("EXTI line 0 set to use port A\r\n");
    }else if(port == GPIOB)
    {
        SYSCFG->EXTICR[reg] |= SYSCFG_EXTICR1_EXTI0_PB << (4 * (pin % 4));
        //DEBUGLOG("EXTI line 0 set to use port B\r\n");
    }else if(port == GPIOC)
    {
        SYSCFG->EXTICR[reg] |= SYSCFG_EXTICR1_EXTI0_PC << (4 * (pin % 4));
//        DEBUGLOG("EXTI line 0 set to use port C\r\n");
    }else if(port == GPIOD)
    {
        SYSCFG->EXTICR[reg] |= SYSCFG_EXTICR1_EXTI0_PD << (4 * (pin % 4));
//        DEBUGLOG("EXTI line 0 set to use port D\r\n");
    }

    EXTI->IMR |= 1<<pin;

    //trigger selection EXTI_RTSR and EXTI_FTSR
    EXTI->RTSR |= (!!(edgeRate & 0b01))<<pin;  //rising edge
    EXTI->FTSR |= (!!(edgeRate & 0b10))<<pin; //falling

    //EXTI->PR |= 0xFFFF;

    //set IRQ priority in NVIC
	//Enable interrupt in NVIC
    if (pin == 0)
    {
        NVIC_SetPriority(EXTI0_IRQn, 0);
        NVIC_EnableIRQ(EXTI0_IRQn);
    }
    else if (pin == 1)
    {
    	NVIC_SetPriority(EXTI1_IRQn, 0);
    	NVIC_EnableIRQ(EXTI1_IRQn);
    }
    else if (pin == 2)
    {
    	NVIC_SetPriority(EXTI2_IRQn, 0);
    	NVIC_EnableIRQ(EXTI2_IRQn);
    }
    else if (pin == 3)
    {
    	NVIC_SetPriority(EXTI3_IRQn, 0);
    	NVIC_EnableIRQ(EXTI3_IRQn);
    }
    else if (pin == 4)
    {
    	NVIC_SetPriority(EXTI4_IRQn, 0);
    	NVIC_EnableIRQ(EXTI4_IRQn);
    }
    else if ((10 > pin) & (pin > 4))
    {
    	NVIC_SetPriority(EXTI9_5_IRQn, 0);
    	NVIC_EnableIRQ(EXTI9_5_IRQn);
    }
    else if ((16 > pin) & (pin > 9))
    {
    	NVIC_SetPriority(EXTI15_10_IRQn, 0);
    	NVIC_EnableIRQ(EXTI15_10_IRQn);
    }

    EXTI->PR |= (0x01 << pin);
}



void EXTI9_5_IRQHandler(void)
{
	UI_interrupt();
}

void EXTI2_IRQHandler(void)
{
    if(gpio_read(GPIOB, GPIO_2) == 0) {
        GPIOD->ODR &= ~GPIO_ODR_OD12;
    } else {
        GPIOD->ODR |= GPIO_ODR_OD12;
    }

    if(gpio_read(GPIOB, GPIO_2)== 1 ) {
        GPIOD->ODR &= ~GPIO_ODR_OD11;
    } else {
        GPIOD->ODR |= GPIO_ODR_OD11;
    }
    //DEBUGLOG("HANDLER\r\n");
    if (EXTI->PR & EXTI_PR_PR0) {
        GPIOD->ODR ^= GPIO_ODR_OD12;
        if(start)
        {
            if (gpio_read(GPIOD, GPIO_0) == 0)
            {
                GPIOD->ODR |= GPIO_ODR_OD14;
                //DEBUGLOG("Detected falling edge\r\n");
                TIM4->CNT = 0;  //Make sure CNT register is 0
                switchPWM(TIM4, 1); //Start timer TIM4
                start = false;  //Set flag to false
            }
        }else {
            //DEBUGLOG("Detected rising edge\r\n");
            switchPWM(TIM4, 0); //Stop timer
            lowDuration = TIM4->CNT; //Capture lowDuration 7112 = 889us, 4802 = 600 us
            if(lowDuration < 4000) {
                TIM4->CNT = 0;
                switchPWM(TIM4, 1); //Start timer
                EXTI->PR |= EXTI_PR_PR0;
            } else if(lowDuration > 10000) {
                TIM4->CNT = 0;
                switchPWM(TIM4, 1); //Start timer
                EXTI->PR |= EXTI_PR_PR0;
            } else {
            	if(6500 < lowDuration < 7600) { //lowDuration tussen 815us en 950us
            		lowDuration = 7112; //lowDuration is 889us
            	}
                TIM4->DIER |= TIM_DIER_UIE;  // Enable TIM4 update interrupt
                TIM4->ARR = lowDuration * 1.1;  //Auto reload is 3/4 bitTime
                //NVIC_DisableIRQ(EXTI0_IRQn);  //Disable this interrupt
                EXTI->IMR &= 0<<0; //Disable this interrupt
                NVIC_DisableIRQ(EXTI0_IRQn);
                start = true;  //Reset start flag
                GPIOD->ODR &= ~GPIO_ODR_OD14;
                NVIC_EnableIRQ(TIM4_IRQn);
                TIM4->CNT = 0;
                switchPWM(TIM4, 1); //Start timer TIM4
            }
        }
    }
    EXTI->PR |= EXTI_PR_PR2; // Clear the EXTI line 0 pending flag
}

void TIM4_IRQHandler(void) {
    //DEBUGLOG("Got to TIM4 interrupt\r\n");
    GPIOD->ODR |= GPIO_ODR_OD11;
    GPIOD->ODR |= GPIO_ODR_OD13;
    TIM4->ARR = lowDuration * 2;
    // if(msgPart == 1) {
    //     message[bitIndex] = gpio_read(GPIOD, GPIO_0);
    //     bitIndex++;
    //     msgPart++;
    // } else if(msgPart == 2) {
    //     if((GPIOD->IDR & GPIO_IDR_ID0) == oldPD0) {
    //         msgFlag++;
    //     }
    //     msgPart --;
    // }
    message[bitIndex] = 1-gpio_read(GPIOD, GPIO_0); //data inverteren
    bitIndex++;
    if(bitIndex == messageLength) 
    {
        bitIndex = 0;
        for(int i = 0; i<messageLength; i++)
        {
            receivedIR = receivedIR << 1;
            receivedIR |= message[i];
        }
        receiveFlag = true;
        switchPWM(TIM4, 0); //Stop timer
        GPIOD->ODR &= ~GPIO_ODR_OD13;
        EXTI->IMR |= 1<<0; //Enable EXTI interrupt
        NVIC_EnableIRQ(EXTI0_IRQn);
        TIM4->ARR = 65000;
        TIM4->DIER &= TIM_DIER_UIE;  //Disable this interrupt
    }
    GPIOD->ODR &= ~GPIO_ODR_OD11;
    TIM4->SR &= ~TIM_SR_UIF;
}
