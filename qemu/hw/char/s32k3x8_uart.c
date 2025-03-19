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
#include "hw/irq.h" // 添加缺少的头文件

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
#define CTRL_TE          (1 << 19)    /* 发送使能 - 位19，与RM匹配 */
#define CTRL_RE          (1 << 18)    /* 接收使能 - 位18，与RM匹配 */

/* FIFO Register */
#define FIFO_TXFE        (1 << 0)     /* Transmit FIFO Empty */
#define FIFO_RXFF        (1 << 1)     /* Receive FIFO Full */
#define FIFO_TXOF        (1 << 2)     /* Transmit FIFO Overflow */
#define FIFO_RXUF        (1 << 3)     /* Receive FIFO Underflow */

/* STAT Register bits */
#define STAT_RDRF        (1 << 21)    /* Receive Data Register Full */
#define STAT_BRK13       (1 << 16)    /* Break Character Detected */

/* LPUART 寄存器结构体定义 */
typedef struct {
    uint32_t verid;    /* 版本ID寄存器 */
    uint32_t param;    /* 参数寄存器 */
    uint32_t global;   /* 全局寄存器 */
    uint32_t pincfg;   /* 引脚配置寄存器 */
    uint32_t baud;     /* 波特率寄存器 */
    uint32_t stat;     /* 状态寄存器 */
    uint32_t ctrl;     /* 控制寄存器 */
    uint32_t data;     /* 数据寄存器 */
    uint32_t match;    /* 匹配地址寄存器 */
    uint32_t modir;    /* MODEM IrDA寄存器 */
    uint32_t fifo;     /* FIFO寄存器 */
    uint32_t water;    /* 水位寄存器 */
    uint32_t dataro;   /* 只读数据寄存器 */
    uint32_t mcr;      /* MODEM控制寄存器 */
    uint32_t msr;      /* MODEM状态寄存器 */
    uint32_t reir;     /* 接收器扩展空闲寄存器 */
    uint32_t teir;     /* 发送器扩展空闲寄存器 */
    uint32_t hdcr;     /* 半双工控制寄存器 */
    uint32_t tocr;     /* 超时控制寄存器 */
    uint32_t tosr;     /* 超时状态寄存器 */
} LPUART_reg;

/* FIFO结构体定义 */ 
typedef struct {
    uint8_t *data;
    uint32_t sp, rp;
    uint32_t size; 
} FIFO_LPUART;

OBJECT_DECLARE_SIMPLE_TYPE(S32E8_LPUART_state, S32E8_LPUART)

/*函数声明*/
//========================================================================
static uint64_t S32E8_LPUART_read(void *opaque, hwaddr addr, unsigned size);
static void S32E8_LPUART_write(void *opaque, hwaddr addr, uint64_t val, unsigned size);
static void S32E8_LPUART_reset(DeviceState *dev);
static int S32E8_LPUART_can_receive(void *opaque);
static void S32E8_LPUART_receive(void *opaque, const uint8_t *buf, int size);
static void S32E8_LPUART_event(void *opaque, QEMUChrEvent event);

//数据结构定义
struct S32E8_LPUART_state
{
    SysBusDevice parent_obj;  //systembus 设备
    MemoryRegion iomem;
    CharBackend chr;
    qemu_irq irq;

    uint32_t instance_ID; // RM: 0-15
    uint32_t base_addr;
    
    FIFO_LPUART rx;
    FIFO_LPUART tx;

    LPUART_reg regs; 
};

//结构化初始化语法 c99引入的特性
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


/*具体实现函数 */
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

// 简化版的FIFO reset函数
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

    // 先执行重置，初始化所有寄存器
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
    
    // 初始化FIFO
    fifo_reset(&s->rx);
    fifo_reset(&s->tx);
}

// 简化版read函数 - 暂不实现
static uint64_t S32E8_LPUART_read(void *opaque, hwaddr addr, unsigned size) {
    S32E8_LPUART_state *s = (S32E8_LPUART_state *)opaque;
    uint64_t ret = 0;
    
    // 简单返回对应寄存器值
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
        // 其他寄存器可以根据需要添加
        break;
    }
    
    return ret;
}

// write函数 - 重点实现DATA寄存器写入到QEMU后端的功能
static void S32E8_LPUART_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) 
{
    S32E8_LPUART_state *s = (S32E8_LPUART_state *)opaque;

    switch (addr) {
    case GLOBAL_OFFSET:
        if (val & GLOBAL_RST) {
            // 软件复位
            S32E8_LPUART_reset(DEVICE(s));
            // 清除复位标志
            val &= ~GLOBAL_RST;
        }
        s->regs.global = val;
        break;
        
    case CTRL_OFFSET:
        s->regs.ctrl = val;
        break;
        
     case DATA_OFFSET:
        s->regs.data = val;
        // 关键功能：当发送使能且有字符设备连接时，写入字符到QEMU后端
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
        // 其他寄存器写入暂不处理，或者根据需要添加
        break;
    }
}

// 简化版can_receive函数
static int S32E8_LPUART_can_receive(void *opaque) {
    S32E8_LPUART_state *s = (S32E8_LPUART_state *)opaque;
    
    // 只检查接收使能
    if (!(s->regs.ctrl & CTRL_RE)) {
        return 0;
    }
    
    // 返回固定值，简化实现
    return 1;
}

// 简化版receive函数
static void S32E8_LPUART_receive(void *opaque, const uint8_t *buf, int size) {
    // 暂不实现接收功能
}

// 简化版event函数
static void S32E8_LPUART_event(void *opaque, QEMUChrEvent event) {
    // 暂不处理事件
}

static void S32E8_LPUART_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    
    /* 只设置必要的 realize 函数，reset 在 realize 中调用 */
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
