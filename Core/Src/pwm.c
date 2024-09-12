#include <stdint.h>
#include <stdio.h>
#include "platform.h"
#include "gpio.h"

void initTimPDLC2(uint16_t dutycycle, uint32_t frequency)
{
    //pa3 timer 5 op bepaalde frequentie
    //vervolgens pwm op timer 2 

    uint16_t prescal = 0;
    uint16_t autoreload = (F_CPU/(2*frequency*(prescal+1)))-1;
    uint16_t ccr = (dutycycle*autoreload)/100;

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = prescal;
    TIM2->ARR = autoreload/10;  //Higher frequency
    TIM2->CCR4 = ccr/10;


    /* center aligned, mode 3: Output compare interrupt flags set when counting up and down */
    TIM2->CR1 |= (3<<TIM_CR1_CMS_Pos);
    /* Output 4 is active when CNT<CCR1 */
    TIM2->CCMR2 |= (0b110<<TIM_CCMR2_OC4M_Pos);

    init_gpio(GPIOA, GPIO_3, GPIO_MODER_ALT, GPIO_ALTFUNC_1, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);
    /* Connect timer output to gpio output*/
    TIM2->CCER |= TIM_CCER_CC4E;
    /* Enable timer */
    TIM2->CR1 |= TIM_CR1_CEN;

    /***********/
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
    TIM5->PSC = prescal;
    TIM5->ARR = autoreload;
    TIM5->CCR3 = ccr;
    TIM5->CCR2 = ccr;

    /* center aligned, mode 3: Output compare interrupt flags set when counting up and down */
    TIM5->CR1 |= (3<<TIM_CR1_CMS_Pos);
    /* Output 3 is active when CNT<CCR1 */
    TIM5->CCMR2 |= (0b110<<TIM_CCMR2_OC3M_Pos);
    TIM5->CCMR1 |= (0b111<<TIM_CCMR1_OC2M_Pos);

    init_gpio(GPIOA, GPIO_2, GPIO_MODER_ALT, GPIO_ALTFUNC_2, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);
    init_gpio(GPIOA, GPIO_1, GPIO_MODER_ALT, GPIO_ALTFUNC_2, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);

    /* Connect timer output to gpio output*/
    TIM5->CCER |= (TIM_CCER_CC2E | TIM_CCER_CC3E);
    /* Enable timer */
    TIM5->CR1 |= TIM_CR1_CEN;

}

void initTIMPDLC(uint16_t dutycycle, uint32_t frequency) {
    if(dutycycle > 100) dutycycle = 50;
    
    uint16_t prescal = 0;
    uint16_t autoreload = (F_CPU/(2*frequency*(prescal+1)))-1;
    uint16_t ccr1 = (dutycycle*autoreload)/100;
    uint16_t ccr2 = autoreload - ccr1;
    

    #ifdef SEMIHOSTING
    printf("initialising PWM\r\n");
    printf("Requested values\r\n");
    printf("dutycycle: %d\t frequency: %d\r\n", dutycycle, frequency);
    printf("\nActual\r\n");
    printf("dutycycle1: %d\tdutycycle2: %d\t frequency: %d Hz\r\n", ((ccr1*100)/autoreload), ((ccr2*100)/autoreload), ((F_CPU)/((autoreload+1)*(prescal+1))) );

    printf("Debug: PSC: %d\tARR: %d\tCCR1: %d\tCCR2: %d\r\n", prescal, autoreload, ccr1, ccr2);

    #endif
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->PSC = prescal;
    TIM3->ARR = autoreload;
    TIM3->CCR1 = ccr1;
    TIM3->CCR2 = ccr2;

    /* center aligned, mode 3: Output compare interrupt flags set when counting up and down */
    TIM3->CR1 |= (3<<TIM_CR1_CMS_Pos);
    /* Output 1 is active when CNT<CCR1 */
    TIM3->CCMR1 |= (0b110<<TIM_CCMR1_OC1M_Pos);
    /* Output 2 is active when CNT>CCR2 */
    TIM3->CCMR1 |= (0b111<<TIM_CCMR1_OC2M_Pos);

    /* Tim3_CH1 PA6 */
    /* Tim3_CH2 PA7 */

    /* enable GPIO outputs */
    init_gpio(GPIOA, GPIO_6, GPIO_MODER_ALT, GPIO_ALTFUNC_2, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);
    init_gpio(GPIOA, GPIO_7, GPIO_MODER_ALT, GPIO_ALTFUNC_2, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);

    /* invert 1 of the timer outputs */
    TIM3->CCER |= TIM_CCER_CC2P;
    TIM3->CCER |= TIM_CCER_CC1P;
    /* Connect timer output to gpio output*/
    TIM3->CCER |= (TIM_CCER_CC2E | TIM_CCER_CC1E);
    /* Enable timer */
    TIM3->CR1 |= TIM_CR1_CEN;
}

