#pragma once
#include <stdint.h>
#include <stm32f4xx.h>

#define INT_RISING_EDGE     (0b01)
#define INT_FALLING_EDGE    (0b10)
#define INT_ALL_EDGE        (0b11)

void set_interrupt(GPIO_TypeDef* port, uint8_t pin, uint8_t edgeRate, uint8_t pull);
