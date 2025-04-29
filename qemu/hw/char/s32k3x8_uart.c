/*
 *S32E8_LPUART Emulation
 *
 */
#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "chardev/char-fe.h"
#include "chardev/char-serial.h"
#include "hw/qdev-core.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/sysbus.h"  
#include "trace.h"
#include "qom/object.h"
#include "hw/irq.h" // Added missing header file

/*define everything*/ 
//=========================================================================
#define TYPE_S32E8_LPUART "S32E8_LPUART"
#define S32E8_LPUART_REGS_MEM_SIZE 0x800
#define I_(reg) (reg / sizeof(uint32_t))

/* UART Register definitions */
/*data from rm 4609/5253 page*/
#define VERID_OFFSET      0x00    /* Version ID Register */
#define PARAM_OFFSET      0x04    /* Parameter Register */
#define GLOBAL_OFFSET     0x08    /* Global Register */
#define PINCFG_OFFSET     0x0C    /* Pin Configuration Register */
#define BAUD_OFFSET       0x10    /* Baud Rate Register */
#define STAT_OFFSET       0x14    /* Status Register */
#define CTRL_OFFSET       0x18    /* Control Register */
#define DATA_OFFSET       0x1C    /* Data Register */
#define MATCH_OFFSET      0x20    /* Match Address Register */
#define MODIR_OFFSET      0x24    /* MODEM IrDA Register */
#define FIFO_OFFSET       0x28    /* FIFO Register */
#define WATER_OFFSET      0x2C    /* Watermark Register */
#define DATARO_OFFSET     0x30    /* Data Read-Only Register */
#define MCR_OFFSET        0x40    /* MODEM Control Register */
#define MSR_OFFSET        0x44    /* MODEM Status Register */
#define REIR_OFFSET       0x48    /* Receiver Extended Idle Register */
#define TEIR_OFFSET       0x4C    /* Transmitter Extended Idle Register */
#define HDCR_OFFSET       0x50    /* Half Duplex Control Register */
#define TOCR_OFFSET       0x58    /* Timeout Control Register */
#define TOSR_OFFSET       0x5C    /* Timeout Status Register */

/* Bit field definitions for key registers */
/* GLOBAL Register */
#define GLOBAL_RST        (1 << 0)    /* Software Reset */
#define GLOBAL_ENABLE     (1 << 1)    /* UART Enable */

/* CTRL Register */
#define CTRL_TE          (1 << 19)    /* Transmit Enable - Bit 19, matches RM */
#define CTRL_RE          (1 << 18)    /* Receive Enable - Bit 18, matches RM */

/* FIFO Register */
#define FIFO_TXFE        (1 << 0)     /* Transmit FIFO Empty */
#define FIFO_RXFF        (1 << 1)     /* Receive FIFO Full */
#define FIFO_TXOF        (1 << 2)     /* Transmit FIFO Overflow */
#define FIFO_RXUF        (1 << 3)     /* Receive FIFO Underflow */

/* STAT Register bits */
#define STAT_RDRF        (1 << 21)    /* Receive Data Register Full */
#define STAT_BRK13       (1 << 16)    /* Break Character Detected */

/* LPUART Register Structure Definition */
typedef struct {
    uint32_t verid;    /* Version ID Register */
    uint32_t param;    /* Parameter Register */
    uint32_t global;   /* Global Register */
    uint32_t pincfg;   /* Pin Configuration Register */
    uint32_t baud;     /* Baud Rate Register */
    uint32_t stat;     /* Status Register */
    uint32_t ctrl;     /* Control Register */
    uint32_t data;     /* Data Register */
    uint32_t match;    /* Match Address Register */
    uint32_t modir;    /* MODEM IrDA Register */
    uint32_t fifo;     /* FIFO Register */
    uint32_t water;    /* Watermark Register */
    uint32_t dataro;   /* Data Read-Only Register */
    uint32_t mcr;      /* MODEM Control Register */
    uint32_t msr;      /* MODEM Status Register */
    uint32_t reir;     /* Receiver Extended Idle Register */
    uint32_t teir;     /* Transmitter Extended Idle Register */
    uint32_t hdcr;     /* Half Duplex Control Register */
    uint32_t tocr;     /* Timeout Control Register */
    uint32_t tosr;     /* Timeout Status Register */
} LPUART_reg;

