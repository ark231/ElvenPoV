#include <iso646.h>
#include <simple.h>
#include <stddef.h>
#include <stdint.h>

#include "ch32v003fun.h"

#define LED1  PC1
#define SW    PC2
#define SER   PA1
#define SRCLK PA2
#define RCLK  PC4
/* #define LED2  PD1 */

#define LOOP_FREQ     100
#define SEC_IN_US     1'000'000
#define LOOP_DURATION Ticks_from_Us(SEC_IN_US / LOOP_FREQ)

#define REFRESH_RATE (16 / 2)

#include "data/kinga.h"

void shift_out16(uint8_t data_pin, uint8_t clk_pin, uint8_t flags, uint16_t value) {
    uint8_t upper = (value & 0xff00) >> 8;
    uint8_t lower = (value & 0x00ff);
    if (flags & SHIFT_LSBFIRST) {  // non zero value is truthy
        shift_out(data_pin, clk_pin, flags, lower);
        shift_out(data_pin, clk_pin, flags, upper);
    } else {
        shift_out(data_pin, clk_pin, flags, upper);
        shift_out(data_pin, clk_pin, flags, lower);
    }
}

// mask for the CCxP bits
// when set PWM outputs are held HIGH by default and pulled LOW
// when zero PWM outputs are held LOW by default and pulled HIGH
#define TIM2_DEFAULT 0xff
// #define TIM2_DEFAULT 0x00

void t2pwm_init() {
    // remap TIM2: PC1 is CH4
    AFIO->PCFR1 &= ~(0b11 << 8);
    AFIO->PCFR1 |= 0b01 << 8;

    // enable TIM2
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

    // https://github.com/cnlohr/ch32v003fun/blob/master/examples/tim2_pwm/tim2_pwm.c
    //  Reset TIM2 to init all regs
    RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

    // SMCFGR: default clk input is CK_INT
    // set TIM2 clock prescaler divider
    TIM2->PSC = 0x0000;
    // set PWM total cycle width
    TIM2->ATRLR = 255;

    TIM2->CHCTLR2 |= TIM_OC4M_2 | TIM_OC4M_1 | TIM_OC4PE;

    // CTLR1: default is up, events generated, edge align
    // enable auto-reload of preload
    TIM2->CTLR1 |= TIM_ARPE;

    // Enable Channel outputs, set default state (based on TIM2_DEFAULT)
    TIM2->CCER |= TIM_CC4E | (TIM_CC4P & TIM2_DEFAULT);

    // initialize counter
    TIM2->SWEVGR |= TIM_UG;

    // Enable TIM2
    TIM2->CTLR1 |= TIM_CEN;
}

/*
 * set timer channel PW
 */
void t2pwm_setpw(uint8_t chl, uint16_t width) {
    switch (chl) {
        case 1:
            TIM2->CH1CVR = width;
            break;
        case 2:
            TIM2->CH2CVR = width;
            break;
        case 3:
            TIM2->CH3CVR = width;
            break;
        case 4:
            TIM2->CH4CVR = width;
            break;
    }
    // TIM2->SWEVGR |= TIM_UG; // load new value in compare capture register
}

int main(void) {
    SystemInit();

    // Enable GPIOs
    funGpioInitAll();
    funAnalogInit();

    /* funPinMode(LED1, GPIO_CNF_OUT_PP_AF); */
    /* t2pwm_init(); */

    funPinMode(LED1, FUN_OUTPUT);

    funPinMode(SW, GPIO_CFGLR_IN_PUPD);
    funPinMode(SER, FUN_OUTPUT);
    funPinMode(SRCLK, FUN_OUTPUT);
    funPinMode(RCLK, FUN_OUTPUT);

    funDigitalWrite(SW, FUN_HIGH);

    uint32_t count    = 0;
    uint32_t cur_time = SysTick->CNT;

    uint16_t value = 1;
    for (int i = 0; i < 16; i++) {
        funDigitalWrite(RCLK, FUN_LOW);
        shift_out16(SER, SRCLK, SHIFT_LSBFIRST, value << i);
        funDigitalWrite(RCLK, FUN_HIGH);
        Delay_Ms(125);
    }
    funDigitalWrite(RCLK, FUN_LOW);
    shift_out16(SER, SRCLK, SHIFT_LSBFIRST, 0);
    funDigitalWrite(RCLK, FUN_HIGH);
    value = 0;

    /* funPinMode(LED2, FUN_OUTPUT); */

    bool drawing_line = false;
    bool drawing_text = false;

    size_t icol        = 0;
    const size_t ncol  = sizeof(bitmap) / sizeof(bitmap[0]);
    size_t iline       = 0;
    const size_t nline = sizeof(line_widths) / sizeof(line_widths[0]);

    while (1) {
        /* if (count % LOOP_FREQ == 0) { */
        /*     t2pwm_setpw(4, 255); */
        /* } else if (count % LOOP_FREQ == LOOP_FREQ / 2) { */
        /*     if (drawing_line) { */
        /*         t2pwm_setpw(4, 0); */
        /*     } else { */
        /*         t2pwm_setpw(4, 128); */
        /*     } */
        /* } */
        if (count % LOOP_FREQ == 0) {
            funDigitalWrite(LED1, FUN_HIGH);
        } else if ((not drawing_text) && (count % LOOP_FREQ == (LOOP_FREQ * 2) / 4)) {
            funDigitalWrite(LED1, FUN_LOW);
        } else if (drawing_text && (drawing_line) && (count % LOOP_FREQ == LOOP_FREQ / 4)) {
            funDigitalWrite(LED1, FUN_LOW);
        } else if (drawing_text && (not drawing_line) && (count % LOOP_FREQ == (LOOP_FREQ * 3) / 4)) {
            funDigitalWrite(LED1, FUN_LOW);
        }

        if (funDigitalRead(SW) == FUN_LOW) {
            drawing_line = true;
            drawing_text = true;
        }
        if (drawing_line) {
            if (count % (LOOP_FREQ / REFRESH_RATE) == 0) {
                funDigitalWrite(RCLK, FUN_LOW);
                shift_out16(SER, SRCLK, SHIFT_LSBFIRST, bitmap[icol]);
                funDigitalWrite(RCLK, FUN_HIGH);

                icol++;

                if (icol == line_ends[iline]) {
                    drawing_line = false;
                    iline++;
                }
                if (icol == ncol && iline == nline) {
                    icol = iline = 0;
                    drawing_line = false;
                    drawing_text = false;
                }

            } else if (count % (LOOP_FREQ / REFRESH_RATE) == (LOOP_FREQ / REFRESH_RATE) / 2) {
            }
        }

        while ((SysTick->CNT - cur_time) < LOOP_DURATION) {
        }
        cur_time = SysTick->CNT;
        count++;
    }
}
