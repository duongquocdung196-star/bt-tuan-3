#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

#define DS1307_ADDR 0xD0 // Địa chỉ I2C 7-bit (0x68) dịch trái 1 bit

/* --- HÀM TẠO TRỄ (DELAY TƯƠNG ĐỐI) --- */
void Delay_ms(uint32_t ms) {
    // Với thạch anh thông thường, vòng lặp này tạo trễ tương đối 1ms
    uint32_t i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 7200; j++); 
    }
}

/* --- CẤU HÌNH USART1 ĐỂ GỬI DỮ LIỆU LÊN MÁY TÍNH --- */
void UART1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 1. Cấp xung nhịp cho USART1 và GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // 2. Cấu hình chân TX (PA9) - Alternate Function Push-Pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. Cấu hình chân RX (PA10) - Input Floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 4. Cấu hình thông số USART: 115200, 8-bit, 1 Stop, No Parity
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    // 5. Cho phép USART1 hoạt động
    USART_Cmd(USART1, ENABLE);
}

// Hàm gửi chuỗi ký tự qua UART an toàn (thay thế cho printf trong Makefile)
void UART_SendString(char* str) {
    while (*str) {
        USART_SendData(USART1, *str++);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // Đợi gửi xong
    }
}

/* --- CẤU HÌNH I2C1 ĐỂ GIAO TIẾP VỚI DS1307 --- */
void I2C1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    // 1. Cấp xung cho I2C1 và GPIOB
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    // 2. Cấu hình chân SCL (PB6) và SDA (PB7) - Alternate Function Open-Drain
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 3. Cấu hình thông số I2C1
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000; // 100kHz
    I2C_Init(I2C1, &I2C_InitStructure);

    // 4. Cho phép I2C hoạt động
    I2C_Cmd(I2C1, ENABLE);
}

/* --- HÀM CHUYỂN ĐỔI BCD VÀ DECIMAL --- */
uint8_t dec2bcd(uint8_t val) {
    return ((val / 10 * 16) + (val % 10));
}

uint8_t bcd2dec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

/* --- CÁC HÀM GIAO TIẾP VỚI DS1307 --- */

// Cài đặt thời gian cho DS1307
void DS1307_SetTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t day, uint8_t date, uint8_t month, uint8_t year) {
    uint8_t data[7] = {dec2bcd(sec), dec2bcd(min), dec2bcd(hour), dec2bcd(day), dec2bcd(date), dec2bcd(month), dec2bcd(year)};
    int i;

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, 0x00); // Trỏ con trỏ vào thanh ghi 0x00 (Giây)
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    for(i = 0; i < 7; i++) {
        I2C_SendData(I2C1, data[i]);
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }

    I2C_GenerateSTOP(I2C1, ENABLE);
}

// Đọc thời gian từ DS1307
void DS1307_GetTime(uint8_t *sec, uint8_t *min, uint8_t *hour, uint8_t *day, uint8_t *date, uint8_t *month, uint8_t *year) {
    uint8_t data[7];
    int i;

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    // Bước 1: Báo cho DS1307 muốn đọc từ thanh ghi 0x00
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    
    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    
    I2C_SendData(I2C1, 0x00);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    // Bước 2: Bắt đầu quá trình nhận dữ liệu
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    
    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    for(i = 0; i < 7; i++) {
        if(i == 6) {
            // Byte cuối cùng thì ngắt ACK và gửi lệnh STOP
            I2C_AcknowledgeConfig(I2C1, DISABLE);
            I2C_GenerateSTOP(I2C1, ENABLE);
        } else {
            // Các byte trước đó thì cho phép ACK
            I2C_AcknowledgeConfig(I2C1, ENABLE);
        }
        
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
        data[i] = I2C_ReceiveData(I2C1);
    }

    // Chuyển đổi dữ liệu và gắn vào biến
    *sec   = bcd2dec(data[0] & 0x7F);
    *min   = bcd2dec(data[1]);
    *hour  = bcd2dec(data[2]);
    *day   = bcd2dec(data[3]);
    *date  = bcd2dec(data[4]);
    *month = bcd2dec(data[5]);
    *year  = bcd2dec(data[6]);
}


/* --- CHƯƠNG TRÌNH CHÍNH --- */
int main(void) {
    uint8_t sec, min, hour, day, date, month, year;
    char buffer[100];

    // Khởi tạo các ngoại vi
    UART1_Init();
    I2C1_Init();

    UART_SendString("\r\n--- HE THONG BAT DAU ---\r\n");

    // ========================================================
    // NẾU MẠCH LẦN ĐẦU SỬ DỤNG, BỎ COMMENT DÒNG DƯỚI ĐỂ CÀI GIỜ.
    // Sau khi nạp code lần 1, comment lại và nạp code lần 2
    // Cài đặt: 09:32:34 Thứ 7 (Ngày 7 trong tuần), Ngày 19/09/26
    // ========================================================
    //DS1307_SetTime(00, 30, 10, 7, 19, 9, 26);

    while (1) {
        // Đọc dữ liệu
        DS1307_GetTime(&sec, &min, &hour, &day, &date, &month, &year);

        // Ghép thành chuỗi và in lên màn hình
        sprintf(buffer, "Thoi gian: %02d:%02d:%02d - Ngay: %02d/%02d/20%02d\r", 
                hour, min, sec, date, month, year);
        UART_SendString(buffer);

        // Nghỉ 1 giây
        Delay_ms(1000);
    }
}
