#include "mmu.h"

using namespace gb;

mmu::mmu()
{
    memset(BIOS, 0, BIOS_SIZE);
    memset(ROM, 0, ROM_SIZE);
    memset(ERAM, 0, ERAM_SIZE);
    memset(WRAM, 0, WRAM_SIZE);
    memset(ZRAM, 0, ZRAM_SIZE);

    in_bios = true;
}

quint8  mmu::rb(quint16 address)
{
    //rom 0
    if(address >= ROM0_START && address <= ROM0_END)
    {
        if(in_bios)
        {
            if(address <= BIOS_END)
                return BIOS[address];
            else
                in_bios = false;
        }

        return ROM[address];
    }

    //rom 1
    else if(address >= ROM1_START && address <= ROM1_END)
    {
        return ROM[address];
    }

    //vram
    else if(address >= VRAM_START && address <= VRAM_END)
    {
        // return gpu vram here
    }

    //eram
    else if(address >= ERAM_START && address <= ERAM_END)
    {
        return ERAM[address & 0x1FFF];
    }

    //wram
    else if(address >= WRAM_START && address <= WRAM_END)
    {
        return WRAM[address & 0x1FFF];
    }

    //wram shadow
    else if(address >= WRAM_SHADOW_START && address <= WRAM_SHADOW_END)
    {
        return WRAM[address & 0x1FFF];
    }

    //oam
    else if(address >= OAM_START && address <= OAM_END)
    {
        // return gpu oam here
    }

    //io
    else if(address >= IO_START && address <= IO_END)
    {
        //io management here
    }

    //zram
    else if(address >= ZRAM_START && address <= ZRAM_END)
    {
        return ZRAM[address & 0x7F];
    }

    return 0;
}

quint16 mmu::rw(quint16 address)
{
    return rb(address) + ( rb(address + 1) << 8);
}

void    mmu::wb(quint16 address, quint8 byte)
{
    //rom 0
    if(address >= ROM0_START && address <= ROM0_END)
    {
        if(in_bios)
        {
            if(address <= BIOS_END)
            {
                BIOS[address] = byte;
                return;
            }
            else
                in_bios = false;
        }

        ROM[address] = byte;
    }

    //rom 1
    else if(address >= ROM1_START && address <= ROM1_END)
    {
        ROM[address] = byte;
    }

    //vram
    else if(address >= VRAM_START && address <= VRAM_END)
    {
        // set gpu vram here
    }

    //eram
    else if(address >= ERAM_START && address <= ERAM_END)
    {
        ERAM[address & 0x1FFF] = byte;
    }

    //wram
    else if(address >= WRAM_START && address <= WRAM_END)
    {
        WRAM[address & 0x1FFF] = byte;
    }

    //wram shadow
    else if(address >= WRAM_SHADOW_START && address <= WRAM_SHADOW_END)
    {
        WRAM[address & 0x1FFF] = byte;
    }

    //oam
    else if(address >= OAM_START && address <= OAM_END)
    {
        // set gpu oam here
    }

    //io
    else if(address >= IO_START && address <= IO_END)
    {
        //io management here
    }

    //zram
    else if(address >= ZRAM_START && address <= ZRAM_END)
    {
        ZRAM[address & 0x7F] = byte;
    }
}

void    mmu::ww(quint16 address, quint16 word)
{
    wb(address, word & 0x0F);
    wb(address + 1, word & 0xF0);
}