/* FIFO Structure Definition */ 
typedef struct {
    uint8_t *data;
    uint32_t sp, rp;
    uint32_t size; 
} FIFO_LPUART;

OBJECT_DECLARE_SIMPLE_TYPE(S32E8_LPUART_state, S32E8_LPUART)

/*Function Declarations*/
//========================================================================
static uint64_t S32E8_LPUART_read(void *opaque, hwaddr addr, unsigned size);
static void S32E8_LPUART_write(void *opaque, hwaddr addr, uint64_t val, unsigned size);
static void S32E8_LPUART_reset(DeviceState *dev);
static int S32E8_LPUART_can_receive(void *opaque);
static void S32E8_LPUART_receive(void *opaque, const uint8_t *buf, int size);
static void S32E8_LPUART_event(void *opaque, QEMUChrEvent event);

//Data Structure Definition
struct S32E8_LPUART_state
{
    SysBusDevice parent_obj;  //systembus device
    MemoryRegion iomem;
    CharBackend chr;
    qemu_irq irq;

    uint32_t instance_ID; // RM: 0-15
    uint32_t base_addr;
    
    FIFO_LPUART rx;
    FIFO_LPUART tx;

    LPUART_reg regs; 
};

//Structured initialization syntax - feature introduced in C99
//=========================================================================
static const MemoryRegionOps S32E8_LPUART_OPS = {
    .read = S32E8_LPUART_read,
    .write = S32E8_LPUART_write,
    .endianness = DEVICE_LITTLE_ENDIAN,

    //RM: 4668/5253 page align limit
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
    },

    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    }
};

static Property S32E8_LPUART_properties[] = {
    DEFINE_PROP_CHR("chardev", S32E8_LPUART_state, chr),
    DEFINE_PROP_UINT32("lpuart_id", S32E8_LPUART_state, instance_ID, 0),
    DEFINE_PROP_UINT32("rx-size", S32E8_LPUART_state, rx.size, 16),
    DEFINE_PROP_UINT32("tx-size", S32E8_LPUART_state, tx.size, 16),
    DEFINE_PROP_END_OF_LIST(),
};


/*Function Implementations*/
//==========================================================================

static void S32E8_LPUART_reset(DeviceState *dev)
{
    S32E8_LPUART_state *s = S32E8_LPUART(dev);
    
    // Reset registers to default values
    s->regs.verid = 0x0;          // Implementation specific
    s->regs.param = 0x0;          // Implementation specific
    s->regs.global = 0x00000000;
    s->regs.pincfg = 0x00000000;
    s->regs.baud = 0x0F000004;
    s->regs.stat = 0x00C00000;
    s->regs.ctrl = 0x00000000;
    s->regs.data = 0x00001000;
    s->regs.match = 0x00000000;
    s->regs.modir = 0x00000000;
    s->regs.fifo = 0x0;           // Implementation specific
    s->regs.water = 0x00000000;
    s->regs.dataro = 0x00001000;
    s->regs.mcr = 0x00000000;
    s->regs.msr = 0x00000000;
    s->regs.reir = 0x00000000;
    s->regs.teir = 0x00000000;
    s->regs.hdcr = 0x00000000;
    s->regs.tocr = 0x00000000;
    s->regs.tosr = 0x0000000F;

    s->base_addr = 0x40000000;
    s->instance_ID = 0;
}

// Simplified version of FIFO reset function
static void fifo_reset(FIFO_LPUART *q) {
    if (q->data == NULL && q->size > 0) {
        q->data = g_malloc0(q->size);
    } else if (q->data != NULL && q->size > 0) {
        memset(q->data, 0, q->size);
    }
    q->sp = q->rp = 0;
}

static void S32E8_LPUART_instance_init(Object *obj)
{
    SysBusDevice *dev = SYS_BUS_DEVICE(obj);
    S32E8_LPUART_state *s = S32E8_LPUART(obj);

    memory_region_init_io(&s->iomem, obj, &S32E8_LPUART_OPS, s,
                          TYPE_S32E8_LPUART, S32E8_LPUART_REGS_MEM_SIZE);
    sysbus_init_mmio(dev, &s->iomem);
    sysbus_init_irq(dev, &s->irq);
}

