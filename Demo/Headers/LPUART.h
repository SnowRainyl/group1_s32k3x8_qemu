#define S32K3_PERIPH_BASE        0x40000000
#define S32K3_UART_BASE          (S32K3_PERIPH_BASE + 0x4A000) 
/* UART register offsets */
#define GLOBAL_OFFSET     0x08    /* Global register */
#define BAUD_OFFSET       0x10    /* Baud rate register */
#define STAT_OFFSET       0x14    /* Status register */
#define CTRL_OFFSET       0x18    /* Control register */
#define DATA_OFFSET       0x1C    /* Data register */
#define FIFO_OFFSET       0x28    /* FIFO register */
#define WATER_OFFSET      0x2C    /* Watermark register */

/* UART bit definitions */
#define GLOBAL_RST        (1 << 0)    /* Software reset */
#define GLOBAL_ENABLE     (1 << 1)    /* UART enable */
#define CTRL_TE           (1 << 19)   /* Transmit enable */
#define CTRL_RE           (1 << 18)   /* Receive enable */
#define CTRL_TIE          (1 << 23)
#define STAT_TC           (1 << 22)   /* Transmission complete flag */
#define STAT_TDRE         (1 << 23)   /* Transmit data register empty flag */
#define FIFO_RXFE         (1 << 3)    /* Receive FIFO enable */
#define FIFO_TXFE         (1 << 7)    /* Transmit FIFO enable */

/*read related*/
#define STAT_RDRF         (1 << 21)   /* Receive data register full flag */
#define STAT_OR           (1 << 19)   /* Receiver overrun flag */
#define STAT_NF           (1 << 18)   /* Noise flag */
#define STAT_FE           (1 << 17)   /* Frame error flag */
#define STAT_PF           (1 << 16)   /* Parity error flag */
#define CTRL_RIE          (1 << 21)   /* Receiver interrupt enable */

/* Access macros */
#define UART_REG(offset) (*(volatile uint32_t *)(S32K3_UART_BASE + (offset)))
#define mainTASK_PRIORITY ( 4UL )

