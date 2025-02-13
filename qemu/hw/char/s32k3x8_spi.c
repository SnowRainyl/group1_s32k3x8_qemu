#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "hw/ssi/ssi.h"
#include "s32k3x8_spi.h"
#include "migration/vmstate.h"

static void s32k3x8_spi_reset(DeviceState* dev)
{
    S32K3X8SPIState* s = S32K3X8_SPI(dev);

    s->spi_cr1 = 0x00000000;
    s->spi_cr2 = 0x00000000;
    s->spi_sr = 0x00000002;
    s->spi_dr = 0x00000000;
    s->spi_crcpr = 0x00000007;
    s->spi_rxcrcr = 0x00000000;
    s->spi_txcrcr = 0x00000000;
}

static void s32k3x8_spi_transfer(S32K3X8SPIState* s)
{
    qemu_log("S32K3X8 SPI: Transferring data: 0x%x\n", s->spi_dr);

    s->spi_dr = ssi_transfer(s->ssi, s->spi_dr); /* Send and receive */
    s->spi_sr |= S32K3X8_SPI_SR_RXNE;

    qemu_log("S32K3X8 SPI: Received data: 0x%x\n", s->spi_dr);
}

static uint64_t s32k3x8_spi_read(void* opaque, hwaddr addr, unsigned size)
{
    S32K3X8SPIState* s = opaque;

    switch (addr) {
    case S32K3X8_SPI_CR1:
        return s->spi_cr1;
    case S32K3X8_SPI_CR2:
        return s->spi_cr2;
    case S32K3X8_SPI_SR:
        return s->spi_sr;
    case S32K3X8_SPI_DR:
        s32k3x8_spi_transfer(s);
        s->spi_sr &= ~S32K3X8_SPI_SR_RXNE;
        return s->spi_dr;
    default:
        qemu_log("S32K3X8 SPI: Invalid read address 0x%" HWADDR_PRIx "\n", addr);
        return 0;
    }
}

static void s32k3x8_spi_write(void* opaque, hwaddr addr, uint64_t value, unsigned size)
{
    S32K3X8SPIState* s = opaque;

    switch (addr) {
    case S32K3X8_SPI_CR1:
        s->spi_cr1 = value;
        break;
    case S32K3X8_SPI_CR2:
        s->spi_cr2 = value;
        break;
    case S32K3X8_SPI_DR:
        s->spi_dr = value;
        s32k3x8_spi_transfer(s);
        break;
    default:
        qemu_log("S32K3X8 SPI: Invalid write address 0x%" HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps s32k3x8_spi_ops = {
    .read = s32k3x8_spi_read,
    .write = s32k3x8_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_s32k3x8_spi = {
    .name = TYPE_S32K3X8_SPI,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(spi_cr1, S32K3X8SPIState),
        VMSTATE_UINT32(spi_cr2, S32K3X8SPIState),
        VMSTATE_UINT32(spi_sr, S32K3X8SPIState),
        VMSTATE_UINT32(spi_dr, S32K3X8SPIState),
        VMSTATE_UINT32(spi_crcpr, S32K3X8SPIState),
        VMSTATE_UINT32(spi_rxcrcr, S32K3X8SPIState),
        VMSTATE_UINT32(spi_txcrcr, S32K3X8SPIState),
        VMSTATE_END_OF_LIST()
    }
};

static void s32k3x8_spi_init(Object* obj)
{
printf("begin spi device!\n");
    S32K3X8SPIState* s = S32K3X8_SPI(obj);
   printf("spi device!\n");
    memory_region_init_io(&s->mmio, obj, &s32k3x8_spi_ops, s,
        TYPE_S32K3X8_SPI, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    s->ssi = ssi_create_bus(DEVICE(obj), "spi");
}

static void s32k3x8_spi_class_init(ObjectClass* klass, void* data)
{
    DeviceClass* dc = DEVICE_CLASS(klass);

    device_class_set_legacy_reset(dc, s32k3x8_spi_reset);
    dc->vmsd = &vmstate_s32k3x8_spi;
}

static const TypeInfo s32k3x8_spi_info = {
    .name = TYPE_S32K3X8_SPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K3X8SPIState),
    .instance_init = s32k3x8_spi_init,
    .class_init = s32k3x8_spi_class_init,
};

static void s32k3x8_spi_register_types(void)
{
    printf("begin spi!\n");
    type_register_static(&s32k3x8_spi_info);
    printf("finish spi!\n");

}

type_init(s32k3x8_spi_register_types);