static void S32E8_LPUART_realize(DeviceState *dev, Error **errp)
{
    S32E8_LPUART_state *s = S32E8_LPUART(dev);

    // First perform reset, initialize all registers
    S32E8_LPUART_reset(dev);

    qemu_chr_fe_set_handlers(
        &s->chr,
        S32E8_LPUART_can_receive,
        S32E8_LPUART_receive,
        S32E8_LPUART_event,
        NULL,
        s,
        NULL,
        true
    );
    
    // Initialize FIFO
    fifo_reset(&s->rx);
    fifo_reset(&s->tx);
}

// Simplified read function - not implemented yet
static uint64_t S32E8_LPUART_read(void *opaque, hwaddr addr, unsigned size) {
    S32E8_LPUART_state *s = (S32E8_LPUART_state *)opaque;
    uint64_t ret = 0;
    
    // Simply return corresponding register value
    switch (addr) {
    case DATA_OFFSET:
        ret = s->regs.data;
        break;
    case CTRL_OFFSET:
        ret = s->regs.ctrl;
        break;
    case GLOBAL_OFFSET:
        ret = s->regs.global;
        break;
    default:
        // Other registers can be added as needed
        break;
    }
    
    return ret;
}

// write function - focus on implementing DATA register writing to QEMU backend
static void S32E8_LPUART_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) 
{
    S32E8_LPUART_state *s = (S32E8_LPUART_state *)opaque;

    switch (addr) {
    case GLOBAL_OFFSET:
        if (val & GLOBAL_RST) {
            // Software reset
            S32E8_LPUART_reset(DEVICE(s));
            // Clear reset flag
            val &= ~GLOBAL_RST;
        }
        s->regs.global = val;
        break;
        
    case CTRL_OFFSET:
        s->regs.ctrl = val;
        break;
        
     case DATA_OFFSET:
        s->regs.data = val;
        // Key functionality: When transmit is enabled and character device is connected, write character to QEMU backend
        if ((s->regs.ctrl & CTRL_TE) && qemu_chr_fe_backend_connected(&s->chr)) {
            uint8_t ch = val & 0xFF;
            qemu_chr_fe_write(&s->chr, &ch, 1);
            error_report("LPUART: Sent char '%c' (0x%02x) to chardev", 
                   isprint(ch) ? ch : '.', ch);
        } else {
            error_report("LPUART: Failed to send char - TE=%d, connected=%d", 
                   (s->regs.ctrl & CTRL_TE) ? 1 : 0,
                   qemu_chr_fe_backend_connected(&s->chr) ? 1 : 0);
        }
        break;
	
    default:
        // Other register writes not handled yet, or add as needed
        break;
    }
}

// Simplified can_receive function
static int S32E8_LPUART_can_receive(void *opaque) {
    S32E8_LPUART_state *s = (S32E8_LPUART_state *)opaque;
    
    // Only check receive enable
    if (!(s->regs.ctrl & CTRL_RE)) {
        return 0;
    }
    
    // Return fixed value, simplified implementation
    return 1;
}

// Simplified receive function
static void S32E8_LPUART_receive(void *opaque, const uint8_t *buf, int size) {
    // Receive functionality not implemented yet
}

// Simplified event function
static void S32E8_LPUART_event(void *opaque, QEMUChrEvent event) {
    // Events not handled yet
}

static void S32E8_LPUART_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    
    /* Only set necessary realize function, reset is called in realize */
    dc->realize = S32E8_LPUART_realize;
    device_class_set_props(dc, S32E8_LPUART_properties);
}

static const TypeInfo S32E8_LPUART_info = {
    .name = TYPE_S32E8_LPUART,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32E8_LPUART_state),
    .instance_init = S32E8_LPUART_instance_init,
    .class_init = S32E8_LPUART_class_init,
};

static void S32E8_LPUART_register_types(void)
{
    type_register_static(&S32E8_LPUART_info);
}

type_init(S32E8_LPUART_register_types);
