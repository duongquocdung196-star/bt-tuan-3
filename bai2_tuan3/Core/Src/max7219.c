#include "max7219.h"

/* PA4 -> CS(LOAD), PA5 -> CLK, PA7 -> DIN. MAX7219 VCC = 5V, GND chung. */

void SPI1_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;

    /* PA4 (CS): output push-pull 50 MHz */
    GPIOA->CRL &= ~(0xFU << 16);
    GPIOA->CRL |=  (0x3U << 16);
    GPIOA->BSRR = (1U << 4);

    /* PA5 (SCK), PA7 (MOSI): AF push-pull 50 MHz */
    GPIOA->CRL &= ~((0xFU << 20) | (0xFU << 28));
    GPIOA->CRL |=  ((0xBU << 20) | (0xBU << 28));

    /* Master, mode 0, 8 bit, MSB first, NSS mem, PCLK2/16 */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV16 | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR1 |= SPI_CR1_SPE;
}

static uint8_t SPI1_Transmit(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)SPI1->DR;
}

void MAX7219_WriteReg(uint8_t reg, uint8_t data) {
    GPIOA->BSRR = (1U << (4 + 16));      /* CS LOW  */
    SPI1_Transmit(reg);
    SPI1_Transmit(data);
    while (SPI1->SR & SPI_SR_BSY);
    GPIOA->BSRR = (1U << 4);             /* CS HIGH */
}

void MAX7219_Clear(void) {
    for (uint8_t i = MAX7219_REG_DIGIT0; i <= MAX7219_REG_DIGIT7; i++) {
        MAX7219_WriteReg(i, MAX7219_BLANK);
    }
}

void MAX7219_DisplayTest(uint8_t on) {
    MAX7219_WriteReg(MAX7219_REG_DISPLAYTEST, on ? 0x01 : 0x00);
}

void MAX7219_Init(void) {
    SPI1_Init();

    MAX7219_WriteReg(MAX7219_REG_SHUTDOWN,    0x00);
    MAX7219_WriteReg(MAX7219_REG_DISPLAYTEST, 0x00);
    MAX7219_WriteReg(MAX7219_REG_SCANLIMIT,   0x07);
    MAX7219_WriteReg(MAX7219_REG_DECODEMODE,  0xFF);
    MAX7219_WriteReg(MAX7219_REG_INTENSITY,   0x07);
    MAX7219_Clear();
    MAX7219_WriteReg(MAX7219_REG_SHUTDOWN,    0x01);
}

void MAX7219_DisplayDigit(uint8_t digit) {
    if (digit > 9) return;
    MAX7219_WriteReg(MAX7219_REG_DIGIT0, digit);
}

void MAX7219_DisplayNumber(uint32_t number) {
    for (uint8_t pos = MAX7219_REG_DIGIT0; pos <= MAX7219_REG_DIGIT7; pos++) {
        if (number == 0 && pos != MAX7219_REG_DIGIT0) {
            MAX7219_WriteReg(pos, MAX7219_BLANK);
        } else {
            MAX7219_WriteReg(pos, (uint8_t)(number % 10));
            number /= 10;
        }
    }
}
