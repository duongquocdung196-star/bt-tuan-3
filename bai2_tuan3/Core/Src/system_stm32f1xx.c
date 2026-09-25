#include "main.h"

/* Khong cau hinh PLL: chip chay bang HSI 8 MHz sau reset. */
uint32_t SystemCoreClock = 8000000UL;

void SystemInit(void) {
    /* Giu clock mac dinh sau reset (HSI 8 MHz) */
}
