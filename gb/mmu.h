#ifndef MMU_H
#define MMU_H

#include <Utils.h>
#include <Qt>

#define _MMU (gb::mmu::getInstance())

namespace gb
{

class mmu : public utils::patterns::Singleton<mmu>
{
    friend class utils::patterns::Singleton<mmu>;

public:

    enum MEMORY_MAP
    {
        //bios
        BIOS_START = 0x0000,
        BIOS_END = 0x00FF,
        //cartridge header
        CARTRIDGE_HEADER_START = 0x0100,
        CARTRIDGE_HEADER_END = 0x014F,
        // rom 0
        ROM0_START = 0x0000,
        ROM0_END = 0x3FFF,
        // rom 1
        ROM1_START = 0x4000,
        ROM1_END = 0x7FFF,
        // graphics memory
        VRAM_START = 0x8000,
        VRAM_END = 0x9FFF,
        // external memory
        ERAM_START = 0xA000,
        ERAM_END = 0xBFFF,
        // working memory
        WRAM_START = 0xC000,
        WRAM_END = 0xDFFF,
        // working memory (shadow)
        WRAM_SHADOW_START = 0xE000,
        WRAM_SHADOW_END = 0xFDFF,
        // object attribute memory : sprite information (160 bytes)
        OAM_START = 0xFE00,
        OAM_END = 0xFE9F,
        //0xFEA0 - 0xFEFF : unused oam (set as 0)
        // io memory
        IO_START = 0xFF00,
        IO_END = 0xFF7F,
        //zero page memory
        ZRAM_START = 0xFF80,
        ZRAM_END = 0xFFFF
    };

    enum MEMORY_SIZE
    {
        BIOS_SIZE = BIOS_END - BIOS_START,
        ROM0_SIZE = ROM0_END - ROM0_START,
        ROM1_SIZE = ROM1_END - ROM1_START,
        ROM_SIZE  = ROM0_SIZE + ROM1_SIZE,
        VRAM_SIZE = VRAM_END - VRAM_START,
        ERAM_SIZE = ERAM_END - ERAM_START,
        WRAM_SIZE = WRAM_END - WRAM_START,
        OAM_SIZE = OAM_END - OAM_START,
        IO_SIZE = IO_END - IO_START,
        ZRAM_SIZE = ZRAM_END - ZRAM_START
    };

    mmu();

    quint8  rb(quint16 address);
    quint16 rw(quint16 address);
    void    wb(quint16 address, quint8 byte);
    void    ww(quint16 address, quint16 word);

private:

    quint8 BIOS[BIOS_SIZE]      ;
    quint8 ROM[ROM_SIZE]        ;
    quint8 ERAM[ERAM_SIZE]      ;
    quint8 WRAM[WRAM_SIZE]      ;
    quint8 ZRAM[ZRAM_SIZE]      ;

    bool in_bios                ;
};

}

#endif // MMU_H