void initTIMIRR(uint16_t dutycycle, uint32_t frequency) {
    if(dutycycle > 100) dutycycle = 50;
    //periodetijd tussen 200us en 1000us
    // if(frequency > 5000) frequency = 5000;
    // if(frequency < 1000) frequency = 1000;
    uint32_t prescal = 0;
    uint32_t autoreload = (F_CPU/(frequency*(prescal+1)))-1;
    uint32_t ccr1 = (dutycycle*autoreload)/100;
    uint32_t ccr2 = autoreload - ccr1;

    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    TIM4->PSC = prescal;
    TIM4->ARR = autoreload;
    TIM4->CCR1 = ccr1;
    TIM4->CCR2 = ccr2;

    TIM4->CR1 |= (0<<TIM_CR1_CMS_Pos) | (0<<TIM_CR1_DIR_Pos);

    TIM4->CCMR1 |= (0b010<<TIM_CCMR1_OC1M_Pos); //CCMR1 register OC2M op 010 zodat channel 2 werkt op CCR1

    TIM4->CCER |= TIM_CCER_CC1P;

    // TIM4->DIER |= TIM_DIER_UIE;

    TIM4->CR1 &= ~TIM_CR1_CEN;
}

void initTIMIRGeneric(uint16_t dutycycle, uint32_t frequency) {
    if(dutycycle > 100) dutycycle = 50;
    //periodetijd tussen 200us en 1000us
    if(frequency > 5000) frequency = 5000;
    if(frequency < 1000) frequency = 1000;
    uint32_t prescal = 0;
    uint32_t autoreload = (F_CPU/(frequency*(prescal+1)))-1;
    uint32_t ccr1 = (dutycycle*autoreload)/100;
    uint32_t ccr2 = autoreload - ccr1;

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = prescal;
    TIM2->ARR = autoreload;
    TIM2->CCR1 = ccr1;
    TIM2->CCR2 = ccr2;
    TIM2->CR1 |= (0<<TIM_CR1_CMS_Pos) | (1<<TIM_CR1_DIR_Pos);
    TIM2->CCMR1 |= (0b010<<TIM_CCMR1_OC1M_Pos); //CCMR1 register OC2M op 010 zodat channel 2 werkt op CCR1
    TIM2->CCER |= TIM_CCER_CC1P;
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1 &= ~(TIM_CR1_CEN);
}

void initTIMIRS(uint16_t dutycycle, uint32_t frequency) {
    if(dutycycle > 100) dutycycle = 50;
    //periodetijd tussen 200us en 1000us
    if(frequency > 50000) frequency = 50000;
    if(frequency < 20000) frequency = 20000;
    uint32_t prescal = 0;
    uint32_t autoreload = (F_CPU/(frequency/(prescal+1)))-1;
    uint32_t ccr2 = (dutycycle*autoreload)/100;

    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
    TIM5->PSC = prescal;
    TIM5->ARR = autoreload;
    TIM5->CCR2 = ccr2;
    TIM5->CR1 |= (0<<TIM_CR1_CMS_Pos) | (1<<TIM_CR1_DIR_Pos);
    TIM5->CCMR1 |= (0b110<<TIM_CCMR1_OC2M_Pos); //CCMR1 register OC2M op 010 zodat channel 2 werkt op CCR1
    init_gpio(GPIOA, GPIO_1, GPIO_MODER_ALT, GPIO_ALTFUNC_2, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);
    TIM5->CCER |= TIM_CCER_CC2P | TIM_CCER_CC2E;
    TIM5->CR1 &= ~(TIM_CR1_CEN);
}

void update_dutycycle(TIM_TypeDef* TIMx, uint32_t dutycycle) {
    uint32_t autoreload = TIMx->ARR;
    uint16_t ccr1 = (dutycycle*autoreload)/100;
    uint16_t ccr2 = autoreload - ccr1;

    TIMx->CCR1 = ccr1;
    TIMx->CCR2 = ccr2;
}

void togglePWM(TIM_TypeDef* TIMx) {
    /* Enable timer */
    TIMx->CR1 ^= TIM_CR1_CEN;
}

void switchPWM(TIM_TypeDef* TIMx, uint8_t HL) {
    if(HL == 0) {
        TIMx->CR1 &= ~TIM_CR1_CEN;
    } else if(HL == 1) {
        TIMx->CR1 |= TIM_CR1_CEN;
    }
}

void initPWM(GPIO_TypeDef* Port, uint8_t pin, TIM_TypeDef* TIMx) {
    /* Initialize GPIO with correct function*/
    init_gpio(Port, pin, GPIO_MODER_ALT, GPIO_ALTFUNC_2, GPIO_OTYPER_PUSHPULL, GPIO_PULL_NONE, GPIO_OSPEEDR_HIGH);
    /* Connect timer output to gpio output*/

    TIMx->CCER |= (TIM_CCER_CC2E | TIM_CCER_CC1E);
}
