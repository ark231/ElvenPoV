#include <simple.h>
#include <stdint.h>
#include <stdio.h>

#include "ch32v003fun.h"

#define LED1  PC1
#define LED2  PC2
#define SER   PA1
#define SRCLK PA2
#define RCLK  PC4

#define LOOP_FREQ     100
#define SEC_IN_US     1'000'000
#define LOOP_DURATION Ticks_from_Us(SEC_IN_US / LOOP_FREQ)

#define COUNT_FREQ 4

int main(void) {
    SystemInit();

    // Enable GPIOs
    funGpioInitAll();

    funPinMode(LED1, FUN_OUTPUT);
    funPinMode(LED2, FUN_OUTPUT);
    funPinMode(SER, FUN_OUTPUT);
    funPinMode(SRCLK, FUN_OUTPUT);
    funPinMode(RCLK, FUN_OUTPUT);

    uint32_t count    = 0;
    uint32_t cur_time = SysTick->CNT;

    uint8_t value = 1;
    for (int i = 0; i < 8; i++) {
        funDigitalWrite(RCLK, FUN_LOW);
        shift_out(SER, SRCLK, SHIFT_LSBFIRST, value << i);
        funDigitalWrite(RCLK, FUN_HIGH);
        Delay_Ms(1000);
    }
    value = 0;

    while (1) {
        if (count % LOOP_FREQ == 0) {
            funDigitalWrite(LED1, FUN_HIGH);
        } else if (count % LOOP_FREQ == LOOP_FREQ / 2) {
            funDigitalWrite(LED1, FUN_LOW);
        }

        if (count % (LOOP_FREQ / COUNT_FREQ) == 0) {
            funDigitalWrite(LED2, FUN_HIGH);

            funDigitalWrite(RCLK, FUN_LOW);
            shift_out(SER, SRCLK, SHIFT_LSBFIRST, value++);
            funDigitalWrite(RCLK, FUN_HIGH);
        } else if (count % (LOOP_FREQ / COUNT_FREQ) == (LOOP_FREQ / COUNT_FREQ) / 2) {
            funDigitalWrite(LED2, FUN_LOW);
        }

        while ((SysTick->CNT - cur_time) < LOOP_DURATION) {
        }
        cur_time = SysTick->CNT;
        count++;
    }
}
