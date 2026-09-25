#include <stdint.h>
 
/* ---------------- Định nghĩa thanh ghi (không cần main.h / CMSIS) ---------------- */
#define CPU_HZ  8000000U   /* HSI 8 MHz mặc định sau reset. Nếu bạn cấu hình PLL thì đổi số này */
 
typedef struct { volatile uint32_t CTRL, LOAD, VAL, CALIB; } SysTick_Type;
typedef struct { volatile uint32_t CR, CFGR, CIR, APB2RSTR, APB1RSTR, AHBENR, APB2ENR, APB1ENR; } RCC_Type;
typedef struct { volatile uint32_t CRL, CRH, IDR, ODR, BSRR, BRR, LCKR; } GPIO_Type;
typedef struct { volatile uint32_t CR1, CR2, SR, DR; } SPI_Type;
 
#define SysTick  ((SysTick_Type *)0xE000E010UL)
#define RCC      ((RCC_Type *)0x40021000UL)
#define GPIOA    ((GPIO_Type *)0x40010800UL)
#define SPI1     ((SPI_Type *)0x40013000UL)
 
#define SysTick_CTRL_ENABLE_Msk      (1U << 0)
#define SysTick_CTRL_TICKINT_Msk     (1U << 1)
#define SysTick_CTRL_CLKSOURCE_Msk   (1U << 2)
 
#define RCC_APB2ENR_AFIOEN   (1U << 0)
#define RCC_APB2ENR_IOPAEN   (1U << 2)
#define RCC_APB2ENR_SPI1EN   (1U << 12)
 
#define SPI_CR1_MSTR   (1U << 2)
#define SPI_CR1_SPE    (1U << 6)
#define SPI_CR1_SSI    (1U << 8)
#define SPI_CR1_SSM    (1U << 9)
#define SPI_CR1_DFF    (1U << 11)
#define SPI_SR_TXE     (1U << 1)
#define SPI_SR_BSY     (1U << 7)
 
/* ---------------- Chân kết nối MAX7219 ----------------
 *  PA4 -> CS (LOAD)   GPIO output, điều khiển bằng phần mềm
 *  PA5 -> CLK         SPI1_SCK
 *  PA7 -> DIN         SPI1_MOSI
 */
#define CS_LOW()   (GPIOA->BSRR = (1U << (4U + 16U)))
#define CS_HIGH()  (GPIOA->BSRR = (1U << 4U))
 
/* Thanh ghi MAX7219 */
#define MAX_REG_DIGIT0      0x01
#define MAX_REG_DIGIT1      0x02
#define MAX_REG_DECODE      0x09
#define MAX_REG_INTENSITY   0x0A
#define MAX_REG_SCANLIMIT   0x0B
#define MAX_REG_SHUTDOWN    0x0C
#define MAX_REG_TEST        0x0F
 
#define MAX_BLANK           0x0F   /* mã tắt digit ở chế độ decode BCD */
 
/* ---------------- Delay bằng SysTick (polling, không dùng ngắt) ----------------
 * Không cần SysTick_Handler nên không đụng tới các file khác trong project. */
#define SysTick_CTRL_COUNTFLAG_Msk   (1U << 16)
 
static void SysTick_Init(void)
{
    SysTick->LOAD = (CPU_HZ / 1000U) - 1U;   /* 1 ms */
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}
 
static void delay_ms(uint32_t ms)
{
    while (ms--)
    {
        while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)) { }   /* đọc CTRL sẽ tự xóa cờ */
    }
}
 
/* ---------------- SPI1 ---------------- */
static void SPI1_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_SPI1EN;
 
    /* PA4: output push-pull 2 MHz (0x2)
     * PA5, PA7: alternate function push-pull 50 MHz (0xB) */
    GPIOA->CRL &= ~((0xFU << 16) | (0xFU << 20) | (0xFU << 28));
    GPIOA->CRL |=  ((0x2U << 16) | (0xBU << 20) | (0xBU << 28));
    CS_HIGH();
 
    /* Master, phần mềm quản lý NSS, frame 16 bit, fPCLK/8, mode 0 (CPOL=0, CPHA=0) */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                SPI_CR1_DFF  | (0x2U << 3);
    SPI1->CR1 |= SPI_CR1_SPE;
}
 
/* Gửi 1 lệnh 16 bit: [thanh ghi][dữ liệu] */
static void MAX7219_Write(uint8_t reg, uint8_t data)
{
    CS_LOW();
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    SPI1->DR = ((uint16_t)reg << 8) | data;
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    while (SPI1->SR & SPI_SR_BSY) { }
    CS_HIGH();
 
    (void)SPI1->DR;   /* xóa cờ RXNE/OVR */
    (void)SPI1->SR;
}
 
static void MAX7219_Init(void)
{
    MAX7219_Write(MAX_REG_TEST,      0x00);   /* tắt chế độ test */
    MAX7219_Write(MAX_REG_SHUTDOWN,  0x01);   /* thoát shutdown (bắt buộc, mặc định chip đang tắt) */
    MAX7219_Write(MAX_REG_SCANLIMIT, 0x01);   /* quét digit 0 và 1 */
    MAX7219_Write(MAX_REG_DECODE,    0x03);   /* decode BCD cho digit 0 và 1 */
    MAX7219_Write(MAX_REG_INTENSITY, 0x08);   /* độ sáng 0..15 */
    MAX7219_Write(MAX_REG_DIGIT0,    MAX_BLANK);
    MAX7219_Write(MAX_REG_DIGIT1,    MAX_BLANK);
}
 
/* Hiển thị số 0..15 trên 2 LED 7 đoạn: digit 1 = hàng chục, digit 0 = hàng đơn vị.
 * Nếu LED hiện ngược vị trí, đổi chỗ MAX_REG_DIGIT0 và MAX_REG_DIGIT1. */
static void Display(uint8_t n)
{
    MAX7219_Write(MAX_REG_DIGIT0, n % 10U);
    MAX7219_Write(MAX_REG_DIGIT1, (n >= 10U) ? (n / 10U) : MAX_BLANK);
}
 
/* ---------------- main ---------------- */
int main(void)
{
    uint8_t count = 0;
 
    SysTick_Init();
    SPI1_Init();
    MAX7219_Init();
 
    while (1)
    {
        Display(count);
        delay_ms(1000);                    /* 1 giây đổi số */
        count = (count + 1U) % 16U;        /* 0,1,...,15 rồi quay lại 0 */
    }
}
