#ifndef S32K3X8_UART_H
#define S32K3X8_UART_H

#include "hw/sysbus.h"
#include "hw/qdev-core.h"
#include "chardev/char-fe.h"
#include "qom/object.h"


#define TYPE_S32E8_LPUART "S32E8_LPUART"
#define S32K3X8_LPUART(obj) \
    OBJECT_CHECK(S32K3X8LPUARTState, (obj), TYPE_S32E8_LPUART)



void s32k3x8_lpuart_register_types(void);
#endif /* S32K3X8_UART_H */



