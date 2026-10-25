#ifndef _SJA1000_H_
#define _SJA1000_H_

#include <stdint.h>

#define CAN_BASE        0x4000

/* Mode & Control Register */
#define CAN_MODECTRL    (*(volatile uint8_t *)(CAN_BASE + 0))
#define RESREQ          0x01                                    

/* Command Register */
#define CAN_CMD         (*(volatile uint8_t *)(CAN_BASE + 1))
#define TXREQ           0x01                                    
#define RRB             0x04                                    

/* Status Register */
#define CAN_STATUS      (*(volatile uint8_t *)(CAN_BASE + 2))
#define TBS             0x04                                   

/* Interrupt Register */
#define CAN_INT         (*(volatile uint8_t *)(CAN_BASE + 3))
#define RI              0x01   
#define TI              0x02                                   
#define EI              0x04   
#define PI              0x20   
#define ALI             0x40
#define BIE             0x80                                

/* Interrupt Enable & Control Register */
#define CAN_INTEN       (*(volatile uint8_t *)(CAN_BASE + 4))
#define RIEN            0x01        					
#define TIEN            0x02       
#define EIEN            0x04  
#define ALIEN           0x40
#define BIEEN           0x80

/* Bus Timing Registers */
#define CAN_TMG0        (*(volatile uint8_t *)(CAN_BASE + 6))
#define CAN_TMG1        (*(volatile uint8_t *)(CAN_BASE + 7))

/* Output Control Register */
#define CAN_OCR         (*(volatile uint8_t *)(CAN_BASE + 8))

/* Acceptance Code & Mask Registers */
#define CAN_AC0         (*(volatile uint8_t *)(CAN_BASE + 16))
#define CAN_AC1         (*(volatile uint8_t *)(CAN_BASE + 17))
#define CAN_AC2         (*(volatile uint8_t *)(CAN_BASE + 18))
#define CAN_AC3         (*(volatile uint8_t *)(CAN_BASE + 19))
#define CAN_AM0         (*(volatile uint8_t *)(CAN_BASE + 20))
#define CAN_AM1         (*(volatile uint8_t *)(CAN_BASE + 21))
#define CAN_AM2         (*(volatile uint8_t *)(CAN_BASE + 22))
#define CAN_AM3         (*(volatile uint8_t *)(CAN_BASE + 23))

/* Receive (RX) Registers */
#define CAN_RX_FRMINFO  (*(volatile uint8_t *)(CAN_BASE + 16))
#define CAN_RX_BUF1     (*(volatile uint8_t *)(CAN_BASE + 17))
#define CAN_RX_BUF2     (*(volatile uint8_t *)(CAN_BASE + 18))
#define CAN_RX_DATA1    (*(volatile uint8_t *)(CAN_BASE + 19))
#define CAN_RX_DATA2    (*(volatile uint8_t *)(CAN_BASE + 20))
#define CAN_RX_DATA3    (*(volatile uint8_t *)(CAN_BASE + 21))
#define CAN_RX_DATA4    (*(volatile uint8_t *)(CAN_BASE + 22))
#define CAN_RX_DATA5    (*(volatile uint8_t *)(CAN_BASE + 23))
#define CAN_RX_DATA6    (*(volatile uint8_t *)(CAN_BASE + 24))
#define CAN_RX_DATA7    (*(volatile uint8_t *)(CAN_BASE + 25))
#define CAN_RX_DATA8    (*(volatile uint8_t *)(CAN_BASE + 26))
#define CAN_RX_DATA_BASE                      (CAN_BASE + 19)

/* Transmit (TX) Registers */
#define CAN_TX_FRMINFO  (*(volatile uint8_t *)(CAN_BASE + 16))
#define CAN_TX_ID1      (*(volatile uint8_t *)(CAN_BASE + 17))
#define CAN_TX_ID2      (*(volatile uint8_t *)(CAN_BASE + 18))
#define CAN_TX_DATA1    (*(volatile uint8_t *)(CAN_BASE + 19))
#define CAN_TX_DATA2    (*(volatile uint8_t *)(CAN_BASE + 20))
#define CAN_TX_DATA3    (*(volatile uint8_t *)(CAN_BASE + 21)) 
#define CAN_TX_DATA4    (*(volatile uint8_t *)(CAN_BASE + 22))
#define CAN_TX_DATA5    (*(volatile uint8_t *)(CAN_BASE + 23))
#define CAN_TX_DATA6    (*(volatile uint8_t *)(CAN_BASE + 24))
#define CAN_TX_DATA7    (*(volatile uint8_t *)(CAN_BASE + 25))
#define CAN_TX_DATA8    (*(volatile uint8_t *)(CAN_BASE + 26))
#define CAN_TX_DATA_BASE                      (CAN_BASE + 19)

/* Clock Divider Register */
#define CAN_CLKDIV      (*(volatile uint8_t *)(CAN_BASE + 31))
#define COMPBYPASS      0x40     
#define PCANMODE        0x80                                    

/* Listen / Operational Modes */
#define READWRITE       0x00
#define LISTEN          0x01

struct CAN_Message_Struct {
    uint16_t id;
    uint8_t  dataLength;
    uint8_t  data[8];
};

void CAN_Reg_Init(void);
void SJA1000_Init(uint8_t BTR0, uint8_t BTR1, uint8_t mode);
void SJA1000_Auto_Baud_Err_IRQ(void); 
void SJA1000_Print_Frame_Rcv_IRQ(void); 
void SJA1000_Auto_Baud_Rcv_IRQ(void);
void SJA1000_Transmit_IRQ(void);
void CAN_Transmit(uint16_t id, uint8_t dataLength, 
                  uint8_t byte1, uint8_t byte2,
                  uint8_t byte3, uint8_t byte4,
                  uint8_t byte5, uint8_t byte6,
                  uint8_t byte7, uint8_t byte8);

#endif /* _SJA1000_H_ */