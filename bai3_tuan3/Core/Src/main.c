#include "main.h"
#include <stdio.h>
#include <stdint.h>

#define CLASS_ID    "ID-HE THONG NHUNG-B23DCDT066"
#define GROUP_ID    "ID-Nhom6"

#define IDLE_MS     10000   // Không nhấn reset trong khoảng này thì lần sau về 1

#define RCC_APB1ENR   (*(volatile uint32_t *)(0x40021000 + 0x1C))
#define RCC_APB2ENR   (*(volatile uint32_t *)(0x40021000 + 0x18))
#define RCC_AHBENR    (*(volatile uint32_t *)(0x40021000 + 0x14))

#define GPIOA_CRH     (*(volatile uint32_t *)(0x40010800 + 0x04))

#define USART1_DR     (*(volatile uint32_t *)(0x40013800 + 0x04))
#define USART1_BRR    (*(volatile uint32_t *)(0x40013800 + 0x08))
#define USART1_CR1    (*(volatile uint32_t *)(0x40013800 + 0x0C))
#define USART1_CR3    (*(volatile uint32_t *)(0x40013800 + 0x14))

#define PWR_CR        (*(volatile uint32_t *)(0x40007000 + 0x00))
#define BKP_DR1       (*(volatile uint32_t *)(0x40006C00 + 0x04))

#define DMA1_CCR4     (*(volatile uint32_t *)(0x40020000 + 0x44))
#define DMA1_CNDTR4   (*(volatile uint32_t *)(0x40020000 + 0x48))
#define DMA1_CPAR4    (*(volatile uint32_t *)(0x40020000 + 0x4C))
#define DMA1_CMAR4    (*(volatile uint32_t *)(0x40020000 + 0x50))

#define STK_CTRL      (*(volatile uint32_t *)(0xE000E010))
#define STK_LOAD      (*(volatile uint32_t *)(0xE000E014))
#define STK_VAL       (*(volatile uint32_t *)(0xE000E018))

char tx_buffer[100];

void System_Init(void) {
    // Clock: GPIOA, USART1 (APB2); PWR, BKP (APB1); DMA1 (AHB)
    RCC_APB2ENR |= (1 << 2) | (1 << 14);
    RCC_APB1ENR |= (1 << 28) | (1 << 27);
    RCC_AHBENR  |= (1 << 0);

    // PA9: USART1_TX, AF push-pull 50MHz
    GPIOA_CRH &= ~(0xF << 4);
    GPIOA_CRH |=  (0xB << 4);

    // USART1: 9600 baud @ 8MHz, TX + DMA
    USART1_BRR = 0x0341;
    USART1_CR1 = (1 << 3) | (1 << 13);
    USART1_CR3 |= (1 << 7);

    // Cho phép ghi vùng backup
    PWR_CR |= (1 << 8);

    // SysTick 1ms (8MHz), polling
    STK_LOAD = 8000 - 1;
    STK_VAL  = 0;
    STK_CTRL = (1 << 2) | (1 << 0);
}

void delay_ms(uint32_t ms) {
    while (ms--) {
        while (!(STK_CTRL & (1u << 16)));
    }
}

void DMA1_Send_Buffer(char *buf, uint16_t len) {
    DMA1_CCR4 &= ~(1 << 0);
    DMA1_CPAR4  = (uint32_t)&USART1_DR;
    DMA1_CMAR4  = (uint32_t)buf;
    DMA1_CNDTR4 = len;
    DMA1_CCR4   = (1 << 7) | (1 << 4) | (1 << 0);
}

int main(void) {
    System_Init();

    uint16_t count = (uint16_t)BKP_DR1;
    if (count > 50) count = 0;      // giá trị rác -> bắt đầu lại

    count++;
    BKP_DR1 = count;

    int len = sprintf(tx_buffer, "%s_%s:BTN:%u\r\n", CLASS_ID, GROUP_ID, (unsigned)count);
    DMA1_Send_Buffer(tx_buffer, (uint16_t)len);

    // Chờ: nếu không có reset nào trong IDLE_MS thì xóa để lần sau về 1
    delay_ms(IDLE_MS);
    BKP_DR1 = 0;

    while (1) {
    }
}
