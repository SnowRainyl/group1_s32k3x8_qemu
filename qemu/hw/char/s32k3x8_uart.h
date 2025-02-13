#ifndef S32K3X8_UART_H
#define S32K3X8_UART_H

#include "hw/sysbus.h"
#include "hw/qdev-core.h"
#include "chardev/char-fe.h"
#include "qom/object.h"


#define TYPE_S32K3X8_LPUART "s32k3x8-lpuart"
#define S32K3X8_LPUART(obj) \
    OBJECT_CHECK(S32K3X8LPUARTState, (obj), TYPE_S32K3X8_LPUART)
/* Register offsets */
#define S32K3X8_LPUART_VERID         0x000
#define S32K3X8_LPUART_PARAM         0x004
#define S32K3X8_LPUART_GLOBAL        0x008
#define S32K3X8_LPUART_PINCFG        0x00C
#define S32K3X8_LPUART_BAUD          0x010
#define S32K3X8_LPUART_STAT          0x014
#define S32K3X8_LPUART_CTRL          0x018
#define S32K3X8_LPUART_DATA          0x01C
#define S32K3X8_LPUART_MATCH         0x020
#define S32K3X8_LPUART_MODIR         0x024
#define S32K3X8_LPUART_FIFO          0x028
#define S32K3X8_LPUART_WATER         0x02C
#define S32K3X8_LPUART_DATARO        0x030
#define S32K3X8_LPUART_MCR           0x034
#define S32K3X8_LPUART_MSR           0x038
#define S32K3X8_LPUART_REIR          0x03C
#define S32K3X8_LPUART_TEIR          0x040
#define S32K3X8_LPUART_HDCR          0x044
#define S32K3X8_LPUART_TOCR          0x048
#define S32K3X8_LPUART_TOSR          0x04C
#define S32K3X8_LPUART_TIMEOUT0      0x050
#define S32K3X8_LPUART_TCBR0         0x060
#define S32K3X8_LPUART_TDBR0         0x080

/* Register bit definitions */
#define S32K3X8_LPUART_GLOBAL_EN     (1 << 0)
#define S32K3X8_LPUART_GLOBAL_RST    (1 << 1)

#define S32K3X8_LPUART_STAT_RXRDY    (1 << 21)
#define S32K3X8_LPUART_STAT_TXRDY    (1 << 22)
#define S32K3X8_LPUART_STAT_ERR_MASK 0x0F000000

#define S32K3X8_LPUART_CTRL_RX_EN    (1 << 18)
#define S32K3X8_LPUART_CTRL_TX_EN    (1 << 19)
#define S32K3X8_LPUART_CTRL_RXIE     (1 << 21)
#define S32K3X8_LPUART_CTRL_TXIE     (1 << 23)

/* Reset values */
#define S32K3X8_LPUART_VERID_RESET01  0x00000001
#define S32K3X8_LPUART_PARAM_RESET01  0x00002000
#define S32K3X8_LPUART_FIFO_RESET01   0x0F0F0000
#define S32K3X8_LPUART_VERID_RESET    0x00000001
#define S32K3X8_LPUART_PARAM_RESET    0x00002000
#define S32K3X8_LPUART_FIFO_RESET     0x0F0F0000
typedef struct S32K3X8LPUARTClass {
    SysBusDeviceClass parent_class;
} S32K3X8LPUARTClass;

typedef struct S32K3X8LPUARTState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    CharBackend chr;
    qemu_irq irq;

    uint8_t instance_id;
    
    /* Registers */
    uint32_t verid;
    uint32_t param;
    uint32_t global;
    uint32_t pincfg;
    uint32_t baud;
    uint32_t stat;
    uint32_t ctrl;
    uint32_t data;
    uint32_t match;
    uint32_t modir;
    uint32_t fifo;
    uint32_t water;
    uint32_t dataro;
    uint32_t mcr;
    uint32_t msr;
    uint32_t reir;
    uint32_t teir;
    uint32_t hdcr;
    uint32_t tocr;
    uint32_t tosr;
    uint32_t timeout[4];
    uint32_t tcbr[128];
    uint32_t tdbr[256];
} S32K3X8LPUARTState;


void s32k3x8_lpuart_register_types(void);
#endif /* S32K3X8_UART_H */



