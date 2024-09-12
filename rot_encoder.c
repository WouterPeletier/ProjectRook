#include <stdint.h>
#include <stdio.h>
#include <stm32f4xx.h>

/* when rotating, the phase transitions will be 3 -> 2 -> 0 -> 1 -> 3
 * When rotating in the opposite direction this sequence will simply be reversed: 3 -> 1 -> 0 -> 2 ->
 */
volatile int rot_cnt = 0;

uint8_t rot_new_state, rot_old_state;

uint8_t rot_get_state() {
	uint8_t value = 0;
	value = (GPIOB->IDR & 0x01) | (GPIOB->IDR & 0x02);
	return value;
}

void Interrupt_EXTI_PB0_init(void);


void EXTI0_IRQHandler(void)
{
    if(EXTI->PR & EXTI_PR_PR0)
    {
		rot_new_state = rot_get_state();

		// Check transition
		if (rot_old_state == 3 && rot_new_state == 2) {        // 3 -> 2 transition
			rot_cnt++;
		} else if (rot_old_state == 2 && rot_new_state == 0) { // 2 -> 0 transition
			rot_cnt++;
		} else if (rot_old_state == 0 && rot_new_state == 1) { // 0 -> 1 transition
			rot_cnt++;
		} else if (rot_old_state == 1 && rot_new_state == 3) { // 1 -> 3 transition
			rot_cnt++;
		} else if (rot_old_state == 3 && rot_new_state == 1) { // 3 -> 1 transition
			rot_cnt--;
		} else if (rot_old_state == 1 && rot_new_state == 0) { // 1 -> 0 transition
			rot_cnt--;
		} else if (rot_old_state == 0 && rot_new_state == 2) { // 0 -> 2 transition
			rot_cnt--;
		} else if (rot_old_state == 2 && rot_new_state == 3) { // 2 -> 3 transition
			rot_cnt--;
		}

		rot_old_state = rot_new_state;
    }
    EXTI->PR |= EXTI_PR_PR0;
}

int main(void)
{
	// GPIO Port B Clock Enable
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	// GPIO Port B Pin 0 and 1 as input
	GPIOB->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1);
	/* Enable Pullup on PB0 en PB1 */
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR0);
	/* Clear PUPDR0 fields (No Pullup, No Pull down) */
	GPIOB->PUPDR =  0x03;
	/* Set PUPDR0 fields to 0x01 (Pullup) */
	Interrupt_EXTI_PB0_init();

	while (1) {}
}

void Interrupt_EXTI_PB0_init(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PB;
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PB;

    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0; //Rising trigger enabled
    EXTI->FTSR |= EXTI_FTSR_TR0;

    NVIC_SetPriority(EXTI0_IRQn, 0);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

