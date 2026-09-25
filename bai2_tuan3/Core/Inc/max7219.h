#ifndef __MAX7219_H
#define __MAX7219_H

#include <stdint.h>

#define PERIPH_BASE       0x40000000UL
#define APB2PERIPH_BASE   (PERIPH_BASE + 0x00010000UL)
#define AHBPERIPH_BASE    (PERIPH_BASE + 0x00020000UL)

#define GPIOA_BASE        (APB2PERIPH_BASE + 0x0800UL)
#define GPIOC_BASE        (APB2PERIPH_BASE + 0x1000UL)
#define SPI1_BASE         (APB2PERIPH_BASE + 0x3000UL)
#define RCC_BASE          (AHBPERIPH_BASE  + 0x1000UL)
#define SYSTICK_BASE      0xE000E010UL

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

/* Thanh ghi SPI cua F1 rong 16 bit, moi thanh ghi cach nhau 4 byte */
typedef struct {
    volatile uint16_t CR1;      uint16_t RESERVED0;
    volatile uint16_t CR2;      uint16_t RESERVED1;
    volatile uint16_t SR;       uint16_t RESERVED2;
    volatile uint16_t DR;       uint16_t RESERVED3;
    volatile uint16_t CRCPR;    uint16_t RESERVED4;
    volatile uint16_t RXCRCR;   uint16_t RESERVED5;
    volatile uint16_t TXCRCR;   uint16_t RESERVED6;
    volatile uint16_t I2SCFGR;  uint16_t RESERVED7;
    volatile uint16_t I2SPR;    uint16_t RESERVED8;
} SPI_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_TypeDef;

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

#define GPIOA    ((GPIO_TypeDef *)     GPIOA_BASE)
#define GPIOC    ((GPIO_TypeDef *)     GPIOC_BASE)
#define SPI1     ((SPI_TypeDef *)      SPI1_BASE)
#define RCC      ((RCC_TypeDef *)      RCC_BASE)
#define SYSTICK  ((SysTick_TypeDef *)  SYSTICK_BASE)

#define RCC_APB2ENR_IOPAEN   (1U << 2)
#define RCC_APB2ENR_IOPCEN   (1U << 4)
#define RCC_APB2ENR_SPI1EN   (1U << 12)

#define SPI_CR1_MSTR         (1U << 2)
#define SPI_CR1_BR_DIV16     (3U << 3)
#define SPI_CR1_SPE          (1U << 6)
#define SPI_CR1_SSI          (1U << 8)
#define SPI_CR1_SSM          (1U << 9)

#define SPI_SR_RXNE          (1U << 0)
#define SPI_SR_TXE           (1U << 1)
#define SPI_SR_BSY           (1U << 7)

#define MAX7219_REG_NOOP         0x00
#define MAX7219_REG_DIGIT0       0x01
#define MAX7219_REG_DIGIT1       0x02
#define MAX7219_REG_DIGIT2       0x03
#define MAX7219_REG_DIGIT3       0x04
#define MAX7219_REG_DIGIT4       0x05
#define MAX7219_REG_DIGIT5       0x06
#define MAX7219_REG_DIGIT6       0x07
#define MAX7219_REG_DIGIT7       0x08
#define MAX7219_REG_DECODEMODE   0x09
#define MAX7219_REG_INTENSITY    0x0A
#define MAX7219_REG_SCANLIMIT    0x0B
#define MAX7219_REG_SHUTDOWN     0x0C
#define MAX7219_REG_DISPLAYTEST  0x0F

#define MAX7219_BLANK            0x0F

void SPI1_Init(void);
void MAX7219_WriteReg(uint8_t reg, uint8_t data);
void MAX7219_Init(void);
void MAX7219_Clear(void);
void MAX7219_DisplayTest(uint8_t on);
void MAX7219_DisplayDigit(uint8_t digit);
void MAX7219_DisplayNumber(uint32_t number);

#endif /* __MAX7219_H */
