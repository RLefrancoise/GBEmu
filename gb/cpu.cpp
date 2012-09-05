#include "cpu.h"
#include "mmu.h"

#define _A (R[REGISTER_A])
#define _B (R[REGISTER_B])
#define _C (R[REGISTER_C])
#define _D (R[REGISTER_D])
#define _E (R[REGISTER_E])
#define _H (R[REGISTER_H])
#define _L (R[REGISTER_L])

#define _BC ( (quint16)(_C + (_B << 8)) )
#define _DE ( (quint16)(_E + (_D << 8)) )
#define _HL ( (quint16)(_L + (_H << 8)) )

#define _d8 ( (quint8) _MMU->rb(PC+1) )
#define _d16 ( (quint16)( _MMU->rb(PC+1) + (_MMU->rb(PC+2) << 8) ) )
#define _a8 ( (quint8) _MMU->rb(PC+1) )
#define _a16 ( (quint16)( _MMU->rb(PC+1) + (_MMU->rb(PC+2) << 8) ) )
#define _r8 ( (qint8) _MMU->rb(PC+1) )

using namespace gb;

cpu::opcode::opcode(QString mnemonic, quint8 length, quint8 cycles, opcode_func exec, quint8 not_exec_cycles)
{
    this->mnemonic = mnemonic;
    this->length = length;
    this->cycles = cycles;
    this->exec = exec;
    this->not_exec_cycles = not_exec_cycles;
}




cpu::cpu()
{
    //init registers
    for(quint8 i = 0 ; i < REGISTER_NUMBER ; ++i)
        R[i] = 0;

    F  = 0;
    SP = 0;
    PC = 0;

    cycles_counter = 0;

    STOP = false;
    HALT = false;
    IME = true;
    last_opcode_not_executed = false;

    //init opcodes

    current_opcode = NULL;

    //1-byte
    opcodes_table.insert( 0x00, new opcode("NOP", 1, 4, &cpu::nop) );                           /* 0x00 - NOP */
    opcodes_table.insert( 0x01, new opcode("LD BC,d16", 3, 12, &cpu::ld_bc_d16) );              /* 0x01 - LD BC, d16 */
    opcodes_table.insert( 0x02, new opcode("LD (BC),A", 1, 8, &cpu::ld_bc_a) );                 /* 0x02 - LD (BC), A */
    opcodes_table.insert( 0x03, new opcode("INC BC", 1, 8, &cpu::inc_bc) );                     /* 0x03 - INC BC */
    opcodes_table.insert( 0x04, new opcode("INC B", 1, 4, &cpu::inc_b) );                       /* 0x04 - INC B */
    opcodes_table.insert( 0x05, new opcode("DEC B", 1, 4, &cpu::dec_b) );                       /* 0x05 - DEC B */
    opcodes_table.insert( 0x06, new opcode("LD B,d8", 2, 8, &cpu::ld_b_d8) );                   /* 0x06 - LD B, d8 */
    opcodes_table.insert( 0x07, new opcode("RLCA", 1, 4, &cpu::rlca) );                         /* 0x07 - RLCA */
    opcodes_table.insert( 0x08, new opcode("LD (a16), SP", 3, 20, &cpu::ld_a16_sp) );           /* 0x08 - LD (a16), SP */
    opcodes_table.insert( 0x09, new opcode("ADD HL, BC", 1, 8, &cpu::add_hl_bc) );              /* 0x09 - ADD HL, BC */
    opcodes_table.insert( 0x0A, new opcode("LD A, (BC)", 1, 8, &cpu::ld_a_bc) );                /* 0x0A - LD A, (BC) */
    opcodes_table.insert( 0x0B, new opcode("DEC BC", 1, 8, &cpu::dec_bc) );                     /* 0x0B - DEC BC */
    opcodes_table.insert( 0x0C, new opcode("INC C", 1, 4, &cpu::inc_c) );                       /* 0x0C - INC C */
    opcodes_table.insert( 0x0D, new opcode("DEC C", 1, 4, &cpu::dec_c) );                       /* 0x0D - DEC C */
    opcodes_table.insert( 0x0E, new opcode("LD C, d8", 2, 8, &cpu::ld_c_d8) );                  /* 0x0E - LD C, d8 */
    opcodes_table.insert( 0x0F, new opcode("RRCA", 1, 4, &cpu::rrca) );                         /* 0x0F - RRCA */

    opcodes_table.insert( 0x10, new opcode("STOP 0", 2, 4, &cpu::stop) );                       /* 0x10 - STOP 0 */
    opcodes_table.insert( 0x11, new opcode("LD DE,d16", 3, 12, &cpu::ld_de_d16) );              /* 0x11 - LD DE, d16 */
    opcodes_table.insert( 0x12, new opcode("LD (DE),A", 1, 8, &cpu::ld_de_a) );                 /* 0x12 - LD (DE), A */
    opcodes_table.insert( 0x13, new opcode("INC DE", 1, 8, &cpu::inc_de) );                     /* 0x13 - INC DE */
    opcodes_table.insert( 0x14, new opcode("INC D", 1, 4, &cpu::inc_d) );                       /* 0x14 - INC D */
    opcodes_table.insert( 0x15, new opcode("DEC D", 1, 4, &cpu::dec_d) );                       /* 0x15 - DEC D */
    opcodes_table.insert( 0x16, new opcode("LD D,d8", 2, 8, &cpu::ld_d_d8) );                   /* 0x16 - LD D, d8 */
    opcodes_table.insert( 0x17, new opcode("RLA", 1, 4, &cpu::rla) );                           /* 0x17 - RLA */
    opcodes_table.insert( 0x18, new opcode("JR r8", 2, 12, &cpu::jr_r8) );                      /* 0x18 - JR r8 */
    opcodes_table.insert( 0x19, new opcode("ADD HL, DE", 1, 8, &cpu::add_hl_de) );              /* 0x19 - ADD HL, DE */
    opcodes_table.insert( 0x1A, new opcode("LD A, (DE)", 1, 8, &cpu::ld_a_de) );                /* 0x1A - LD A, (DE) */
    opcodes_table.insert( 0x1B, new opcode("DEC DE", 1, 8, &cpu::dec_de) );                     /* 0x1B - DEC DE */
    opcodes_table.insert( 0x1C, new opcode("INC E", 1, 4, &cpu::inc_e) );                       /* 0x1C - INC E */
    opcodes_table.insert( 0x1D, new opcode("DEC E", 1, 4, &cpu::dec_e) );                       /* 0x1D - DEC E */
    opcodes_table.insert( 0x1E, new opcode("LD E, d8", 2, 8, &cpu::ld_e_d8) );                  /* 0x1E - LD E, d8 */
    opcodes_table.insert( 0x1F, new opcode("RRA", 1, 4, &cpu::rra) );                           /* 0x1F - RRA */

    opcodes_table.insert( 0x20, new opcode("JR NZ,r8", 2, 12, &cpu::jr_nz_r8, 8) );             /* 0x20 - JR NZ, r8 */
    opcodes_table.insert( 0x21, new opcode("LD HL,d16", 3, 12, &cpu::ld_hl_d16) );              /* 0x21 - LD HL, d16 */
    opcodes_table.insert( 0x22, new opcode("LDI (HL),A", 1, 8, &cpu::ldi_hl_a) );               /* 0x22 - LDI (HL), A */
    opcodes_table.insert( 0x23, new opcode("INC HL", 1, 8, &cpu::inc_hl) );                     /* 0x23 - INC HL */
    opcodes_table.insert( 0x24, new opcode("INC H", 1, 4, &cpu::inc_h) );                       /* 0x24 - INC H */
    opcodes_table.insert( 0x25, new opcode("DEC H", 1, 4, &cpu::dec_h) );                       /* 0x25 - DEC H */
    opcodes_table.insert( 0x26, new opcode("LD H,d8", 2, 8, &cpu::ld_h_d8) );                   /* 0x26 - LD H, d8 */
    opcodes_table.insert( 0x27, new opcode("DAA", 1, 4, &cpu::daa) );                           /* 0x27 - DAA */
    opcodes_table.insert( 0x28, new opcode("JR Z,r8", 2, 12, &cpu::jr_z_r8, 8) );               /* 0x28 - JR Z, r8 */
    opcodes_table.insert( 0x29, new opcode("ADD HL, HL", 1, 8, &cpu::add_hl_hl) );              /* 0x29 - ADD HL, HL */
    opcodes_table.insert( 0x2A, new opcode("LDI A, (HL)", 1, 8, &cpu::ldi_a_hl) );              /* 0x2A - LDI A, (HL) */
    opcodes_table.insert( 0x2B, new opcode("DEC HL", 1, 8, &cpu::dec_hl) );                     /* 0x2B - DEC HL */
    opcodes_table.insert( 0x2C, new opcode("INC L", 1, 4, &cpu::inc_l) );                       /* 0x2C - INC L */
    opcodes_table.insert( 0x2D, new opcode("DEC L", 1, 4, &cpu::dec_l) );                       /* 0x2D - DEC L */
    opcodes_table.insert( 0x2E, new opcode("LD L, d8", 2, 8, &cpu::ld_l_d8) );                  /* 0x2E - LD L, d8 */
    opcodes_table.insert( 0x2F, new opcode("CPL", 1, 4, &cpu::cpl) );                           /* 0x2F - CPL */

    opcodes_table.insert( 0x30, new opcode("JR NC,r8", 2, 12, &cpu::jr_nc_r8, 8) );             /* 0x30 - JR NC, r8 */
    opcodes_table.insert( 0x31, new opcode("LD SP,d16", 3, 12, &cpu::ld_sp_d16) );              /* 0x31 - LD SP, d16 */
    opcodes_table.insert( 0x32, new opcode("LDD (HL),A", 1, 8, &cpu::ldd_hl_a) );               /* 0x32 - LDD (HL), A */
    opcodes_table.insert( 0x33, new opcode("INC SP", 1, 8, &cpu::inc_sp) );                     /* 0x33 - INC SP */
    opcodes_table.insert( 0x34, new opcode("INC (HL)", 1, 12, &cpu::inc_hl_) );                 /* 0x34 - INC (HL) */
    opcodes_table.insert( 0x35, new opcode("DEC (HL)", 1, 12, &cpu::dec_hl_) );                 /* 0x35 - DEC (HL) */
    opcodes_table.insert( 0x36, new opcode("LD (HL),d8", 2, 12, &cpu::ld_hl_d8) );              /* 0x36 - LD (HL), d8 */
    opcodes_table.insert( 0x37, new opcode("SCF", 1, 4, &cpu::scf) );                           /* 0x37 - SCF */
    opcodes_table.insert( 0x38, new opcode("JR C,r8", 2, 12, &cpu::jr_c_r8, 8) );               /* 0x38 - JR C, r8 */
    opcodes_table.insert( 0x39, new opcode("ADD HL, SP", 1, 8, &cpu::add_hl_sp) );              /* 0x39 - ADD HL, SP */
    opcodes_table.insert( 0x3A, new opcode("LDD A, (HL)", 1, 8, &cpu::ldd_a_hl) );              /* 0x3A - LDD A, (HL) */
    opcodes_table.insert( 0x3B, new opcode("DEC SP", 1, 8, &cpu::dec_sp) );                     /* 0x3B - DEC SP */
    opcodes_table.insert( 0x3C, new opcode("INC A", 1, 4, &cpu::inc_a) );                       /* 0x3C - INC A */
    opcodes_table.insert( 0x3D, new opcode("DEC A", 1, 4, &cpu::dec_a) );                       /* 0x3D - DEC A */
    opcodes_table.insert( 0x3E, new opcode("LD A, d8", 2, 8, &cpu::ld_a_d8) );                  /* 0x3E - LD A, d8 */
    opcodes_table.insert( 0x3F, new opcode("CCF", 1, 4, &cpu::ccf) );                           /* 0x3F - CCF */

    opcodes_table.insert( 0x40, new opcode("LD B, B", 1, 4, &cpu::ld_b_b) );                    /* 0x40 - LD B, B */
    opcodes_table.insert( 0x41, new opcode("LD B, C", 1, 4, &cpu::ld_b_c) );                    /* 0x41 - LD B, C */
    opcodes_table.insert( 0x42, new opcode("LD B, D", 1, 4, &cpu::ld_b_d) );                    /* 0x42 - LD B, D */
    opcodes_table.insert( 0x43, new opcode("LD B, E", 1, 4, &cpu::ld_b_e) );                    /* 0x43 - LD B, E */
    opcodes_table.insert( 0x44, new opcode("LD B, H", 1, 4, &cpu::ld_b_h) );                    /* 0x44 - LD B, H */
    opcodes_table.insert( 0x45, new opcode("LD B, L", 1, 4, &cpu::ld_b_l) );                    /* 0x45 - LD B, L */
    opcodes_table.insert( 0x46, new opcode("LD B, (HL)", 1, 8, &cpu::ld_b_hl) );                /* 0x46 - LD B, (HL) */
    opcodes_table.insert( 0x47, new opcode("LD B, A", 1, 4, &cpu::ld_b_a) );                    /* 0x47 - LD B, A */
    opcodes_table.insert( 0x48, new opcode("LD C, B", 1, 4, &cpu::ld_c_b) );                    /* 0x48 - LD C, B */
    opcodes_table.insert( 0x49, new opcode("LD C, C", 1, 4, &cpu::ld_c_c) );                    /* 0x49 - LD C, C */
    opcodes_table.insert( 0x4A, new opcode("LD C, D", 1, 4, &cpu::ld_c_d) );                    /* 0x4A - LD C, D */
    opcodes_table.insert( 0x4B, new opcode("LD C, E", 1, 4, &cpu::ld_c_e) );                    /* 0x4B - LD C, E */
    opcodes_table.insert( 0x4C, new opcode("LD C, H", 1, 4, &cpu::ld_c_h) );                    /* 0x4C - LD C, H */
    opcodes_table.insert( 0x4D, new opcode("LD C, L", 1, 4, &cpu::ld_c_l) );                    /* 0x4D - LD C, L */
    opcodes_table.insert( 0x4E, new opcode("LD C, (HL)", 1, 8, &cpu::ld_c_hl) );                /* 0x4E - LD C, (HL) */
    opcodes_table.insert( 0x4F, new opcode("LD C, A", 1, 4, &cpu::ld_c_a) );                    /* 0x4F - LD C, A */

    opcodes_table.insert( 0x50, new opcode("LD D, B", 1, 4, &cpu::ld_d_b) );                    /* 0x50 - LD D, B */
    opcodes_table.insert( 0x51, new opcode("LD D, C", 1, 4, &cpu::ld_d_c) );                    /* 0x51 - LD D, C */
    opcodes_table.insert( 0x52, new opcode("LD D, D", 1, 4, &cpu::ld_d_d) );                    /* 0x52 - LD D, D */
    opcodes_table.insert( 0x53, new opcode("LD D, E", 1, 4, &cpu::ld_d_e) );                    /* 0x53 - LD D, E */
    opcodes_table.insert( 0x54, new opcode("LD D, H", 1, 4, &cpu::ld_d_h) );                    /* 0x54 - LD D, H */
    opcodes_table.insert( 0x55, new opcode("LD D, L", 1, 4, &cpu::ld_d_l) );                    /* 0x55 - LD D, L */
    opcodes_table.insert( 0x56, new opcode("LD D, (HL)", 1, 8, &cpu::ld_d_hl) );                /* 0x56 - LD D, (HL) */
    opcodes_table.insert( 0x57, new opcode("LD D, A", 1, 4, &cpu::ld_d_a) );                    /* 0x57 - LD D, A */
    opcodes_table.insert( 0x58, new opcode("LD E, B", 1, 4, &cpu::ld_e_b) );                    /* 0x58 - LD E, B */
    opcodes_table.insert( 0x59, new opcode("LD E, C", 1, 4, &cpu::ld_e_c) );                    /* 0x59 - LD E, C */
    opcodes_table.insert( 0x5A, new opcode("LD E, D", 1, 4, &cpu::ld_e_d) );                    /* 0x5A - LD E, D */
    opcodes_table.insert( 0x5B, new opcode("LD E, E", 1, 4, &cpu::ld_e_e) );                    /* 0x5B - LD E, E */
    opcodes_table.insert( 0x5C, new opcode("LD E, H", 1, 4, &cpu::ld_e_h) );                    /* 0x5C - LD E, H */
    opcodes_table.insert( 0x5D, new opcode("LD E, L", 1, 4, &cpu::ld_e_l) );                    /* 0x5D - LD E, L */
    opcodes_table.insert( 0x5E, new opcode("LD E, (HL)", 1, 8, &cpu::ld_e_hl) );                /* 0x5E - LD E, (HL) */
    opcodes_table.insert( 0x5F, new opcode("LD E, A", 1, 4, &cpu::ld_e_a) );                    /* 0x5F - LD E, A */

    opcodes_table.insert( 0x60, new opcode("LD H, B", 1, 4, &cpu::ld_h_b) );                    /* 0x60 - LD H, B */
    opcodes_table.insert( 0x61, new opcode("LD H, C", 1, 4, &cpu::ld_h_c) );                    /* 0x61 - LD H, C */
    opcodes_table.insert( 0x62, new opcode("LD H, D", 1, 4, &cpu::ld_h_d) );                    /* 0x62 - LD H, D */
    opcodes_table.insert( 0x63, new opcode("LD H, E", 1, 4, &cpu::ld_h_e) );                    /* 0x63 - LD H, E */
    opcodes_table.insert( 0x64, new opcode("LD H, H", 1, 4, &cpu::ld_h_h) );                    /* 0x64 - LD H, H */
    opcodes_table.insert( 0x65, new opcode("LD H, L", 1, 4, &cpu::ld_h_l) );                    /* 0x65 - LD H, L */
    opcodes_table.insert( 0x66, new opcode("LD H, (HL)", 1, 8, &cpu::ld_h_hl) );                /* 0x66 - LD H, (HL) */
    opcodes_table.insert( 0x67, new opcode("LD H, A", 1, 4, &cpu::ld_h_a) );                    /* 0x67 - LD H, A */
    opcodes_table.insert( 0x68, new opcode("LD L, B", 1, 4, &cpu::ld_l_b) );                    /* 0x68 - LD L, B */
    opcodes_table.insert( 0x69, new opcode("LD L, C", 1, 4, &cpu::ld_l_c) );                    /* 0x69 - LD L, C */
    opcodes_table.insert( 0x6A, new opcode("LD L, D", 1, 4, &cpu::ld_l_d) );                    /* 0x6A - LD L, D */
    opcodes_table.insert( 0x6B, new opcode("LD L, E", 1, 4, &cpu::ld_l_e) );                    /* 0x6B - LD L, E */
    opcodes_table.insert( 0x6C, new opcode("LD L, H", 1, 4, &cpu::ld_l_h) );                    /* 0x6C - LD L, H */
    opcodes_table.insert( 0x6D, new opcode("LD L, L", 1, 4, &cpu::ld_l_l) );                    /* 0x6D - LD L, L */
    opcodes_table.insert( 0x6E, new opcode("LD L, (HL)", 1, 8, &cpu::ld_l_hl) );                /* 0x6E - LD L, (HL) */
    opcodes_table.insert( 0x6F, new opcode("LD L, A", 1, 4, &cpu::ld_l_a) );                    /* 0x6F - LD L, A */

    opcodes_table.insert( 0x70, new opcode("LD (HL), B", 1, 8, &cpu::ld_hl_b) );                /* 0x70 - LD (HL), B */
    opcodes_table.insert( 0x71, new opcode("LD (HL), C", 1, 8, &cpu::ld_hl_c) );                /* 0x71 - LD (HL), C */
    opcodes_table.insert( 0x72, new opcode("LD (HL), D", 1, 8, &cpu::ld_hl_d) );                /* 0x72 - LD (HL), D */
    opcodes_table.insert( 0x73, new opcode("LD (HL), E", 1, 8, &cpu::ld_hl_e) );                /* 0x73 - LD (HL), E */
    opcodes_table.insert( 0x74, new opcode("LD (HL), H", 1, 8, &cpu::ld_hl_h) );                /* 0x74 - LD (HL), H */
    opcodes_table.insert( 0x75, new opcode("LD (HL), L", 1, 8, &cpu::ld_hl_l) );                /* 0x75 - LD (HL), L */
    opcodes_table.insert( 0x76, new opcode("HALT", 1, 4, &cpu::halt) );                         /* 0x76 - HALT */
    opcodes_table.insert( 0x77, new opcode("LD (HL), A", 1, 8, &cpu::ld_hl_a) );                /* 0x77 - LD (HL), A */
    opcodes_table.insert( 0x78, new opcode("LD A, B", 1, 4, &cpu::ld_a_b) );                    /* 0x78 - LD A, B */
    opcodes_table.insert( 0x79, new opcode("LD A, C", 1, 4, &cpu::ld_a_c) );                    /* 0x79 - LD A, C */
    opcodes_table.insert( 0x7A, new opcode("LD A, D", 1, 4, &cpu::ld_a_d) );                    /* 0x7A - LD A, D */
    opcodes_table.insert( 0x7B, new opcode("LD A, E", 1, 4, &cpu::ld_a_e) );                    /* 0x7B - LD A, E */
    opcodes_table.insert( 0x7C, new opcode("LD A, H", 1, 4, &cpu::ld_a_h) );                    /* 0x7C - LD A, H */
    opcodes_table.insert( 0x7D, new opcode("LD A, L", 1, 4, &cpu::ld_a_l) );                    /* 0x7D - LD A, L */
    opcodes_table.insert( 0x7E, new opcode("LD A, (HL)", 1, 8, &cpu::ld_a_hl) );                /* 0x7E - LD A, (HL) */
    opcodes_table.insert( 0x7F, new opcode("LD A, A", 1, 4, &cpu::ld_a_a) );                    /* 0x7F - LD A, A */

    opcodes_table.insert( 0x80, new opcode("ADD A, B", 1, 4, &cpu::add_a_b) );                  /* 0x80 - ADD A, B */
    opcodes_table.insert( 0x81, new opcode("ADD A, C", 1, 4, &cpu::add_a_c) );                  /* 0x81 - ADD A, C */
    opcodes_table.insert( 0x82, new opcode("ADD A, D", 1, 4, &cpu::add_a_d) );                  /* 0x82 - ADD A, D */
    opcodes_table.insert( 0x83, new opcode("ADD A, E", 1, 4, &cpu::add_a_e) );                  /* 0x83 - ADD A, E */
    opcodes_table.insert( 0x84, new opcode("ADD A, H", 1, 4, &cpu::add_a_h) );                  /* 0x84 - ADD A, H */
    opcodes_table.insert( 0x85, new opcode("ADD A, L", 1, 4, &cpu::add_a_l) );                  /* 0x85 - ADD A, L */
    opcodes_table.insert( 0x86, new opcode("ADD A, (HL)", 1, 8, &cpu::add_a_hl) );              /* 0x86 - ADD A, (HL) */
    opcodes_table.insert( 0x87, new opcode("ADD A, A", 1, 4, &cpu::add_a_a) );                  /* 0x87 - ADD A, A */
    opcodes_table.insert( 0x88, new opcode("ADC A, B", 1, 4, &cpu::adc_a_b) );                  /* 0x88 - ADC A, B */
    opcodes_table.insert( 0x89, new opcode("ADC A, C", 1, 4, &cpu::adc_a_c) );                  /* 0x89 - ADC A, C */
    opcodes_table.insert( 0x8A, new opcode("ADC A, D", 1, 4, &cpu::adc_a_d) );                  /* 0x8A - ADC A, D */
    opcodes_table.insert( 0x8B, new opcode("ADC A, E", 1, 4, &cpu::adc_a_e) );                  /* 0x8B - ADC A, E */
    opcodes_table.insert( 0x8C, new opcode("ADC A, H", 1, 4, &cpu::adc_a_h) );                  /* 0x8C - ADC A, H */
    opcodes_table.insert( 0x8D, new opcode("ADC A, L", 1, 4, &cpu::adc_a_l) );                  /* 0x8D - ADC A, L */
    opcodes_table.insert( 0x8E, new opcode("ADC A, (HL)", 1, 8, &cpu::adc_a_hl) );              /* 0x8E - ADC A, (HL) */
    opcodes_table.insert( 0x8F, new opcode("ADC A, A", 1, 4, &cpu::adc_a_a) );                  /* 0x8F - ADC A, A */

    opcodes_table.insert( 0x90, new opcode("SUB B", 1, 4, &cpu::sub_b) );                       /* 0x90 - SUB B */
    opcodes_table.insert( 0x91, new opcode("SUB C", 1, 4, &cpu::sub_c) );                       /* 0x91 - SUB C */
    opcodes_table.insert( 0x92, new opcode("SUB D", 1, 4, &cpu::sub_d) );                       /* 0x92 - SUB D */
    opcodes_table.insert( 0x93, new opcode("SUB E", 1, 4, &cpu::sub_e) );                       /* 0x93 - SUB E */
    opcodes_table.insert( 0x94, new opcode("SUB H", 1, 4, &cpu::sub_h) );                       /* 0x94 - SUB H */
    opcodes_table.insert( 0x95, new opcode("SUB L", 1, 4, &cpu::sub_l) );                       /* 0x95 - SUB L */
    opcodes_table.insert( 0x96, new opcode("SUB (HL)", 1, 8, &cpu::sub_hl) );                   /* 0x96 - SUB (HL) */
    opcodes_table.insert( 0x97, new opcode("SUB A", 1, 4, &cpu::sub_a) );                       /* 0x97 - SUB A */
    opcodes_table.insert( 0x98, new opcode("SBC A, B", 1, 4, &cpu::sbc_a_b) );                  /* 0x98 - SBC A, B */
    opcodes_table.insert( 0x99, new opcode("SBC A, C", 1, 4, &cpu::sbc_a_c) );                  /* 0x99 - SBC A, C */
    opcodes_table.insert( 0x9A, new opcode("SBC A, D", 1, 4, &cpu::sbc_a_d) );                  /* 0x9A - SBC A, D */
    opcodes_table.insert( 0x9B, new opcode("SBC A, E", 1, 4, &cpu::sbc_a_e) );                  /* 0x9B - SBC A, E */
    opcodes_table.insert( 0x9C, new opcode("SBC A, H", 1, 4, &cpu::sbc_a_h) );                  /* 0x9C - SBC A, H */
    opcodes_table.insert( 0x9D, new opcode("SBC A, L", 1, 4, &cpu::sbc_a_l) );                  /* 0x9D - SBC A, L */
    opcodes_table.insert( 0x9E, new opcode("SBC A, (HL)", 1, 8, &cpu::sbc_a_hl) );              /* 0x9E - SBC A, (HL) */
    opcodes_table.insert( 0x9F, new opcode("SBC A, A", 1, 4, &cpu::sbc_a_a) );                  /* 0x9F - SBC A, A */

    opcodes_table.insert( 0xA0, new opcode("AND B", 1, 4, &cpu::and_b) );                       /* 0xA0 - AND B */
    opcodes_table.insert( 0xA1, new opcode("AND C", 1, 4, &cpu::and_c) );                       /* 0xA1 - AND C */
    opcodes_table.insert( 0xA2, new opcode("AND D", 1, 4, &cpu::and_d) );                       /* 0xA2 - AND D */
    opcodes_table.insert( 0xA3, new opcode("AND E", 1, 4, &cpu::and_e) );                       /* 0xA3 - AND E */
    opcodes_table.insert( 0xA4, new opcode("AND H", 1, 4, &cpu::and_h) );                       /* 0xA4 - AND H */
    opcodes_table.insert( 0xA5, new opcode("AND L", 1, 4, &cpu::and_l) );                       /* 0xA5 - AND L */
    opcodes_table.insert( 0xA6, new opcode("AND (HL)", 1, 8, &cpu::and_hl) );                   /* 0xA6 - AND (HL) */
    opcodes_table.insert( 0xA7, new opcode("AND A", 1, 4, &cpu::and_a) );                       /* 0xA7 - AND A */
    opcodes_table.insert( 0xA8, new opcode("XOR B", 1, 4, &cpu::xor_b) );                       /* 0xA8 - XOR B */
    opcodes_table.insert( 0xA9, new opcode("XOR C", 1, 4, &cpu::xor_c) );                       /* 0xA9 - XOR C */
    opcodes_table.insert( 0xAA, new opcode("XOR D", 1, 4, &cpu::xor_d) );                       /* 0xAA - XOR D */
    opcodes_table.insert( 0xAB, new opcode("XOR E", 1, 4, &cpu::xor_e) );                       /* 0xAB - XOR E */
    opcodes_table.insert( 0xAC, new opcode("XOR H", 1, 4, &cpu::xor_h) );                       /* 0xAC - XOR H */
    opcodes_table.insert( 0xAD, new opcode("XOR L", 1, 4, &cpu::xor_l) );                       /* 0xAD - XOR L */
    opcodes_table.insert( 0xAE, new opcode("XOR (HL)", 1, 8, &cpu::xor_hl) );                   /* 0xAE - XOR (HL) */
    opcodes_table.insert( 0xAF, new opcode("XOR A", 1, 4, &cpu::xor_a) );                       /* 0xAF - XOR A */

    opcodes_table.insert( 0xB0, new opcode("OR B", 1, 4, &cpu::or_b) );                         /* 0xB0 - OR B */
    opcodes_table.insert( 0xB1, new opcode("OR C", 1, 4, &cpu::or_c) );                         /* 0xB1 - OR C */
    opcodes_table.insert( 0xB2, new opcode("OR D", 1, 4, &cpu::or_d) );                         /* 0xB2 - OR D */
    opcodes_table.insert( 0xB3, new opcode("OR E", 1, 4, &cpu::or_e) );                         /* 0xB3 - OR E */
    opcodes_table.insert( 0xB4, new opcode("OR H", 1, 4, &cpu::or_h) );                         /* 0xB4 - OR H */
    opcodes_table.insert( 0xB5, new opcode("OR L", 1, 4, &cpu::or_l) );                         /* 0xB5 - OR L */
    opcodes_table.insert( 0xB6, new opcode("OR (HL)", 1, 8, &cpu::or_hl) );                     /* 0xB6 - OR (HL) */
    opcodes_table.insert( 0xB7, new opcode("OR A", 1, 4, &cpu::or_a) );                         /* 0xB7 - OR A */
    opcodes_table.insert( 0xB8, new opcode("CP B", 1, 4, &cpu::cp_b) );                         /* 0xB8 - CP B */
    opcodes_table.insert( 0xB9, new opcode("CP C", 1, 4, &cpu::cp_c) );                         /* 0xB9 - CP C */
    opcodes_table.insert( 0xBA, new opcode("CP D", 1, 4, &cpu::cp_d) );                         /* 0xBA - CP D */
    opcodes_table.insert( 0xBB, new opcode("CP E", 1, 4, &cpu::cp_e) );                         /* 0xBB - CP E */
    opcodes_table.insert( 0xBC, new opcode("CP H", 1, 4, &cpu::cp_h) );                         /* 0xBC - CP H */
    opcodes_table.insert( 0xBD, new opcode("CP L", 1, 4, &cpu::cp_l) );                         /* 0xBD - CP L */
    opcodes_table.insert( 0xBE, new opcode("CP (HL)", 1, 8, &cpu::cp_hl) );                     /* 0xBE - CP (HL) */
    opcodes_table.insert( 0xBF, new opcode("CP A", 1, 4, &cpu::cp_a) );                         /* 0xBF - CP A */

    opcodes_table.insert( 0xC0, new opcode("RET NZ", 1, 20, &cpu::ret_nz, 8) );                 /* 0xC0 - RET NZ */
    opcodes_table.insert( 0xC1, new opcode("POP BC", 1, 12, &cpu::pop_bc) );                    /* 0xC1 - POP BC */
    opcodes_table.insert( 0xC2, new opcode("JP NZ, a16", 3, 16, &cpu::jp_nz_a16, 12) );         /* 0xC2 - JP NZ, a16 */
    opcodes_table.insert( 0xC3, new opcode("JP a16", 3, 16, &cpu::jp_a16) );                    /* 0xC3 - JP a16 */
    opcodes_table.insert( 0xC4, new opcode("CALL NZ, a16", 3, 24, &cpu::call_nz_a16, 12) );     /* 0xC4 - CALL NZ, a16 */
    opcodes_table.insert( 0xC5, new opcode("PUSH BC", 1, 16, &cpu::push_bc) );                  /* 0xC5 - PUSH BC */
    opcodes_table.insert( 0xC6, new opcode("ADD A, d8", 2, 8, &cpu::add_a_d8) );                /* 0xC6 - ADD A, d8 */
    opcodes_table.insert( 0xC7, new opcode("RST 00h", 1, 16, &cpu::rst_00h) );                  /* 0xC7 - RST 00h */
    opcodes_table.insert( 0xC8, new opcode("RET Z", 1, 20, &cpu::ret_z, 8) );                   /* 0xC8 - RET Z */
    opcodes_table.insert( 0xC9, new opcode("RET", 1, 16, &cpu::ret) );                          /* 0xC9 - RET */
    opcodes_table.insert( 0xCA, new opcode("JP Z, a16", 3, 16, &cpu::jp_z_a16, 12) );           /* 0xCA - JP Z, a16 */
    opcodes_table.insert( 0xCB, new opcode("PREFIX CB", 1, 4, &cpu::prefix_cb) );               /* 0xCB - PREFIX CB */
    opcodes_table.insert( 0xCC, new opcode("CALL Z, a16", 3, 24, &cpu::call_z_a16, 12) );       /* 0xCC - CALL Z, a16 */
    opcodes_table.insert( 0xCD, new opcode("CALL a16", 3, 24, &cpu::call_a16) );                /* 0xCD - CALL a16 */
    opcodes_table.insert( 0xCE, new opcode("ADC A, d8", 2, 8, &cpu::adc_a_d8) );                /* 0xCE - ADC A, d8 */
    opcodes_table.insert( 0xCF, new opcode("RST 08h", 1, 16, &cpu::rst_08h) );                  /* 0xCF - RST 08h */

    opcodes_table.insert( 0xD0, new opcode("RET NC", 1, 20, &cpu::ret_nc, 8) );                 /* 0xD0 - RET NC */
    opcodes_table.insert( 0xD1, new opcode("POP DE", 1, 12, &cpu::pop_de) );                    /* 0xD1 - POP DE */
    opcodes_table.insert( 0xD2, new opcode("JP NC, a16", 3, 16, &cpu::jp_nc_a16, 12) );         /* 0xD2 - JP NC, a16 */
    opcodes_table.insert( 0xD3, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xD3 - NOT IMPL */
    opcodes_table.insert( 0xD4, new opcode("CALL NC, a16", 3, 24, &cpu::call_nc_a16, 12) );     /* 0xD4 - CALL NC, a16 */
    opcodes_table.insert( 0xD5, new opcode("PUSH DE", 1, 16, &cpu::push_de) );                  /* 0xD5 - PUSH DE */
    opcodes_table.insert( 0xD6, new opcode("SUB d8", 2, 8, &cpu::sub_d8) );                     /* 0xD6 - SUB d8 */
    opcodes_table.insert( 0xD7, new opcode("RST 10h", 1, 16, &cpu::rst_10h) );                  /* 0xD7 - RST 10h */
    opcodes_table.insert( 0xD8, new opcode("RET C", 1, 20, &cpu::ret_c, 8) );                   /* 0xD8 - RET C */
    opcodes_table.insert( 0xD9, new opcode("RETI", 1, 16, &cpu::reti) );                        /* 0xD9 - RETI */
    opcodes_table.insert( 0xDA, new opcode("JP C, a16", 3, 16, &cpu::jp_c_a16, 12) );           /* 0xDA - JP C, a16 */
    opcodes_table.insert( 0xDB, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xDB - NOT IMPL */
    opcodes_table.insert( 0xDC, new opcode("CALL C, a16", 3, 24, &cpu::call_c_a16, 12) );       /* 0xDC - CALL C, a16 */
    opcodes_table.insert( 0xDD, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xDD - NOT IMPL */
    opcodes_table.insert( 0xDE, new opcode("SBC A, d8", 2, 8, &cpu::sbc_a_d8) );                /* 0xDE - SBC A, d8 */
    opcodes_table.insert( 0xDF, new opcode("RST 18h", 1, 16, &cpu::rst_18h) );                  /* 0xDF - RST 18h */

    opcodes_table.insert( 0xE0, new opcode("LDH (a8), A", 2, 12, &cpu::ldh_a8_a) );             /* 0xE0 - LDH (a8), A */
    opcodes_table.insert( 0xE1, new opcode("POP HL", 1, 12, &cpu::pop_hl) );                    /* 0xE1 - POP HL */
    opcodes_table.insert( 0xE2, new opcode("LD (C), A", 2, 8, &cpu::ld_c_a_) );                 /* 0xE2 - LD (C), A */
    opcodes_table.insert( 0xE3, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xE3 - NOT IMPL */
    opcodes_table.insert( 0xE4, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xE4 - NOT IMPL */
    opcodes_table.insert( 0xE5, new opcode("PUSH HL", 1, 16, &cpu::push_hl) );                  /* 0xE5 - PUSH HL */
    opcodes_table.insert( 0xE6, new opcode("AND d8", 2, 8, &cpu::and_d8) );                     /* 0xE6 - AND d8 */
    opcodes_table.insert( 0xE7, new opcode("RST 20h", 1, 16, &cpu::rst_20h) );                  /* 0xE7 - RST 20h */
    opcodes_table.insert( 0xE8, new opcode("ADD SP, r8", 2, 16, &cpu::add_sp_r8) );             /* 0xE8 - ADD SP, r8 */
    opcodes_table.insert( 0xE9, new opcode("JP (HL)", 1, 4, &cpu::jp_hl) );                     /* 0xE9 - JP (HL) */
    opcodes_table.insert( 0xEA, new opcode("LD (a16), A", 3, 16, &cpu::ld_a16_a) );             /* 0xEA - LD (a16), A */
    opcodes_table.insert( 0xEB, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xEB - NOT IMPL */
    opcodes_table.insert( 0xEC, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xEC - NOT IMPL */
    opcodes_table.insert( 0xED, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xED - NOT IMPL */
    opcodes_table.insert( 0xEE, new opcode("XOR d8", 2, 8, &cpu::xor_d8) );                     /* 0xEE - XOR d8 */
    opcodes_table.insert( 0xEF, new opcode("RST 28h", 1, 16, &cpu::rst_28h) );                  /* 0xEF - RST 28h */

    opcodes_table.insert( 0xF0, new opcode("LDH A, (a8)", 2, 12, &cpu::ldh_a_a8) );             /* 0xF0 - LDH A, (a8) */
    opcodes_table.insert( 0xF1, new opcode("POP AF", 1, 12, &cpu::pop_af) );                    /* 0xF1 - POP AF */
    opcodes_table.insert( 0xF2, new opcode("LD A, (C)", 2, 8, &cpu::ld_a_c_) );                 /* 0xF2 - LD A, (C) */
    opcodes_table.insert( 0xF3, new opcode("DI", 1, 4, &cpu::di) );                             /* 0xF3 - DI */
    opcodes_table.insert( 0xF4, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xF4 - NOT IMPL */
    opcodes_table.insert( 0xF5, new opcode("PUSH AF", 1, 16, &cpu::push_af) );                  /* 0xF5 - PUSH AF */
    opcodes_table.insert( 0xF6, new opcode("OR d8", 2, 8, &cpu::or_d8) );                       /* 0xF6 - OR d8 */
    opcodes_table.insert( 0xF7, new opcode("RST 30h", 1, 16, &cpu::rst_30h) );                  /* 0xF7 - RST 30h */
    opcodes_table.insert( 0xF8, new opcode("LDHL SP, r8", 2, 12, &cpu::ldhl_sp_r8) );           /* 0xF8 - LDHL SP, r8 */
    opcodes_table.insert( 0xF9, new opcode("LD SP, HL", 1, 8, &cpu::ld_sp_hl) );                /* 0xF9 - LD SP, HL */
    opcodes_table.insert( 0xFA, new opcode("LD A, (a16)", 3, 16, &cpu::ld_a_a16) );             /* 0xFA - LD A, (a16) */
    opcodes_table.insert( 0xFB, new opcode("EI", 1, 4, &cpu::ei) );                             /* 0xFB - EI */
    opcodes_table.insert( 0xFC, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xFC - NOT IMPL */
    opcodes_table.insert( 0xFD, new opcode("NOT IMPL", 1, 1, &cpu::not_impl) );                 /* 0xFD - NOT IMPL */
    opcodes_table.insert( 0xFE, new opcode("CP d8", 2, 8, &cpu::cp_d8) );                       /* 0xFE - CP d8 */
    opcodes_table.insert( 0xFF, new opcode("RST 38h", 1, 16, &cpu::rst_38h) );                  /* 0xFF - RST 38h */

    //2 bytes-opcodes
    extended_opcodes_table.insert( 0x00, new opcode("RLC B", 2, 8, &cpu::rlc_b));               /* 0xCB00 - RLC B */
    extended_opcodes_table.insert( 0x01, new opcode("RLC C", 2, 8, &cpu::rlc_c));               /* 0xCB01 - RLC C */
    extended_opcodes_table.insert( 0x02, new opcode("RLC D", 2, 8, &cpu::rlc_d));               /* 0xCB02 - RLC D */
    extended_opcodes_table.insert( 0x03, new opcode("RLC E", 2, 8, &cpu::rlc_e));               /* 0xCB03 - RLC E */
    extended_opcodes_table.insert( 0x04, new opcode("RLC H", 2, 8, &cpu::rlc_h));               /* 0xCB04 - RLC H */
    extended_opcodes_table.insert( 0x05, new opcode("RLC L", 2, 8, &cpu::rlc_l));               /* 0xCB05 - RLC L */
    extended_opcodes_table.insert( 0x06, new opcode("RLC (HL)", 2, 16, &cpu::rlc_hl));          /* 0xCB06 - RLC (HL) */
    extended_opcodes_table.insert( 0x07, new opcode("RLC A", 2, 8, &cpu::rlc_a));               /* 0xCB07 - RLC A */
    extended_opcodes_table.insert( 0x08, new opcode("RRC B", 2, 8, &cpu::rrc_b));               /* 0xCB08 - RRC B */
    extended_opcodes_table.insert( 0x09, new opcode("RRC C", 2, 8, &cpu::rrc_c));               /* 0xCB09 - RRC C */
    extended_opcodes_table.insert( 0x0A, new opcode("RRC D", 2, 8, &cpu::rrc_d));               /* 0xCB0A - RRC D */
    extended_opcodes_table.insert( 0x0B, new opcode("RRC E", 2, 8, &cpu::rrc_e));               /* 0xCB0B - RRC E */
    extended_opcodes_table.insert( 0x0C, new opcode("RRC H", 2, 8, &cpu::rrc_h));               /* 0xCB0C - RRC H */
    extended_opcodes_table.insert( 0x0D, new opcode("RRC L", 2, 8, &cpu::rrc_l));               /* 0xCB0D - RRC L */
    extended_opcodes_table.insert( 0x0E, new opcode("RRC (HL)", 2, 16, &cpu::rrc_hl));          /* 0xCB0E - RRC (HL) */
    extended_opcodes_table.insert( 0x0F, new opcode("RRC A", 2, 8, &cpu::rrc_a));               /* 0xCB0F - RRC A */

    extended_opcodes_table.insert( 0x10, new opcode("RL B", 2, 8, &cpu::rl_b));                 /* 0xCB10 - RL B */
    extended_opcodes_table.insert( 0x11, new opcode("RL C", 2, 8, &cpu::rl_c));                 /* 0xCB11 - RL C */
    extended_opcodes_table.insert( 0x12, new opcode("RL D", 2, 8, &cpu::rl_d));                 /* 0xCB12 - RL D */
    extended_opcodes_table.insert( 0x13, new opcode("RL E", 2, 8, &cpu::rl_e));                 /* 0xCB13 - RL E */
    extended_opcodes_table.insert( 0x14, new opcode("RL H", 2, 8, &cpu::rl_h));                 /* 0xCB14 - RL H */
    extended_opcodes_table.insert( 0x15, new opcode("RL L", 2, 8, &cpu::rl_l));                 /* 0xCB15 - RL L */
    extended_opcodes_table.insert( 0x16, new opcode("RL (HL)", 2, 16, &cpu::rl_hl));            /* 0xCB16 - RL (HL) */
    extended_opcodes_table.insert( 0x17, new opcode("RL A", 2, 8, &cpu::rl_a));                 /* 0xCB17 - RL A */
    extended_opcodes_table.insert( 0x18, new opcode("RR B", 2, 8, &cpu::rr_b));                 /* 0xCB18 - RR B */
    extended_opcodes_table.insert( 0x19, new opcode("RR C", 2, 8, &cpu::rr_c));                 /* 0xCB19 - RR C */
    extended_opcodes_table.insert( 0x1A, new opcode("RR D", 2, 8, &cpu::rr_d));                 /* 0xCB1A - RR D */
    extended_opcodes_table.insert( 0x1B, new opcode("RR E", 2, 8, &cpu::rr_e));                 /* 0xCB1B - RR E */
    extended_opcodes_table.insert( 0x1C, new opcode("RR H", 2, 8, &cpu::rr_h));                 /* 0xCB1C - RR H */
    extended_opcodes_table.insert( 0x1D, new opcode("RR L", 2, 8, &cpu::rr_l));                 /* 0xCB1D - RR L */
    extended_opcodes_table.insert( 0x1E, new opcode("RR (HL)", 2, 16, &cpu::rr_hl));            /* 0xCB1E - RR (HL) */
    extended_opcodes_table.insert( 0x1F, new opcode("RR A", 2, 8, &cpu::rr_a));                 /* 0xCB1F - RR A */

    extended_opcodes_table.insert( 0x20, new opcode("SLA B", 2, 8, &cpu::sla_b));               /* 0xCB20 - SLA B */
    extended_opcodes_table.insert( 0x21, new opcode("SLA C", 2, 8, &cpu::sla_c));               /* 0xCB21 - SLA C */
    extended_opcodes_table.insert( 0x22, new opcode("SLA D", 2, 8, &cpu::sla_d));               /* 0xCB22 - SLA D */
    extended_opcodes_table.insert( 0x23, new opcode("SLA E", 2, 8, &cpu::sla_e));               /* 0xCB23 - SLA E */
    extended_opcodes_table.insert( 0x24, new opcode("SLA H", 2, 8, &cpu::sla_h));               /* 0xCB24 - SLA H */
    extended_opcodes_table.insert( 0x25, new opcode("SLA L", 2, 8, &cpu::sla_l));               /* 0xCB25 - SLA L */
    extended_opcodes_table.insert( 0x26, new opcode("SLA (HL)", 2, 16, &cpu::sla_hl));          /* 0xCB26 - SLA (HL) */
    extended_opcodes_table.insert( 0x27, new opcode("SLA A", 2, 8, &cpu::sla_a));               /* 0xCB27 - SLA A */
    extended_opcodes_table.insert( 0x28, new opcode("SRA B", 2, 8, &cpu::sra_b));               /* 0xCB28 - SRA B */
    extended_opcodes_table.insert( 0x29, new opcode("SRA C", 2, 8, &cpu::sra_c));               /* 0xCB29 - SRA C */
    extended_opcodes_table.insert( 0x2A, new opcode("SRA D", 2, 8, &cpu::sra_d));               /* 0xCB2A - SRA D */
    extended_opcodes_table.insert( 0x2B, new opcode("SRA E", 2, 8, &cpu::sra_e));               /* 0xCB2B - SRA E */
    extended_opcodes_table.insert( 0x2C, new opcode("SRA H", 2, 8, &cpu::sra_h));               /* 0xCB2C - SRA H */
    extended_opcodes_table.insert( 0x2D, new opcode("SRA L", 2, 8, &cpu::sra_l));               /* 0xCB2D - SRA L */
    extended_opcodes_table.insert( 0x2E, new opcode("SRA (HL)", 2, 16, &cpu::sra_hl));          /* 0xCB2E - SRA (HL) */
    extended_opcodes_table.insert( 0x2F, new opcode("SRA A", 2, 8, &cpu::sra_a));               /* 0xCB2F - SRA A */

    extended_opcodes_table.insert( 0x30, new opcode("SWAP B", 2, 8, &cpu::swap_b));             /* 0xCB30 - SWAP B */
    extended_opcodes_table.insert( 0x31, new opcode("SWAP C", 2, 8, &cpu::swap_c));             /* 0xCB31 - SWAP C */
    extended_opcodes_table.insert( 0x32, new opcode("SWAP D", 2, 8, &cpu::swap_d));             /* 0xCB32 - SWAP D */
    extended_opcodes_table.insert( 0x33, new opcode("SWAP E", 2, 8, &cpu::swap_e));             /* 0xCB33 - SWAP E */
    extended_opcodes_table.insert( 0x34, new opcode("SWAP H", 2, 8, &cpu::swap_h));             /* 0xCB34 - SWAP H */
    extended_opcodes_table.insert( 0x35, new opcode("SWAP L", 2, 8, &cpu::swap_l));             /* 0xCB35 - SWAP L */
    extended_opcodes_table.insert( 0x36, new opcode("SWAP (HL)", 2, 16, &cpu::swap_hl));        /* 0xCB36 - SWAP (HL) */
    extended_opcodes_table.insert( 0x37, new opcode("SWAP A", 2, 8, &cpu::swap_a));             /* 0xCB37 - SWAP A */
    extended_opcodes_table.insert( 0x38, new opcode("SRL B", 2, 8, &cpu::srl_b));               /* 0xCB38 - SRL B */
    extended_opcodes_table.insert( 0x39, new opcode("SRL C", 2, 8, &cpu::srl_c));               /* 0xCB39 - SRL C */
    extended_opcodes_table.insert( 0x3A, new opcode("SRL D", 2, 8, &cpu::srl_d));               /* 0xCB3A - SRL D */
    extended_opcodes_table.insert( 0x3B, new opcode("SRL E", 2, 8, &cpu::srl_e));               /* 0xCB3B - SRL E */
    extended_opcodes_table.insert( 0x3C, new opcode("SRL H", 2, 8, &cpu::srl_h));               /* 0xCB3C - SRL H */
    extended_opcodes_table.insert( 0x3D, new opcode("SRL L", 2, 8, &cpu::srl_l));               /* 0xCB3D - SRL L */
    extended_opcodes_table.insert( 0x3E, new opcode("SRL (HL)", 2, 16, &cpu::srl_hl));          /* 0xCB3E - SRL (HL) */
    extended_opcodes_table.insert( 0x3F, new opcode("SRL A", 2, 8, &cpu::srl_a));               /* 0xCB3F - SRL A */

    extended_opcodes_table.insert( 0x40, new opcode("BIT 0, B", 2, 8, &cpu::bit_0_b));          /* 0xCB40 - BIT 0, B */
    extended_opcodes_table.insert( 0x41, new opcode("BIT 0, C", 2, 8, &cpu::bit_0_c));          /* 0xCB41 - BIT 0, C */
    extended_opcodes_table.insert( 0x42, new opcode("BIT 0, D", 2, 8, &cpu::bit_0_d));          /* 0xCB42 - BIT 0, D */
    extended_opcodes_table.insert( 0x43, new opcode("BIT 0, E", 2, 8, &cpu::bit_0_e));          /* 0xCB43 - BIT 0, E */
    extended_opcodes_table.insert( 0x44, new opcode("BIT 0, H", 2, 8, &cpu::bit_0_h));          /* 0xCB44 - BIT 0, H */
    extended_opcodes_table.insert( 0x45, new opcode("BIT 0, L", 2, 8, &cpu::bit_0_l));          /* 0xCB45 - BIT 0, L */
    extended_opcodes_table.insert( 0x46, new opcode("BIT 0, (HL)", 2, 16, &cpu::bit_0_hl));     /* 0xCB46 - BIT 0, (HL) */
    extended_opcodes_table.insert( 0x47, new opcode("BIT 0, A", 2, 8, &cpu::bit_0_a));          /* 0xCB47 - BIT 0, A */
    extended_opcodes_table.insert( 0x48, new opcode("BIT 1, B", 2, 8, &cpu::bit_1_b));          /* 0xCB48 - BIT 1, B */
    extended_opcodes_table.insert( 0x49, new opcode("BIT 1, C", 2, 8, &cpu::bit_1_c));          /* 0xCB49 - BIT 1, C */
    extended_opcodes_table.insert( 0x4A, new opcode("BIT 1, D", 2, 8, &cpu::bit_1_d));          /* 0xCB4A - BIT 1, D */
    extended_opcodes_table.insert( 0x4B, new opcode("BIT 1, E", 2, 8, &cpu::bit_1_e));          /* 0xCB4B - BIT 1, E */
    extended_opcodes_table.insert( 0x4C, new opcode("BIT 1, H", 2, 8, &cpu::bit_1_h));          /* 0xCB4C - BIT 1, H */
    extended_opcodes_table.insert( 0x4D, new opcode("BIT 1, L", 2, 8, &cpu::bit_1_l));          /* 0xCB4D - BIT 1, L */
    extended_opcodes_table.insert( 0x4E, new opcode("BIT 1, (HL)", 2, 16, &cpu::bit_1_hl));     /* 0xCB4E - BIT 1, (HL) */
    extended_opcodes_table.insert( 0x4F, new opcode("BIT 1, A", 2, 8, &cpu::bit_1_a));          /* 0xCB4F - BIT 1, A */

    extended_opcodes_table.insert( 0x50, new opcode("BIT 2, B", 2, 8, &cpu::bit_2_b));          /* 0xCB50 - BIT 2, B */
    extended_opcodes_table.insert( 0x51, new opcode("BIT 2, C", 2, 8, &cpu::bit_2_c));          /* 0xCB51 - BIT 2, C */
    extended_opcodes_table.insert( 0x52, new opcode("BIT 2, D", 2, 8, &cpu::bit_2_d));          /* 0xCB52 - BIT 2, D */
    extended_opcodes_table.insert( 0x53, new opcode("BIT 2, E", 2, 8, &cpu::bit_2_e));          /* 0xCB53 - BIT 2, E */
    extended_opcodes_table.insert( 0x54, new opcode("BIT 2, H", 2, 8, &cpu::bit_2_h));          /* 0xCB54 - BIT 2, H */
    extended_opcodes_table.insert( 0x55, new opcode("BIT 2, L", 2, 8, &cpu::bit_2_l));          /* 0xCB55 - BIT 2, L */
    extended_opcodes_table.insert( 0x56, new opcode("BIT 2, (HL)", 2, 16, &cpu::bit_2_hl));     /* 0xCB56 - BIT 2, (HL) */
    extended_opcodes_table.insert( 0x57, new opcode("BIT 2, A", 2, 8, &cpu::bit_2_a));          /* 0xCB57 - BIT 2, A */
    extended_opcodes_table.insert( 0x58, new opcode("BIT 3, B", 2, 8, &cpu::bit_3_b));          /* 0xCB58 - BIT 3, B */
    extended_opcodes_table.insert( 0x59, new opcode("BIT 3, C", 2, 8, &cpu::bit_3_c));          /* 0xCB59 - BIT 3, C */
    extended_opcodes_table.insert( 0x5A, new opcode("BIT 3, D", 2, 8, &cpu::bit_3_d));          /* 0xCB5A - BIT 3, D */
    extended_opcodes_table.insert( 0x5B, new opcode("BIT 3, E", 2, 8, &cpu::bit_3_e));          /* 0xCB5B - BIT 3, E */
    extended_opcodes_table.insert( 0x5C, new opcode("BIT 3, H", 2, 8, &cpu::bit_3_h));          /* 0xCB5C - BIT 3, H */
    extended_opcodes_table.insert( 0x5D, new opcode("BIT 3, L", 2, 8, &cpu::bit_3_l));          /* 0xCB5D - BIT 3, L */
    extended_opcodes_table.insert( 0x5E, new opcode("BIT 3, (HL)", 2, 16, &cpu::bit_3_hl));     /* 0xCB5E - BIT 3, (HL) */
    extended_opcodes_table.insert( 0x5F, new opcode("BIT 3, A", 2, 8, &cpu::bit_3_a));          /* 0xCB5F - BIT 3, A */

    extended_opcodes_table.insert( 0x60, new opcode("BIT 4, B", 2, 8, &cpu::bit_4_b));          /* 0xCB60 - BIT 4, B */
    extended_opcodes_table.insert( 0x61, new opcode("BIT 4, C", 2, 8, &cpu::bit_4_c));          /* 0xCB61 - BIT 4, C */
    extended_opcodes_table.insert( 0x62, new opcode("BIT 4, D", 2, 8, &cpu::bit_4_d));          /* 0xCB62 - BIT 4, D */
    extended_opcodes_table.insert( 0x63, new opcode("BIT 4, E", 2, 8, &cpu::bit_4_e));          /* 0xCB63 - BIT 4, E */
    extended_opcodes_table.insert( 0x64, new opcode("BIT 4, H", 2, 8, &cpu::bit_4_h));          /* 0xCB64 - BIT 4, H */
    extended_opcodes_table.insert( 0x65, new opcode("BIT 4, L", 2, 8, &cpu::bit_4_l));          /* 0xCB65 - BIT 4, L */
    extended_opcodes_table.insert( 0x66, new opcode("BIT 4, (HL)", 2, 16, &cpu::bit_4_hl));     /* 0xCB66 - BIT 4, (HL) */
    extended_opcodes_table.insert( 0x67, new opcode("BIT 4, A", 2, 8, &cpu::bit_4_a));          /* 0xCB67 - BIT 4, A */
    extended_opcodes_table.insert( 0x68, new opcode("BIT 5, B", 2, 8, &cpu::bit_5_b));          /* 0xCB68 - BIT 5, B */
    extended_opcodes_table.insert( 0x69, new opcode("BIT 5, C", 2, 8, &cpu::bit_5_c));          /* 0xCB69 - BIT 5, C */
    extended_opcodes_table.insert( 0x6A, new opcode("BIT 5, D", 2, 8, &cpu::bit_5_d));          /* 0xCB6A - BIT 5, D */
    extended_opcodes_table.insert( 0x6B, new opcode("BIT 5, E", 2, 8, &cpu::bit_5_e));          /* 0xCB6B - BIT 5, E */
    extended_opcodes_table.insert( 0x6C, new opcode("BIT 5, H", 2, 8, &cpu::bit_5_h));          /* 0xCB6C - BIT 5, H */
    extended_opcodes_table.insert( 0x6D, new opcode("BIT 5, L", 2, 8, &cpu::bit_5_l));          /* 0xCB6D - BIT 5, L */
    extended_opcodes_table.insert( 0x6E, new opcode("BIT 5, (HL)", 2, 16, &cpu::bit_5_hl));     /* 0xCB6E - BIT 5, (HL) */
    extended_opcodes_table.insert( 0x6F, new opcode("BIT 5, A", 2, 8, &cpu::bit_5_a));          /* 0xCB6F - BIT 5, A */

    extended_opcodes_table.insert( 0x70, new opcode("BIT 6, B", 2, 8, &cpu::bit_6_b));          /* 0xCB70 - BIT 6, B */
    extended_opcodes_table.insert( 0x71, new opcode("BIT 6, C", 2, 8, &cpu::bit_6_c));          /* 0xCB71 - BIT 6, C */
    extended_opcodes_table.insert( 0x72, new opcode("BIT 6, D", 2, 8, &cpu::bit_6_d));          /* 0xCB72 - BIT 6, D */
    extended_opcodes_table.insert( 0x73, new opcode("BIT 6, E", 2, 8, &cpu::bit_6_e));          /* 0xCB73 - BIT 6, E */
    extended_opcodes_table.insert( 0x74, new opcode("BIT 6, H", 2, 8, &cpu::bit_6_h));          /* 0xCB74 - BIT 6, H */
    extended_opcodes_table.insert( 0x75, new opcode("BIT 6, L", 2, 8, &cpu::bit_6_l));          /* 0xCB75 - BIT 6, L */
    extended_opcodes_table.insert( 0x76, new opcode("BIT 6, (HL)", 2, 16, &cpu::bit_6_hl));     /* 0xCB76 - BIT 6, (HL) */
    extended_opcodes_table.insert( 0x77, new opcode("BIT 6, A", 2, 8, &cpu::bit_6_a));          /* 0xCB77 - BIT 6, A */
    extended_opcodes_table.insert( 0x78, new opcode("BIT 7, B", 2, 8, &cpu::bit_7_b));          /* 0xCB78 - BIT 7, B */
    extended_opcodes_table.insert( 0x79, new opcode("BIT 7, C", 2, 8, &cpu::bit_7_c));          /* 0xCB79 - BIT 7, C */
    extended_opcodes_table.insert( 0x7A, new opcode("BIT 7, D", 2, 8, &cpu::bit_7_d));          /* 0xCB7A - BIT 7, D */
    extended_opcodes_table.insert( 0x7B, new opcode("BIT 7, E", 2, 8, &cpu::bit_7_e));          /* 0xCB7B - BIT 7, E */
    extended_opcodes_table.insert( 0x7C, new opcode("BIT 7, H", 2, 8, &cpu::bit_7_h));          /* 0xCB7C - BIT 7, H */
    extended_opcodes_table.insert( 0x7D, new opcode("BIT 7, L", 2, 8, &cpu::bit_7_l));          /* 0xCB7D - BIT 7, L */
    extended_opcodes_table.insert( 0x7E, new opcode("BIT 7, (HL)", 2, 16, &cpu::bit_7_hl));     /* 0xCB7E - BIT 7, (HL) */
    extended_opcodes_table.insert( 0x7F, new opcode("BIT 7, A", 2, 8, &cpu::bit_7_a));          /* 0xCB7F - BIT 7, A */

    extended_opcodes_table.insert( 0x80, new opcode("RES 0, B", 2, 8, &cpu::res_0_b));          /* 0xCB80 - RES 0, B */
    extended_opcodes_table.insert( 0x81, new opcode("RES 0, C", 2, 8, &cpu::res_0_c));          /* 0xCB81 - RES 0, C */
    extended_opcodes_table.insert( 0x82, new opcode("RES 0, D", 2, 8, &cpu::res_0_d));          /* 0xCB82 - RES 0, D */
    extended_opcodes_table.insert( 0x83, new opcode("RES 0, E", 2, 8, &cpu::res_0_e));          /* 0xCB83 - RES 0, E */
    extended_opcodes_table.insert( 0x84, new opcode("RES 0, H", 2, 8, &cpu::res_0_h));          /* 0xCB84 - RES 0, H */
    extended_opcodes_table.insert( 0x85, new opcode("RES 0, L", 2, 8, &cpu::res_0_l));          /* 0xCB85 - RES 0, L */
    extended_opcodes_table.insert( 0x86, new opcode("RES 0, (HL)", 2, 16, &cpu::res_0_hl));     /* 0xCB86 - RES 0, (HL) */
    extended_opcodes_table.insert( 0x87, new opcode("RES 0, A", 2, 8, &cpu::res_0_a));          /* 0xCB87 - RES 0, A */
    extended_opcodes_table.insert( 0x88, new opcode("RES 1, B", 2, 8, &cpu::res_1_b));          /* 0xCB88 - RES 1, B */
    extended_opcodes_table.insert( 0x89, new opcode("RES 1, C", 2, 8, &cpu::res_1_c));          /* 0xCB89 - RES 1, C */
    extended_opcodes_table.insert( 0x8A, new opcode("RES 1, D", 2, 8, &cpu::res_1_d));          /* 0xCB8A - RES 1, D */
    extended_opcodes_table.insert( 0x8B, new opcode("RES 1, E", 2, 8, &cpu::res_1_e));          /* 0xCB8B - RES 1, E */
    extended_opcodes_table.insert( 0x8C, new opcode("RES 1, H", 2, 8, &cpu::res_1_h));          /* 0xCB8C - RES 1, H */
    extended_opcodes_table.insert( 0x8D, new opcode("RES 1, L", 2, 8, &cpu::res_1_l));          /* 0xCB8D - RES 1, L */
    extended_opcodes_table.insert( 0x8E, new opcode("RES 1, (HL)", 2, 16, &cpu::res_1_hl));     /* 0xCB8E - RES 1, (HL) */
    extended_opcodes_table.insert( 0x8F, new opcode("RES 1, A", 2, 8, &cpu::res_1_a));          /* 0xCB8F - RES 1, A */
}

cpu::~cpu()
{
    //delete opcodes
    QMap<quint8, opcode*>::iterator it;
    for(it = opcodes_table.begin() ; it != opcodes_table.end() ; ++it)
    {
        if(it.value()) delete it.value();
    }

    for(it = extended_opcodes_table.begin() ; it != extended_opcodes_table.end() ; ++it)
    {
        if(it.value()) delete it.value();
    }
}

bool cpu::interpret_opcode()
{
    if(HALT) return true;

    if(current_opcode != NULL)
    {
        if(!last_opcode_not_executed)
        {
            if(cycles_counter < current_opcode->cycles)
            {
                cycles_counter++;
                return true;
            }
        }
        else
        {
            if(cycles_counter < current_opcode->not_exec_cycles)
            {
                cycles_counter++;
                return true;
            }
        }
    }

    if(STOP) return false;

    last_opcode_not_executed = false;

    quint8 opcode_id = _MMU->rb(PC);
    const opcode* op = opcodes_table.value(opcode_id);

    memcpy(current_opcode, op, sizeof(opcode));

    (this->*(current_opcode->exec))();

    cycles_counter = 1;

    PC += current_opcode->length;

    return true;
}

quint16 cpu::get_pc()
{
    return PC;
}





void cpu::write_on_register(DOUBLE_REGISTERS reg, quint16 word)
{
    switch(reg)
    {
    case REGISTER_BC:
        _B = ((word & 0xFF00) >> 8);
        _C = word & 0xFF;
        break;

    case REGISTER_DE:
        _D = ((word & 0xFF00) >> 8);
        _E = word & 0xFF;
        break;

    case REGISTER_HL:
        _H = ((word & 0xFF00) >> 8);
        _L = word & 0xFF;
        break;
    }
}


/////////////////////////////////////
// FLAG FUNCTIONS
/////////////////////////////////////
bool cpu::z_flag()
{
    return F & FLAG_Z;
}

void cpu::check_z(quint16 val)
{
    if(!val) set_z(); // Z set if val = 0
    else reset_z();
}

void cpu::set_z()
{
    F |= FLAG_Z;
}

void cpu::reset_z()
{
    F &= ~FLAG_Z;
}


bool cpu::n_flag()
{
    return F & FLAG_N;
}

void cpu::set_n()
{
    F |= FLAG_N;
}

void cpu::reset_n()
{
    F &= ~FLAG_N;
}



bool cpu::h_flag()
{
    return F & FLAG_H;
}

void cpu::check_h_add8(quint8 val1, quint8 val2)
{
    if( ( ((val1 & 0xF) + (val2 & 0xF)) & 0x10 ) == 0x10 ) set_h(); // H set if carry from bit 3
    else reset_h();
}

void cpu::check_h_add16(quint16 val1, quint16 val2)
{
    if( ( ((val1 & 0xFFF) + (val2 & 0xFFF)) & 0x1000 ) == 0x1000 ) set_h(); // H set if carry from bit 11
    else reset_h();
}

void cpu::check_h_sub8(quint8 val1, quint8 val2)
{
    if( (val1 & 0x0F) < (val2 & 0x0F)) set_h(); // H set if no borrow from bit 4
    else reset_h();
}

void cpu::set_h()
{
    F |= FLAG_H;
}

void cpu::reset_h()
{
    F &= ~FLAG_H;
}


bool cpu::c_flag()
{
    return F & FLAG_C;
}

void cpu::check_c_rl(quint8 reg)
{
    if(reg & 0x80) set_c();
    else reset_c();
}

void cpu::check_c_rr(quint8 reg)
{
    if(reg & 0x01) set_c();
    else reset_c();
}

void cpu::check_c_add8(quint8 val1, quint8 val2)
{
    if((val1 + val2) > 0xFF) set_c();
    else reset_c();
}

void cpu::check_c_add16(quint16 val1, quint16 val2)
{
    if((val1 + val2) > 0xFFFF) set_c();
    else reset_c();
}

void cpu::check_c_sub8(quint8 val1, quint8 val2)
{
    if(val1 < val2) set_c(); // c set if no borrow
    else reset_c();
}

void cpu::set_c()
{
    F |= FLAG_C;
}

void cpu::reset_c()
{
    F &= ~FLAG_C;
}

/////////////////////////////////////
// OPCODE FUNCTIONS
/////////////////////////////////////

void cpu::not_impl()
{
    std::cerr << "Opcode not implemented" << std::endl;
    exit(EXIT_FAILURE);
}

/* 0x00 NOP : No Operation
 *
 * Flags affected:
 * None
 */
void cpu::nop()
{

}

/* 0x01 LD BC, d16 : Load 16-bit immediate into BC
 *
 * Flags affected:
 * None
 */
void cpu::ld_bc_d16()
{
    write_on_register(REGISTER_BC, _d16);
}

/* 0x02 LD (BC), A : Save A to address pointed by BC
 *
 * Flags affected:
 * None
 */
void cpu::ld_bc_a()
{
    _MMU->wb(_BC, _A);
}

/* 0x03 INC BC : Increment 16-bit BC
 *
 * Flags affected:
 * None
 */
void cpu::inc_bc()
{
    _C++;
    if(!_C) _B++;
}

/* 0x04 INC B : Increment B
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
void cpu::inc_b()
{
    check_h_add8(_B, 1);

    _B++;

    check_z(_B);
    reset_n();
}

/* 0x05 DEC B : Decrement B
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
void cpu::dec_b()
{
    check_h_sub8(_B, 1);

    _B--;

    check_z(_B);
    set_n();
}

/* 0x06 LD B, d8 : Load 8-bit immediate into B
 *
 * Flags affected:
 * None
 */
void cpu::ld_b_d8()
{
    _B = _d8;
}

/* 0x07 RLCA : Rotate A left. Old bit 7 to Carry flag.
 *
 * Flags affected:
 * Z - Reset.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rlca()
{
    reset_z();
    reset_n();
    reset_h();

    check_c_rl(_A);

    _A = ( (_A & 0x80) ? (_A << 1) + 1 : (_A << 1) );


}

/* 0x08 LD (a16), SP : Save SP to given address
 *
 * Flags affected:
 * None
 */
void cpu::ld_a16_sp()
{
    _MMU->ww(_a16, SP);
}

/* 0x09 ADD HL, BC : Add BC to HL.
 *
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Set if carry from bit 11.
 * C - Set if carry from bit 15.
 */
void cpu::add_hl_bc()
{
    reset_n();
    check_h_add16(_HL, _BC);
    check_c_add16(_HL, _BC);

    write_on_register(REGISTER_HL, _HL + _BC);
}

/* 0x0A LD A, (BC) : Load A from address pointed to by BC
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_bc()
{
    _A = _MMU->rb(_BC);
}

/* 0x0B DEC BC : Decrement 16-bit BC
 *
 * Flags affected:
 * None
 */
void cpu::dec_bc()
{
    _C--;
    if(_C == 0xFF) _B--;
}

/* 0x0C INC C : Increment C
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
void cpu::inc_c()
{
    check_h_add8(_C, 1);

    _C++;

    check_z(_C);
    reset_n();
}

/* 0x0D DEC C : Decrement C
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
void cpu::dec_c()
{
    check_h_sub8(_C, 1);

    _C--;

    check_z(_C);
    set_n();
}

/* 0x0E LD C, d8 : Load 8-bit immediate into C
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_d8()
{
    _C = _d8;
}

/* 0x0F RRCA : Rotate n right. Old bit 0 to Carry flag.
 *
 * Flags affected:
 * Z - Reset
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rrca()
{
    reset_z();
    reset_n();
    reset_h();
    check_c_rr(_A);

    _A = ( (_A & 0x1) ? (_A >> 1) + 0x80 : (_A >> 1) );


}



/* 0x10 STOP 0 : Stop processor
 *
 * Flags affected:
 * None
 */
void cpu::stop()
{
    STOP = true;
}

/* 0x11 LD DE, d16 : Load 16-bit immediate into DE
 *
 * Flags affected:
 * None
 */
void cpu::ld_de_d16()
{
    write_on_register(REGISTER_DE, _d16);
}

/* 0x12 LD (DE), A : Save A to address pointed by DE
 *
 * Flags affected:
 * None
 */
void cpu::ld_de_a()
{
    _MMU->wb(_DE, _A);
}

/* 0x13 INC DE : Increment 16-bit DE
 *
 * Flags affected:
 * None
 */
void cpu::inc_de()
{
    _E++;
    if(!_E) _D++;
}

/* 0x14 INC D : Increment D
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
void cpu::inc_d()
{
    check_h_add8(_D, 1);

    _D++;

    check_z(_D);
    reset_n();
}

/* 0x15 DEC D : Decrement D
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
void cpu::dec_d()
{
    check_h_sub8(_D, 1);

    _D--;

    check_z(_D);
    set_n();
}

/* 0x16 LD D, d8 : Load 8-bit immediate into D
 *
 * Flags affected:
 * None
 */
void cpu::ld_d_d8()
{
    _D = _d8;
}

/* 0x17 RLA : Rotate A left through Carry flag.
 *
 * Flags affected:
 * Z - Reset.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rla()
{
    reset_z();
    reset_n();
    reset_h();

    quint8 old_bit = _A & 0x80;

    _A = ( c_flag() ? (_A << 1) + 1 : (_A << 1) );

    if(old_bit) set_c();
    else reset_c();
}

/* 0x18 JR r8 : Relative jump by signed immediate. Add 8-byte signed data to current address and jump to it.
 *
 * Flags affected:
 * None
 */
void cpu::jr_r8()
{
    PC += _r8;
    PC -= current_opcode->length; // correct ?
}

/* 0x19 ADD HL, DE : Add DE to HL.
 *
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Set if carry from bit 11.
 * C - Set if carry from bit 15.
 */
void cpu::add_hl_de()
{
    reset_n();
    check_h_add16(_HL, _DE);
    check_c_add16(_HL, _DE);

    write_on_register(REGISTER_HL, _HL + _DE);
}

/* 0x1A LD A, (DE) : Load A from address pointed to by DE
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_de()
{
    _A = _MMU->rb(_DE);
}

/* 0x1B DEC DE : Decrement 16-bit DE
 *
 * Flags affected:
 * None
 */
void cpu::dec_de()
{
    _E--;
    if(_E == 0xFF) _D--;
}

/* 0x1C INC E : Increment E
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
void cpu::inc_e()
{
    check_h_add8(_E, 1);

    _E++;

    check_z(_E);
    reset_n();
}

/* 0x1D DEC E : Decrement E
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
void cpu::dec_e()
{
    check_h_sub8(_E, 1);

    _E--;

    check_z(_E);
    set_n();
}

/* 0x1E LD E, d8 : Load 8-bit immediate into E
 *
 * Flags affected:
 * None
 */
void cpu::ld_e_d8()
{
    _E = _d8;
}

/* 0x1F RRA : Rotate A right through Carry flag.
 *
 * Flags affected:
 * Z - Reset.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rra()
{
    reset_z();
    reset_n();
    reset_h();

    quint8 old_bit = _A & 0x1;

    _A = ( c_flag() ? (_A >> 1) + 0x80 : (_A >> 1) );

    if(old_bit) set_c();
    else reset_c();
}


/* 0x20 JR NZ, r8 : Relative jump by signed immediate if last result was not zero.
 *
 * Flags affected:
 * None
 */
void cpu::jr_nz_r8()
{
    if(!z_flag())
    {
        PC += _r8;
        PC -= current_opcode->length; // correct ?
    }
    else
        last_opcode_not_executed = true;
}

/* 0x21 LD HL, d16 : Load 16-bit immediate into HL
 *
 * Flags affected:
 * None
 */
void cpu::ld_hl_d16()
{
    write_on_register(REGISTER_HL, _d16);
}

/* 0x22 LDI (HL), A : Save A to address pointed by HL, and increment HL
 *
 * Flags affected:
 * None
 */
void cpu::ldi_hl_a()
{
    _MMU->wb(_HL, _A);
    inc_hl();
}

/* 0x23 INC HL : Increment 16-bit HL
 *
 * Flags affected:
 * None
 */
void cpu::inc_hl()
{
    _L++;
    if(!_L) _H++;
}

/* 0x24 INC H : Increment H
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
void cpu::inc_h()
{
    check_h_add8(_H, 1);

    _H++;

    check_z(_H);
    reset_n();
}

/* 0x25 DEC H : Decrement H
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
void cpu::dec_h()
{
    check_h_sub8(_H, 1);

    _H--;

    check_z(_H);
    set_n();
}

/* 0x26 LD H, d8 : Load 8-bit immediate into H
 *
 * Flags affected:
 * None
 */
void cpu::ld_h_d8()
{
    _H = _d8;
}

/* 0x27 DAA : Decimal adjust register A. This instruction adjusts register A so that the
 * correct representation of Binary Coded Decimal (BCD) is obtained.
 *
 * Flags affected:
 * Z - Set if register A is zero.
 * N - Not affected.
 * H - Reset.
 * C - Set of reset according to operation.
 */
void cpu::daa()
{

}

/* 0x28 JR Z, r8 : Relative jump by signed immediate if last result was zero.
 *
 * Flags affected:
 * None
 */
void cpu::jr_z_r8()
{
    if(z_flag())
    {
        PC += _r8;
        PC -= current_opcode->length; // correct ?
    }
    else
        last_opcode_not_executed = true;
}

/* 0x29 ADD HL, HL : Add HL to HL.
 *
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Set if carry from bit 11.
 * C - Set if carry from bit 15.
 */
void cpu::add_hl_hl()
{
    reset_n();
    check_h_add16(_HL, _HL);
    check_c_add16(_HL, _HL);

    write_on_register(REGISTER_HL, _HL + _HL);
}

/* 0x2A LDI A, (HL) : Load A from address pointed to by HL, and increment HL
 *
 * Flags affected:
 * None
 */
void cpu::ldi_a_hl()
{
    _A = _MMU->rb(_HL);
    inc_hl();
}

/* 0x2B DEC HL : Decrement 16-bit HL
 *
 * Flags affected:
 * None
 */
void cpu::dec_hl()
{
    _L--;
    if(_L == 0xFF) _H--;
}

/* 0x2C INC L : Increment L
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
void cpu::inc_l()
{
    check_h_add8(_L, 1);

    _L++;

    check_z(_L);
    reset_n();
}

/* 0x2D DEC L : Decrement L
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
void cpu::dec_l()
{
    check_h_sub8(_L, 1);

    _L--;

    check_z(_L);
    set_n();
}

/* 0x2E LD L, d8 : Load 8-bit immediate into L
 *
 * Flags affected:
 * None
 */
void cpu::ld_l_d8()
{
    _L = _d8;
}

/* 0x2F CPL : Complement A register. (Flip all bits.)
 *
 * Flags affected:
 * Z - Not affected.
 * N - Set.
 * H - Set.
 * C - Not affected.
 */
void cpu::cpl()
{
    set_n();
    set_h();

    _A = ~_A;
}


/* 0x30 JR NC, r8 : Relative jump by signed immediate if last result caused no carry
 *
 * Flags affected:
 * None
 */
void cpu::jr_nc_r8()
{
    if(!c_flag())
    {
        PC += _r8;
        PC -= current_opcode->length; // correct ?
    }
    else
        last_opcode_not_executed = true;
}

/* 0x31 LD SP, d16 : Load 16-bit immediate into SP
 *
 * Flags affected:
 * None
 */
void cpu::ld_sp_d16()
{
    SP = _d16;
}

/* 0x32 LDD HL, A : Save A to address pointed by HL, and decrement HL
 *
 * Flags affected:
 * None
 */
void cpu::ldd_hl_a()
{
    _MMU->wb(_HL, _A);
    dec_hl();
}

/* 0x33 INC SP : Increment 16-bit SP
 *
 * Flags affected:
 * None
 */
void cpu::inc_sp()
{
    SP++;
}

/* 0x34 INC (HL) : Increment value pointed by HL
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
void cpu::inc_hl_()
{
    check_h_add8(_MMU->rb(_HL), 1);

    _MMU->wb(_HL, _MMU->rb(_HL) + 1);

    check_z(_MMU->rb(_HL));
    reset_n();
}

/* 0x35 DEC (HL) : Decrement value pointed by HL
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
void cpu::dec_hl_()
{
    check_h_sub8(_MMU->rb(_HL), 1);

    _MMU->wb(_HL, _MMU->rb(_HL) - 1);

    check_z(_MMU->rb(_HL));
    set_n();
}

 /* 0x36 LD (HL), d8 : Load 8-bit immediate into address pointed by HL
  *
  * Flags affected:
  * None
  */
void cpu::ld_hl_d8()
{
    _MMU->wb(_HL, _d8);
}

/* 0x37 SCF : Set carry flag
 *
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Reset.
 * C - Set.
 */
void cpu::scf()
{
    reset_n();
    reset_h();
    set_c();
}

/* 0x38 JR C, r8 : Relative jump by signed immediate if last result caused carry
 *
 * Flags affected:
 * None
 */
void cpu::jr_c_r8()
{
    if(c_flag())
    {
        PC += _r8;
        PC -= current_opcode->length; // correct ?
    }
    else
        last_opcode_not_executed = true;
}

/* 0x39 ADD HL, SP : Add SP to HL.
 *
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Set if carry from bit 11.
 * C - Set if carry from bit 15.
 */
void cpu::add_hl_sp()
{
    reset_n();
    check_h_add16(_HL, _SP);
    check_c_add16(_HL, _SP);

    write_on_register(REGISTER_HL, _HL + _SP);
}

/* 0x3A LDD A, (HL) : Load A from address pointed to by HL, and decrement HL
 *
 * Flags affected:
 * None
 */
void cpu::ldd_a_hl()
{
    _A = _MMU->rb(_HL);
    dec_hl();
}

/* 0x3B DEC SP : Decrement 16-bit SP
 *
 * Flags affected:
 * None
 */
void cpu::dec_sp()
{
    SP--;
}

/* 0x3C INC A : Increment A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
void cpu::inc_a()
{
    check_h_add8(_A, 1);

    _A++;

    check_z(_A);
    reset_n();
}

/* 0x3D DEC A : Decrement A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
void cpu::dec_a()
{
    check_h_sub8(_A, 1);

    _A--;

    check_z(_A);
    set_n();
}

/* 0x3E LD A, d8 : Load 8-bit immediate into A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_d8()
{
    _A = _d8;
}

/* 0x3F CCF : Complement carry flag
 *
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Reset.
 * C - Complemented.
 */
void cpu::ccf()
{
    if(c_flag()) reset_c();
    else set_c();
}


/* 0x40 LD B, B : Copy B to B
 *
 * Flags affected:
 * None
 */
void cpu::ld_b_b()
{
    _B = _B;
}

/* 0x41 LD B, C : Copy C to B
 *
 * Flags affected:
 * None
 */
void cpu::ld_b_c()
{
    _B = _C;
}

/* 0x42 LD B, D : Copy D to B
 *
 * Flags affected:
 * None
 */
void cpu::ld_b_d()
{
    _B = _D;
}

/* 0x43 LD B, E : Copy E to B
 *
 * Flags affected:
 * None
 */
void cpu::ld_b_e()
{
    _B = _E;
}

/* 0x44 LD B, H : Copy H to B
 *
 * Flags affected:
 * None
 */
void cpu::ld_b_h()
{
    _B = _H;
}

/* 0x45 LD B, L : Copy L to B
 *
 * Flags affected:
 * None
 */
void cpu::ld_b_l()
{
    _B = _L;
}

/* 0x46 LD B, (HL) : Copy value pointed by HL to B
 *
 * Flags affected:
 * None
 */
void cpu::ld_b_hl()
{
    _B = _MMU->rb(_HL);
}

/* 0x47 LD B, A : Copy A to B
 *
 * Flags affected:
 * None
 */
void cpu::ld_b_a()
{
    _B = _A;
}

/* 0x48 LD C, B : Copy B to C
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_b()
{
    _C = _B;
}

/* 0x49 LD C, C : Copy C to C
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_c()
{
    _C = _C;
}

/* 0x4A LD C, D : Copy D to C
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_d()
{
    _C = _D;
}

/* 0x4B LD C, E : Copy E to C
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_e()
{
    _C = _E;
}

/* 0x4C LD C, H : Copy H to C
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_h()
{
    _C = _H;
}

/* 0x4D LD C, L : Copy L to C
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_l()
{
    _C = _L;
}

/* 0x4E LD C, (HL) : Copy value pointed by HL to C
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_hl()
{
    _C = _MMU->rb(_HL);
}

/* 0x4F LD C, A : Copy A to C
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_a()
{
    _C = _A;
}


/* 0x50 LD D, B : Copy B to D
 *
 * Flags affected:
 * None
 */
void cpu::ld_d_b()
{
    _D = _B;
}

/* 0x51 LD D, C : Copy C to D
 *
 * Flags affected:
 * None
 */
void cpu::ld_d_c()
{
    _D = _C;
}

/* 0x52 LD D, D : Copy D to D
 *
 * Flags affected:
 * None
 */
void cpu::ld_d_d()
{
    _D = _D;
}

/* 0x53 LD D, E : Copy E to D
 *
 * Flags affected:
 * None
 */
void cpu::ld_d_e()
{
    _D = _E;
}

/* 0x54 LD D, H : Copy H to D
 *
 * Flags affected:
 * None
 */
void cpu::ld_d_h()
{
    _D = _H;
}

/* 0x55 LD D, L : Copy L to D
 *
 * Flags affected:
 * None
 */
void cpu::ld_d_l()
{
    _D = _L;
}

/* 0x56 LD D, (HL) : Copy value pointed by HL to D
 *
 * Flags affected:
 * None
 */
void cpu::ld_d_hl()
{
    _D = _MMU->rb(_HL);
}

/* 0x57 LD D, A : Copy A to D
 *
 * Flags affected:
 * None
 */
void cpu::ld_d_a()
{
    _D = _A;
}

/* 0x58 LD E, B : Copy B to E
 *
 * Flags affected:
 * None
 */
void cpu::ld_e_b()
{
    _E = _B;
}

/* 0x59 LD E, C : Copy C to E
 *
 * Flags affected:
 * None
 */
void cpu::ld_e_c()
{
    _E = _C;
}

/* 0x5A LD E, D : Copy D to E
 *
 * Flags affected:
 * None
 */
void cpu::ld_e_d()
{
    _E = _D;
}

/* 0x5B LD E, E : Copy E to E
 *
 * Flags affected:
 * None
 */
void cpu::ld_e_e()
{
    _E = _E;
}

/* 0x5C LD E, H : Copy H to E
 *
 * Flags affected:
 * None
 */
void cpu::ld_e_h()
{
    _E = _H;
}

/* 0x5D LD E, L : Copy L to E
 *
 * Flags affected:
 * None
 */
void cpu::ld_e_l()
{
    _E = _L;
}

/* 0x5E LD E, (HL) : Copy value pointed by HL to E
 *
 * Flags affected:
 * None
 */
void cpu::ld_e_hl()
{
    _E = _MMU->rb(_HL);
}

/* 0x5F LD E, A : Copy A to E
 *
 * Flags affected:
 * None
 */
void cpu::ld_e_a()
{
    _E = _A;
}


/* 0x60 LD H, B : Copy B to H
 *
 * Flags affected:
 * None
 */
void cpu::ld_h_b()
{
    _H = _B;
}

/* 0x61 LD H, C : Copy C to H
 *
 * Flags affected:
 * None
 */
void cpu::ld_h_c()
{
    _H = _C;
}

/* 0x62 LD H, D : Copy D to H
 *
 * Flags affected:
 * None
 */
void cpu::ld_h_d()
{
    _H = _D;
}

/* 0x63 LD H, E : Copy E to H
 *
 * Flags affected:
 * None
 */
void cpu::ld_h_e()
{
    _H = _E;
}

/* 0x64 LD H, H : Copy H to H
 *
 * Flags affected:
 * None
 */
void cpu::ld_h_h()
{
    _H = _H;
}

/* 0x65 LD H, L : Copy L to H
 *
 * Flags affected:
 * None
 */
void cpu::ld_h_l()
{
    _H = _L;
}

/* 0x66 LD H, (HL) : Copy value pointed by HL to H
 *
 * Flags affected:
 * None
 */
void cpu::ld_h_hl()
{
    _H = _MMU->rb(_HL);
}

/* 0x67 LD H, A : Copy A to H
 *
 * Flags affected:
 * None
 */
void cpu::ld_h_a()
{
    _H = _A;
}

/* 0x68 LD L, B : Copy B to L
 *
 * Flags affected:
 * None
 */
void cpu::ld_l_b()
{
    _L = _B;
}

/* 0x69 LD L, C : Copy C to L
 *
 * Flags affected:
 * None
 */
void cpu::ld_l_c()
{
    _L = _C;
}

/* 0x6A LD L, D : Copy D to L
 *
 * Flags affected:
 * None
 */
void cpu::ld_l_d()
{
    _L = _D;
}

/* 0x6B LD L, E : Copy E to L
 *
 * Flags affected:
 * None
 */
void cpu::ld_l_e()
{
    _L = _E;
}

/* 0x6C LD L, H : Copy H to L
 *
 * Flags affected:
 * None
 */
void cpu::ld_l_h()
{
    _L = _H;
}

/* 0x6D LD L, L : Copy L to L
 *
 * Flags affected:
 * None
 */
void cpu::ld_l_l()
{
    _L = _L;
}

/* 0x6E LD L, (HL) : Copy value pointed by HL to L
 *
 * Flags affected:
 * None
 */
void cpu::ld_l_hl()
{
    _L = _MMU->rb(_HL);
}

/* 0x6F LD L, A : Copy A to L
 *
 * Flags affected:
 * None
 */
void cpu::ld_l_a()
{
    _L = _A;
}



/* 0x70 LD (HL), B : Copy B to address pointed by HL
 *
 * Flags affected:
 * None
 */
void cpu::ld_hl_b()
{
    _MMU->wb(_HL, _B);
}

/* 0x71 LD (HL), C : Copy C to address pointed by HL
 *
 * Flags affected:
 * None
 */
void cpu::ld_hl_c()
{
    _MMU->wb(_HL, _C);
}

/* 0x72 LD (HL), D : Copy D to address pointed by HL
 *
 * Flags affected:
 * None
 */
void cpu::ld_hl_d()
{
    _MMU->wb(_HL, _D);
}

/* 0x73 LD (HL), E : Copy E to address pointed by HL
 *
 * Flags affected:
 * None
 */
void cpu::ld_hl_e()
{
    _MMU->wb(_HL, _E);
}

/* 0x74 LD (HL), H : Copy H to address pointed by HL
 *
 * Flags affected:
 * None
 */
void cpu::ld_hl_h()
{
    _MMU->wb(_HL, _H);
}

/* 0x75 LD (HL), L : Copy L to address pointed by HL
 *
 * Flags affected:
 * None
 */
void cpu::ld_hl_l()
{
    _MMU->wb(_HL, _L);
}

/* 0x76 HALT : Power down CPU until an interrupt occurs
 *
 * Flags affected:
 * None
 */
void cpu::halt()
{
    HALT = true;
}

/* 0x77 LD (HL), A : Copy A to address pointed by HL
 *
 * Flags affected:
 * None
 */
void cpu::ld_hl_a()
{
    _MMU->wb(_HL, _A);
}

/* 0x78 LD A, B : Copy B to A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_b()
{
    _A = _B;
}

/* 0x79 LD A, C : Copy C to A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_c()
{
    _A = _C;
}

/* 0x7A LD A, D : Copy D to A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_d()
{
    _A = _D;
}

/* 0x7B LD A, E : Copy E to A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_e()
{
    _A = _E;
}

/* 0x7C LD A, H : Copy H to A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_h()
{
    _A = _H;
}

/* 0x7D LD A, L : Copy L to A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_l()
{
    _A = _L;
}

/* 0x7E LD A, (HL) : Copy value pointed by HL to A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_hl()
{
    _A = _MMU->rb(_HL);
}

/* 0x7F LD A, A : Copy A to A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_a()
{
    _A = _A;
}



/* 0x80 ADD A, B : Add B to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::add_a_b()
{
    reset_n();

    check_h_add8(_A, _B);
    check_c_add8(_A, _B);

    _A += _B;

    check_z(_A);
}

/* 0x81 ADD A, C : Add C to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::add_a_c()
{
    reset_n();

    check_h_add8(_A, _C);
    check_c_add8(_A, _C);

    _A += _C;

    check_z(_A);
}

/* 0x82 ADD A, D : Add D to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::add_a_d()
{
    reset_n();

    check_h_add8(_A, _D);
    check_c_add8(_A, _D);

    _A += _D;

    check_z(_A);
}

/* 0x83 ADD A, E : Add E to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::add_a_e()
{
    reset_n();

    check_h_add8(_A, _E);
    check_c_add8(_A, _E);

    _A += _E;

    check_z(_A);
}

/* 0x84 ADD A, H : Add H to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::add_a_h()
{
    reset_n();

    check_h_add8(_A, _H);
    check_c_add8(_A, _H);

    _A += _H;

    check_z(_A);
}

/* 0x85 ADD A, L : Add L to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::add_a_l()
{
    reset_n();

    check_h_add8(_A, _L);
    check_c_add8(_A, _L);

    _A += _L;

    check_z(_A);
}

/* 0x86 ADD A, (HL) : Add value pointed by HL to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::add_a_hl()
{
    reset_n();

    quint8 val = _MMU->rb(_HL);
    check_h_add8(_A, val);
    check_c_add8(_A, val);

    _A += val;

    check_z(_A);
}

/* 0x87 ADD A, A : Add A to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::add_a_a()
{
    reset_n();

    check_h_add8(_A, _A);
    check_c_add8(_A, _A);

    _A += _A;

    check_z(_A);
}

/* 0x88 ADC A, B : Add B and carry flag to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::adc_a_b()
{
    quint8 cf = (c_flag() ? 1 : 0);

    reset_n();

    check_h_add8(_A, _B + cf);
    check_c_add8(_A, _B + cf);

    _A += (_B + cf);

    check_z(_A);
}

/* 0x89 ADC A, C : Add C and carry flag to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::adc_a_c()
{
    quint8 cf = (c_flag() ? 1 : 0);

    reset_n();

    check_h_add8(_A, _C + cf);
    check_c_add8(_A, _C + cf);

    _A += (_C + cf);

    check_z(_A);
}

/* 0x8A ADC A, D : Add D and carry flag to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::adc_a_d()
{
    quint8 cf = (c_flag() ? 1 : 0);

    reset_n();

    check_h_add8(_A, _D + cf);
    check_c_add8(_A, _D + cf);

    _A += (_D + cf);

    check_z(_A);
}

/* 0x8B ADC A, E : Add E and carry flag to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::adc_a_e()
{
    quint8 cf = (c_flag() ? 1 : 0);

    reset_n();

    check_h_add8(_A, _E + cf);
    check_c_add8(_A, _E + cf);

    _A += (_E + cf);

    check_z(_A);
}

/* 0x8C ADC A, H : Add H and carry flag to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::adc_a_h()
{
    quint8 cf = (c_flag() ? 1 : 0);

    reset_n();

    check_h_add8(_A, _H + cf);
    check_c_add8(_A, _H + cf);

    _A += (_H + cf);

    check_z(_A);
}

/* 0x8D ADC A, L : Add L and carry flag to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::adc_a_l()
{
    quint8 cf = (c_flag() ? 1 : 0);

    reset_n();

    check_h_add8(_A, _L + cf);
    check_c_add8(_A, _L + cf);

    _A += (_L + cf);

    check_z(_A);
}

/* 0x8E ADC A, (HL) : Add value pointed by HL and carry flag to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::adc_a_hl()
{
    quint8 cf = (c_flag() ? 1 : 0);
    quint8 val = _MMU->rb(_HL);

    reset_n();

    check_h_add8(_A, val + cf);
    check_c_add8(_A, val + cf);

    _A += (val + cf);

    check_z(_A);
}

/* 0x8F ADC A, A : Add A and carry flag to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::adc_a_a()
{
    quint8 cf = (c_flag() ? 1 : 0);

    reset_n();

    check_h_add8(_A, _A + cf);
    check_c_add8(_A, _A + cf);

    _A += (_A + cf);

    check_z(_A);
}


/* 0x90 SUB B : Subtract B from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sub_b()
{
    set_n();
    check_h_sub8(_A, _B);
    check_c_sub8(_A, _B);

    _A -= _B;

    check_z(_A);
}

/* 0x91 SUB C : Subtract C from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sub_c()
{
    set_n();
    check_h_sub8(_A, _C);
    check_c_sub8(_A, _C);

    _A -= _C;

    check_z(_A);
}

/* 0x92 SUB D : Subtract D from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sub_d()
{
    set_n();
    check_h_sub8(_A, _D);
    check_c_sub8(_A, _D);

    _A -= _D;

    check_z(_A);
}

/* 0x93 SUB E : Subtract E from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sub_e()
{
    set_n();
    check_h_sub8(_A, _E);
    check_c_sub8(_A, _E);

    _A -= _E;

    check_z(_A);
}

/* 0x94 SUB H : Subtract H from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sub_h()
{
    set_n();
    check_h_sub8(_A, _H);
    check_c_sub8(_A, _H);

    _A -= _H;

    check_z(_A);
}

/* 0x95 SUB L : Subtract L from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sub_l()
{
    set_n();
    check_h_sub8(_A, _L);
    check_c_sub8(_A, _L);

    _A -= _L;

    check_z(_A);
}

/* 0x96 SUB (HL) : Subtract value pointed by HL from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sub_hl()
{
    quint8 val = _MMU->rb(_HL);

    set_n();
    check_h_sub8(_A, val);
    check_c_sub8(_A, val);

    _A -= val;

    check_z(_A);
}

/* 0x97 SUB A : Subtract A from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sub_a()
{
    set_n();
    check_h_sub8(_A, _A);
    check_c_sub8(_A, _A);

    _A -= _A;

    check_z(_A);
}

/* 0x98 SBC A, B : Subtract B and carry flag from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sbc_a_b()
{
    quint8 cf = (c_flag() ? 1 : 0);

    set_n();
    check_h_sub8(_A, _B + cf);
    check_c_sub8(_A, _B + cf);

    _A -= (_B + cf);

    check_z(_A);
}

/* 0x99 SBC A, C : Subtract C and carry flag from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sbc_a_c()
{
    quint8 cf = (c_flag() ? 1 : 0);

    set_n();
    check_h_sub8(_A, _C + cf);
    check_c_sub8(_A, _C + cf);

    _A -= (_C + cf);

    check_z(_A);
}

/* 0x9A SBC A, D : Subtract D and carry flag from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sbc_a_d()
{
    quint8 cf = (c_flag() ? 1 : 0);

    set_n();
    check_h_sub8(_A, _D + cf);
    check_c_sub8(_A, _D + cf);

    _A -= (_D + cf);

    check_z(_A);
}

/* 0x9B SBC A, E : Subtract E and carry flag from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sbc_a_e()
{
    quint8 cf = (c_flag() ? 1 : 0);

    set_n();
    check_h_sub8(_A, _E + cf);
    check_c_sub8(_A, _E + cf);

    _A -= (_E + cf);

    check_z(_A);
}

/* 0x9C SBC A, H : Subtract H and carry flag from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sbc_a_h()
{
    quint8 cf = (c_flag() ? 1 : 0);

    set_n();
    check_h_sub8(_A, _H + cf);
    check_c_sub8(_A, _H + cf);

    _A -= (_H + cf);

    check_z(_A);
}

/* 0x9D SBC A, L : Subtract L and carry flag from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sbc_a_l()
{
    quint8 cf = (c_flag() ? 1 : 0);

    set_n();
    check_h_sub8(_A, _L + cf);
    check_c_sub8(_A, _L + cf);

    _A -= (_L + cf);

    check_z(_A);
}

/* 0x9E SBC A, (HL) : Subtract value pointed by HL and carry flag from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sbc_a_hl()
{
    quint8 cf = (c_flag() ? 1 : 0);
    quint8 val = _MMU->rb(_HL);

    set_n();
    check_h_sub8(_A, val + cf);
    check_c_sub8(_A, val + cf);

    _A -= (val + cf);

    check_z(_A);
}

/* 0x9F SBC A, A : Subtract A and carry flag from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sbc_a_a()
{
    quint8 cf = (c_flag() ? 1 : 0);

    set_n();
    check_h_sub8(_A, _A + cf);
    check_c_sub8(_A, _A + cf);

    _A -= (_A + cf);

    check_z(_A);
}



/* 0xA0 AND B : Logical AND between A and B. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
void cpu::and_b()
{
    reset_n();
    set_h();
    reset_c();

    _A &= _B;

    check_z(_A);
}

/* 0xA1 AND C : Logical AND between A and C. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
void cpu::and_c()
{
    reset_n();
    set_h();
    reset_c();

    _A &= _C;

    check_z(_A);
}

/* 0xA2 AND D : Logical AND between A and D. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
void cpu::and_d()
{
    reset_n();
    set_h();
    reset_c();

    _A &= _D;

    check_z(_A);
}

/* 0xA3 AND E : Logical AND between A and E. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
void cpu::and_e()
{
    reset_n();
    set_h();
    reset_c();

    _A &= _E;

    check_z(_A);
}

/* 0xA4 AND H : Logical AND between A and H. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
void cpu::and_h()
{
    reset_n();
    set_h();
    reset_c();

    _A &= _H;

    check_z(_A);
}

/* 0xA5 AND L : Logical AND between A and L. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
void cpu::and_l()
{
    reset_n();
    set_h();
    reset_c();

    _A &= _L;

    check_z(_A);
}

/* 0xA6 AND (HL) : Logical AND between A and value pointed by HL. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
void cpu::and_hl()
{
    reset_n();
    set_h();
    reset_c();

    _A &= _MMU->rb(_HL);

    check_z(_A);
}

/* 0xA7 AND A : Logical AND between A and A. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
void cpu::and_a()
{
    reset_n();
    set_h();
    reset_c();

    _A &= _A;

    check_z(_A);
}

/* 0xA8 XOR B : Logical XOR between A and B. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::xor_b()
{
    reset_n();
    reset_h();
    reset_c();

    _A ^= _B;

    check_z(_A);
}

/* 0xA9 XOR C : Logical XOR between A and C. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::xor_c()
{
    reset_n();
    reset_h();
    reset_c();

    _A ^= _C;

    check_z(_A);
}

/* 0xAA XOR D : Logical XOR between A and D. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::xor_d()
{
    reset_n();
    reset_h();
    reset_c();

    _A ^= _D;

    check_z(_A);
}

/* 0xAB XOR E : Logical XOR between A and E. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::xor_e()
{
    reset_n();
    reset_h();
    reset_c();

    _A ^= _E;

    check_z(_A);
}

/* 0xAC XOR H : Logical XOR between A and H. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::xor_h()
{
    reset_n();
    reset_h();
    reset_c();

    _A ^= _H;

    check_z(_A);
}

/* 0xAD XOR L : Logical XOR between A and L. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::xor_l()
{
    reset_n();
    reset_h();
    reset_c();

    _A ^= _L;

    check_z(_A);
}

/* 0xAE XOR (HL) : Logical XOR between A and value pointed by HL. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::xor_hl()
{
    reset_n();
    reset_h();
    reset_c();

    _A ^= _MMU->rb(_HL);

    check_z(_A);
}

/* 0xAF XOR A : Logical XOR between A and A. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::xor_a()
{
    reset_n();
    reset_h();
    reset_c();

    _A ^= _A;

    check_z(_A);
}


/* 0xB0 OR B : Logical OR between A and B. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::or_b()
{
    reset_n();
    reset_h();
    reset_c();

    _A |= _B;

    check_z(_A);
}

/* 0xB1 OR C : Logical OR between A and C. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::or_c()
{
    reset_n();
    reset_h();
    reset_c();

    _A |= _C;

    check_z(_A);
}

/* 0xB2 OR D : Logical OR between A and D. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::or_d()
{
    reset_n();
    reset_h();
    reset_c();

    _A |= _D;

    check_z(_A);
}

/* 0xB3 OR E : Logical OR between A and E. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::or_e()
{
    reset_n();
    reset_h();
    reset_c();

    _A |= _E;

    check_z(_A);
}

/* 0xB4 OR H : Logical OR between A and H. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::or_h()
{
    reset_n();
    reset_h();
    reset_c();

    _A |= _H;

    check_z(_A);
}

/* 0xB5 OR L : Logical OR between A and L. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::or_l()
{
    reset_n();
    reset_h();
    reset_c();

    _A |= _L;

    check_z(_A);
}

/* 0xB6 OR (HL) : Logical OR between A and value pointed by HL. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::or_hl()
{
    reset_n();
    reset_h();
    reset_c();

    _A |= _MMU->rb(_HL);

    check_z(_A);
}

/* 0xB7 OR A : Logical OR between A and L. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::or_a()
{
    reset_n();
    reset_h();
    reset_c();

    _A |= _A;

    check_z(_A);
}

/* 0xB8 CP B : Compare A with B. (Basically A - B instruction with result thrown away)
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < B)
 */
void cpu::cp_b()
{
    set_n();
    check_h_sub8(_A, _B);
    check_c_sub8(_A, _B);
    check_z(_A - _B);
}

/* 0xB9 CP C : Compare A with C. (Basically A - C instruction with result thrown away)
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < C)
 */
void cpu::cp_c()
{
    set_n();
    check_h_sub8(_A, _C);
    check_c_sub8(_A, _C);
    check_z(_A - _C);
}

/* 0xBA CP D : Compare A with D. (Basically A - D instruction with result thrown away)
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < D)
 */
void cpu::cp_d()
{
    set_n();
    check_h_sub8(_A, _D);
    check_c_sub8(_A, _D);
    check_z(_A - _D);
}

/* 0xBB CP E : Compare A with E. (Basically A - E instruction with result thrown away)
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < E)
 */
void cpu::cp_e()
{
    set_n();
    check_h_sub8(_A, _E);
    check_c_sub8(_A, _E);
    check_z(_A - _E);
}

/* 0xBC CP H : Compare A with H. (Basically A - H instruction with result thrown away)
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < H)
 */
void cpu::cp_h()
{
    set_n();
    check_h_sub8(_A, _H);
    check_c_sub8(_A, _H);
    check_z(_A - _H);
}

/* 0xBD CP L : Compare A with L. (Basically A - L instruction with result thrown away)
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < L)
 */
void cpu::cp_l()
{
    set_n();
    check_h_sub8(_A, _L);
    check_c_sub8(_A, _L);
    check_z(_A - _L);
}

/* 0xBE CP (HL) : Compare A with value pointed by HL. (Basically A - value instruction with result thrown away)
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < value)
 */
void cpu::cp_hl()
{
    quint8 val = _MMU->rb(_HL);
    set_n();
    check_h_sub8(_A, val);
    check_c_sub8(_A, val);
    check_z(_A - val);
}

/* 0xBF CP A : Compare A with A. (Basically A - A instruction with result thrown away)
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < A)
 */
void cpu::cp_a()
{
    set_n();
    check_h_sub8(_A, _A);
    check_c_sub8(_A, _A);
    check_z(_A - _A);
}



/* 0xC0 RET NZ : Return if last result was not zero
 *
 * Flags affected:
 * None
 */
void cpu::ret_nz()
{
    if(!z_flag())
    {
        PC = _MMU->rw(SP);
        SP += 2;
    }
    else
        last_opcode_not_executed = true;
}

/* 0xC1 POP BC : Pop 16-bit value from stack into BC
 *
 * Flags affected:
 * None
 */
void cpu::pop_bc()
{
    _C = _MMU->rb(SP);
    _B = _MMU->rb(SP+1);

    SP += 2;
}

/* 0xC2 JP NZ, a16 : Absolute jump to 16-bit location if last result was not zero
 *
 * Flags affected:
 * None
 */
void cpu::jp_nz_a16()
{
    if(!z_flag())
    {
        PC = _a16;
        PC -= current_opcode->length;
    }
    else
        last_opcode_not_executed = true;

}

/* 0xC3 JP a16 : Absolute jump to 16-bit location
 *
 * Flags affected:
 * None
 */
void cpu::jp_a16()
{
    PC = _a16;
    PC -= current_opcode->length;
}

/* 0xC4 CALL NZ, a16 : Call routine at 16-bit location if last result was not zero
 *
 * Flags affected:
 * None
 */
void cpu::call_nz_a16()
{
    if(!z_flag())
    {
        _MMU->ww(SP, PC);
        PC = _a16;
        PC -= current_opcode->length;
        SP -= 2;
    }
    else
        last_opcode_not_executed = true;
}

/* 0xC5 PUSH BC : Push BC into stack
 *
 * Flags affected:
 * None
 */
void cpu::push_bc()
{
    _MMU->wb(SP-1, _B);
    _MMU->wb(SP-2, _C);
    SP -= 2;
}

/* 0xC6 ADD A, d8 : Add 8-bit immediate to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::add_a_d8()
{
    quint8 val = _d8;
    reset_n();

    check_h_add8(_A, val);
    check_c_add8(_A, val);

    _A += val;

    check_z(_A);
}

/* 0xC7 RST 00h : Call routine at address 00h
 *
 * Flags affected:
 * None
 */
void cpu::rst_00h()
{
    _MMU->ww(SP, PC);
    PC = 0x0;
    PC -= current_opcode->length;
    SP -= 2;
}

/* 0xC8 RET Z : Return if last result was zero
 *
 * Flags affected:
 * None
 */
void cpu::ret_z()
{
    if(z_flag())
    {
        PC = _MMU->rw(SP);
        SP += 2;
    }
    else
        last_opcode_not_executed = true;
}

/* 0xC9 RET : Return from routine
 *
 * Flags affected:
 * None
 */
void cpu::ret()
{
    PC = _MMU->rw(SP);
    SP += 2;
}

/* 0xCA JP Z, a16 : Absolute jump to 16-bit location if last result was zero
 *
 * Flags affected:
 * None
 */
void cpu::jp_z_a16()
{
    if(z_flag())
    {
        PC = _a16;
        PC -= current_opcode->length;
    }
    else
        last_opcode_not_executed = true;

}
/* 0xCB PREFIX CB : Extended Operations (2-bytes opcodes)
 *
 * Flags affected:
 * None
 */
void cpu::prefix_cb()
{

}

/* 0xCC CALL Z, a16 : Call routine at 16-bit location if last result was zero
 *
 * Flags affected:
 * None
 */
void cpu::call_z_a16()
{
    if(z_flag())
    {
        _MMU->ww(SP, PC);
        PC = _a16;
        PC -= current_opcode->length;
        SP -= 2;
    }
    else
        last_opcode_not_executed = true;
}

/* 0xCD CALL a16 : Call routine at 16-bit location
 *
 * Flags affected:
 * None
 */
void cpu::call_a16()
{
    _MMU->ww(SP, PC);
    PC = _a16;
    PC -= current_opcode->length;
    SP -= 2;
}

/* 0xCE ADC A, d8 : Add 8-bit immediate and carry flag to A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
void cpu::adc_a_d8()
{
    quint8 cf = (c_flag() ? 1 : 0);
    quint8 val = _d8;
    reset_n();

    check_h_add8(_A, val + cf);
    check_c_add8(_A, val + cf);

    _A += (val + cf);

    check_z(_A);
}

/* 0xCF RST 08h : Call routine at address 08h
 *
 * Flags affected:
 * None
 */
void cpu::rst_08h()
{
    _MMU->ww(SP, PC);
    PC = 0x08;
    PC -= current_opcode->length;
    SP -= 2;
}


/* 0xD0 RET NC Return if last result caused no carry
 *
 * Flags affected:
 * None
 */
void cpu::ret_nc()
{
   if(!c_flag())
   {
       PC = _MMU->rw(SP);
       SP += 2;
   }
   else
       last_opcode_not_executed = true;
}

/* 0xD1 POP DE : Pop 16-bit value from stack into DE
 *
 * Flags affected:
 * None
 */
void cpu::pop_de()
{
    _E = _MMU->rb(SP);
    _D = _MMU->rb(SP+1);

    SP += 2;
}

/* 0xD2 JP NC, a16 : Absolute jump to 16-bit location if last result caused no carry
 *
 * Flags affected:
 * None
 */
void cpu::jp_nc_a16()
{
    if(!c_flag())
    {
        PC = _a16;
        PC -= current_opcode->length;
    }
    else
        last_opcode_not_executed = true;

}

/* 0xD4 CALL NC, a16 : Call routine at 16-bit location if last result caused no carry
 *
 * Flags affected:
 * None
 */
void cpu::call_nc_a16()
{
    if(!c_flag())
    {
        _MMU->ww(SP, PC);
        PC = _a16;
        PC -= current_opcode->length;
        SP -= 2;
    }
    else
        last_opcode_not_executed = true;
}

/* 0xD5 PUSH DE : Push DE into stack
 *
 * Flags affected:
 * None
 */
void cpu::push_de()
{
    _MMU->wb(SP-1, _D);
    _MMU->wb(SP-2, _E);
    SP -= 2;
}

/* 0xD6 SUB d8 : Subtract 8-bit immediate from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sub_d8()
{
    quint8 val = _d8;

    set_n();
    check_h_sub8(_A, val);
    check_c_sub8(_A, val);

    _A -= val;

    check_z(_A);
}

/* 0xD7 RST 10h : Call routine at address 10h
 *
 * Flags affected:
 * None
 */
void cpu::rst_10h()
{
    _MMU->ww(SP, PC);
    PC = 0x10;
    PC -= current_opcode->length;
    SP -= 2;
}

/* 0xD8 RET C : Return if last result caused carry
 *
 * Flags affected:
 * None
 */
void cpu::ret_c()
{
    if(c_flag())
    {
        PC = _MMU->rw(SP);
        SP += 2;
    }
    else
        last_opcode_not_executed = true;
}

/* 0xD9 RETI : Enable interrupts and return from routine
 *
 * Flags affected:
 * None
 */
void cpu::reti()
{
    IME = true;
    PC = _MMU->rw(SP);
    SP += 2;
}

/* 0xDA JP C, a16 : Absolute jump to 16-bit location if last result caused carry
 *
 * Flags affected:
 * None
 */
void cpu::jp_c_a16()
{
    if(c_flag())
    {
        PC = _a16;
        PC -= current_opcode->length;
    }
    else
        last_opcode_not_executed = true;

}

/* 0xDC CALL C, a16 : Call routine at 16-bit location if last result caused carry
 *
 * Flags affected:
 * None
 */
void cpu::call_c_a16()
{
    if(c_flag())
    {
        _MMU->ww(SP, PC);
        PC = _a16;
        PC -= current_opcode->length;
        SP -= 2;
    }
    else
        last_opcode_not_executed = true;
}

/* 0xDE SBC A, d8 : Subtract 8-bit immediate and carry flag from A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
void cpu::sbc_a_d8()
{
    quint8 val = _d8;
    quint8 cf = (c_flag() ? 1 : 0);

    set_n();
    check_h_sub8(_A, val + cf);
    check_c_sub8(_A, val + cf);

    _A -= (val + cf);

    check_z(_A);
}

/* 0xDF RST 18h : Call routine at address 18h
 *
 * Flags affected:
 * None
 */
void cpu::rst_18h()
{
    _MMU->ww(SP, PC);
    PC = 0x18;
    PC -= current_opcode->length;
    SP -= 2;
}



/* 0xE0 LDH (a8), A : Save A at address pointed to by (FF00h + 8-bit immediate)
 *
 * Flags affected:
 * None
 */
void cpu::ldh_a8_a()
{
    _MMU->wb(0xFF00 + _a8, _A);
}

/* 0xE1 POP HL : Pop 16-bit value from stack into HL
 *
 * Flags affected:
 * None
 */
void cpu::pop_hl()
{
    _L = _MMU->rb(SP);
    _H = _MMU->rb(SP+1);

    SP += 2;
}

/* 0xE2 LD (C), A : Save A at address pointed to by (FF00h + C)
 *
 * Flags affected:
 * None
 */
void cpu::ld_c_a_()
{
    _MMU->wb(0xFF00 + _C, _A);
}

/* 0xE5 PUSH HL : Push HL into stack
 *
 * Flags affected:
 * None
 */
void cpu::push_hl()
{
    _MMU->wb(SP-1, _H);
    _MMU->wb(SP-2, _L);
    SP -= 2;
}

/* 0xE6 AND d8 : Logical AND between A and 8-bit immediate. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
void cpu::and_d8()
{
    reset_n();
    set_h();
    reset_c();

    _A &= _d8;

    check_z(_A);
}

/* 0xE7 RST 20h : Call routine at address 20h
 *
 * Flags affected:
 * None
 */
void cpu::rst_20h()
{
    _MMU->ww(SP, PC);
    PC = 0x20;
    PC -= current_opcode->length;
    SP -= 2;
}

/* 0xE8 ADD SP, r8 : Add signed 8-bit immediate to SP.
 *
 * Flags affected:
 * Z - Reset.
 * N - Reset.
 * H - Set if carry from bit 11.
 * C - Set if carry from bit 15.
 */
void cpu::add_sp_r8()
{
    qint8 val = _r8;

    reset_z();
    reset_n();
    check_h_add16(SP, val);
    check_c_add16(SP, val);

    SP += val;
}

/* 0xE9 JP (HL) : Absolute jump to 16-bit location pointed to by HL
 *
 * Flags affected:
 * None
 */
void cpu::jp_hl()
{
    PC = _MMU->rw(_HL);
    PC -= current_opcode->length;
}

/* 0xEA LD (a16), A : Save A at given 16-bit address
 *
 * Flags affected:
 * None
 */
void cpu::ld_a16_a()
{
    _MMU->wb(_a16, _A);
}

/* 0xEE XOR d8 : Logical XOR between A and 8-bit immediate. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::xor_d8()
{
    reset_n();
    reset_h();
    reset_c();

    _A ^= _d8;

    check_z(_A);
}

/* 0xEF RST 28h : Call routine at address 28h
 *
 * Flags affected:
 * None
 */
void cpu::rst_28h()
{
    _MMU->ww(SP, PC);
    PC = 0x28;
    PC -= current_opcode->length;
    SP -= 2;
}


/* 0xF0 LDH A, (a8) : Load A from address pointed to by (FF00h + 8-bit immediate)
 *
 * Flags affected:
 * None
 */
void cpu::ldh_a_a8()
{
    _A = _MMU->rb(0xFF00 + _a8);
}

/* 0xF1 POP AF : Pop 16-bit value from stack into AF
 *
 * Flags affected:
 * None
 */
void cpu::pop_af()
{
    F = _MMU->rb(SP);
    _A = _MMU->rb(SP+1);

    SP += 2;
}

/* 0xF2 LD A, (C) : Put value at address (FF00 + C) into A
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_c_()
{
    _A = _MMU->rb(0xFF00 + _C);
}

/* 0xF3 DI : Disable interrupts
 *
 * Flags affected:
 * None
 */
void cpu::di()
{
    IME = false;
}

/* 0xF5 PUSH AF : Push AF into stack
 *
 * Flags affected:
 * None
 */
void cpu::push_af()
{
    _MMU->wb(SP-1, _A);
    _MMU->wb(SP-2, F);
    SP -= 2;
}

/* 0xF6 OR d8 : Logical OR between A and 8-bit immediate. Result in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::or_d8()
{
    reset_n();
    reset_h();
    reset_c();

    _A |= _d8;

    check_z(_A);
}

/* 0xF7 RST 30h : Call routine at address 30h
 *
 * Flags affected:
 * None
 */
void cpu::rst_30h()
{
    _MMU->ww(SP, PC);
    PC = 0x30;
    PC -= current_opcode->length;
    SP -= 2;
}

/* 0xF8 LDHL SP, r8 : Put SP + 8-bit signed value into HL
 *
 * Flags affected:
 * Z - Reset.
 * N - Reset.
 * H - Set if carry from bit 11.
 * C - Set if carry from bit 15.
 */
void cpu::ldhl_sp_r8()
{
    qint8 val = _r8;

    reset_z();
    reset_n();
    check_h_add16(SP, val);
    check_c_add16(SP, val);

    write_on_register(REGISTER_HL, SP + val);
}

/* 0xF9 LD SP, HL : Copy HL to SP
 *
 * Flags affected:
 * None
 */
void cpu::ld_sp_hl()
{
    SP = _HL;
}

/* 0xFA LD A, (a16) : Load A from given 16-bit address
 *
 * Flags affected:
 * None
 */
void cpu::ld_a_a16()
{
    _A = _MMU->rb(_a16);
}

/* 0xFB EI : Enable interrupts
 *
 * Flags affected:
 * None
 */
void cpu::ei()
{
    IME = true;
}

/* 0xFE CP d8 : Compare A with 8-bit immediate. (Basically A - 8-bit immediate instruction with result thrown away)
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < 8-bit immediate)
 */
void cpu::cp_d8()
{
    quint8 val = _d8;

    set_n();
    check_h_sub8(_A, val);
    check_c_sub8(_A, val);
    check_z(_A - val);
}

/* 0xFF RST 38h : Call routine at address 38h
 *
 * Flags affected:
 * None
 */
void cpu::rst_38h()
{
    _MMU->ww(SP, PC);
    PC = 0x38;
    PC -= current_opcode->length;
    SP -= 2;
}



//2-bytes opcodes

/* 0xCB00 RLC B : Rotate B left. Old bit 7 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rlc_b()
{
    reset_n();
    reset_h();

    check_c_rl(_B);

    _B = ( (_B & 0x80) ? (_B << 1) + 1 : (_B << 1) );

    check_z(_B);
}

/* 0xCB01 RLC C : Rotate C left. Old bit 7 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rlc_c()
{
   reset_n();
   reset_h();

   check_c_rl(_C);

   _C = ( (_C & 0x80) ? (_C << 1) + 1 : (_C << 1) );

   check_z(_C);
}

/* 0xCB02 RLC D : Rotate D left. Old bit 7 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rlc_d()
{
   reset_n();
   reset_h();

   check_c_rl(_D);

   _D = ( (_D & 0x80) ? (_D << 1) + 1 : (_D << 1) );

   check_z(_D);
}

/* 0xCB03 RLC E : Rotate E left. Old bit 7 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rlc_e()
{
   reset_n();
   reset_h();

   check_c_rl(_E);

   _E = ( (_E & 0x80) ? (_E << 1) + 1 : (_E << 1) );

   check_z(_E);
}

/* 0xCB04 RLC H : Rotate H left. Old bit 7 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rlc_h()
{
   reset_n();
   reset_h();

   check_c_rl(_H);

   _H = ( (_H & 0x80) ? (_H << 1) + 1 : (_H << 1) );

   check_z(_H);
}

/* 0xCB05 RLC L : Rotate L left. Old bit 7 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rlc_l()
{
   reset_n();
   reset_h();

   check_c_rl(_L);

   _L = ( (_L & 0x80) ? (_L << 1) + 1 : (_L << 1) );

   check_z(_L);
}

/* 0xCB06 RLC (HL) : Rotate value pointed by HL left. Old bit 7 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rlc_hl()
{
    quint8 val = _MMU->rb(_HL);

    reset_n();
    reset_h();

    check_c_rl(val);

    val = ( (val & 0x80) ? (val << 1) + 1 : (val << 1) );
    _MMU->wb(_HL, val);

    check_z(val);
}

/* 0xCB07 RLC A : Rotate A left. Old bit 7 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rlc_a()
{
   reset_n();
   reset_h();

   check_c_rl(_A);

   _A = ( (_A & 0x80) ? (_A << 1) + 1 : (_A << 1) );

   check_z(_A);
}

/* 0xCB08 RRC B : Rotate B right. Old bit 0 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rrc_b()
{
    reset_n();
    reset_h();
    check_c_rr(_B);

    _B = ( (_B & 0x1) ? (_B >> 1) + 0x80 : (_B >> 1) );

    check_z(_B);
}

/* 0xCB09 RRC C : Rotate C right. Old bit 0 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rrc_c()
{
    reset_n();
    reset_h();
    check_c_rr(_C);

    _C = ( (_C & 0x1) ? (_C >> 1) + 0x80 : (_C >> 1) );

    check_z(_C);
}

/* 0xCB0A RRC D : Rotate D right. Old bit 0 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rrc_d()
{
    reset_n();
    reset_h();
    check_c_rr(_D);

    _D = ( (_D & 0x1) ? (_D >> 1) + 0x80 : (_D >> 1) );

    check_z(_D);
}

/* 0xCB0B RRC E : Rotate E right. Old bit 0 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rrc_e()
{
    reset_n();
    reset_h();
    check_c_rr(_E);

    _E = ( (_E & 0x1) ? (_E >> 1) + 0x80 : (_E >> 1) );

    check_z(_E);
}

/* 0xCB0C RRC H : Rotate H right. Old bit 0 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rrc_h()
{
    reset_n();
    reset_h();
    check_c_rr(_H);

    _H = ( (_H & 0x1) ? (_H >> 1) + 0x80 : (_H >> 1) );

    check_z(_H);
}

/* 0xCB0D RRC L : Rotate L right. Old bit 0 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rrc_l()
{
    reset_n();
    reset_h();
    check_c_rr(_L);

    _L = ( (_L & 0x1) ? (_L >> 1) + 0x80 : (_L >> 1) );

    check_z(_L);
}

/* 0xCB0E RRC (HL) : Rotate value pointed by HL right. Old bit 0 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rrc_hl()
{
    quint8 val = _MMU->rb(_HL);

    reset_n();
    reset_h();
    check_c_rr(val);

    val = ( (val & 0x1) ? (val >> 1) + 0x80 : (val >> 1) );
    _MMU->wb(_HL, val);

    check_z(val);
}

/* 0xCB0F RRC A : Rotate A right. Old bit 0 to Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rrc_a()
{
    reset_n();
    reset_h();
    check_c_rr(_A);

    _A = ( (_A & 0x1) ? (_A >> 1) + 0x80 : (_A >> 1) );

    check_z(_A);
}



/* 0xCB10 RL B : Rotate B left through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rl_b()
{
    reset_n();
    reset_h();

    quint8 old_bit = _B & 0x80;

    _B = ( c_flag() ? (_B << 1) + 1 : (_B << 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_B);
}

/* 0xCB11 RL C : Rotate C left through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rl_c()
{
    reset_n();
    reset_h();

    quint8 old_bit = _C & 0x80;

    _C = ( c_flag() ? (_C << 1) + 1 : (_C << 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_C);
}

/* 0xCB12 RL D : Rotate D left through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rl_d()
{
    reset_n();
    reset_h();

    quint8 old_bit = _D & 0x80;

    _D = ( c_flag() ? (_D << 1) + 1 : (_D << 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_D);
}

/* 0xCB13 RL E : Rotate E left through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rl_e()
{
   reset_n();
   reset_h();

   quint8 old_bit = _E & 0x80;

   _E = ( c_flag() ? (_E << 1) + 1 : (_E << 1) );

   if(old_bit) set_c();
   else reset_c();

   check_z(_E);
}

/* 0xCB14 RL H : Rotate H left through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rl_h()
{
   reset_n();
   reset_h();

   quint8 old_bit = _H & 0x80;

   _H = ( c_flag() ? (_H << 1) + 1 : (_H << 1) );

   if(old_bit) set_c();
   else reset_c();

   check_z(_H);
}

/* 0xCB15 RL L : Rotate L left through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rl_l()
{
   reset_n();
   reset_h();

   quint8 old_bit = _L & 0x80;

   _L = ( c_flag() ? (_L << 1) + 1 : (_L << 1) );

   if(old_bit) set_c();
   else reset_c();

   check_z(_L);
}

/* 0xCB16 RL (HL) : Rotate value pointed by HL left through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rl_hl()
{
   reset_n();
   reset_h();

   quint8 val = _MMU->rb(_HL);
   quint8 old_bit = val & 0x80;

   val = ( c_flag() ? (val << 1) + 1 : (val << 1) );
   _MMU->wb(_HL, val);

   if(old_bit) set_c();
   else reset_c();

   check_z(val);
}

/* 0xCB17 RL A : Rotate A left through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::rl_a()
{
   reset_n();
   reset_h();

   quint8 old_bit = _L & 0x80;

   _A = ( c_flag() ? (_A << 1) + 1 : (_A << 1) );

   if(old_bit) set_c();
   else reset_c();

   check_z(_A);
}

/* 0xCB18 RR B : Rotate B right through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rr_b()
{
    reset_n();
    reset_h();

    quint8 old_bit = _B & 0x1;

    _B = ( c_flag() ? (_B >> 1) + 0x80 : (_B >> 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_B);
}

/* 0xCB19 RR C : Rotate C right through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rr_c()
{
    reset_n();
    reset_h();

    quint8 old_bit = _C & 0x1;

    _C = ( c_flag() ? (_C >> 1) + 0x80 : (_C >> 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_C);
}

/* 0xCB1A RR D : Rotate D right through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rr_d()
{
    reset_n();
    reset_h();

    quint8 old_bit = _D & 0x1;

    _D = ( c_flag() ? (_D >> 1) + 0x80 : (_D >> 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_D);
}

/* 0xCB1B RR E : Rotate E right through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rr_e()
{
    reset_n();
    reset_h();

    quint8 old_bit = _E & 0x1;

    _E = ( c_flag() ? (_E >> 1) + 0x80 : (_E >> 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_E);
}

/* 0xCB1C RR H : Rotate H right through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rr_h()
{
    reset_n();
    reset_h();

    quint8 old_bit = _H & 0x1;

    _H = ( c_flag() ? (_H >> 1) + 0x80 : (_H >> 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_H);
}

/* 0xCB1D RR L : Rotate L right through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rr_l()
{
    reset_n();
    reset_h();

    quint8 old_bit = _L & 0x1;

    _L = ( c_flag() ? (_L >> 1) + 0x80 : (_L >> 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_L);
}

/* 0xCB1E RR (HL) : Rotate value pointed by HL right through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rr_hl()
{
    quint8 val = _MMU->rb(_HL);

    reset_n();
    reset_h();

    quint8 old_bit = val & 0x1;

    val = ( c_flag() ? (val >> 1) + 0x80 : (val >> 1) );
    _MMU->wb(_HL, val);

    if(old_bit) set_c();
    else reset_c();

    check_z(val);
}

/* 0xCB1F RR A : Rotate A right through Carry flag.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::rr_a()
{
    reset_n();
    reset_h();

    quint8 old_bit = _A & 0x1;

    _A = ( c_flag() ? (_A >> 1) + 0x80 : (_A >> 1) );

    if(old_bit) set_c();
    else reset_c();

    check_z(_A);
}


/* 0xCB20 SLA B : Shift B left into Carry. LSBit of n set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::sla_b()
{
    reset_n();
    reset_h();

    if(_B & 0x80) set_c();
    else reset_c();

    _B <<= 1;

    check_z(_B);
}

/* 0xCB21 SLA C : Shift C left into Carry. LSBit of n set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::sla_c()
{
   reset_n();
   reset_h();

   if(_C & 0x80) set_c();
   else reset_c();

   _C <<= 1;

   check_z(_C);
}

/* 0xCB22 SLA D : Shift D left into Carry. LSBit of n set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::sla_d()
{
   reset_n();
   reset_h();

   if(_D & 0x80) set_c();
   else reset_c();

   _D <<= 1;

   check_z(_D);
}

/* 0xCB23 SLA E : Shift E left into Carry. LSBit of n set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::sla_e()
{
   reset_n();
   reset_h();

   if(_E & 0x80) set_c();
   else reset_c();

   _E <<= 1;

   check_z(_E);
}

/* 0xCB24 SLA H : Shift H left into Carry. LSBit of n set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::sla_h()
{
   reset_n();
   reset_h();

   if(_H & 0x80) set_c();
   else reset_c();

   _H <<= 1;

   check_z(_H);
}

/* 0xCB25 SLA L : Shift L left into Carry. LSBit of n set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::sla_l()
{
   reset_n();
   reset_h();

   if(_L & 0x80) set_c();
   else reset_c();

   _L <<= 1;

   check_z(_L);
}

/* 0xCB26 SLA (HL) : Shift value pointed by HL left into Carry. LSBit of n set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::sla_hl()
{
    quint8 val = _MMU->rb(_HL);

    reset_n();
    reset_h();

    if(val & 0x80) set_c();
    else reset_c();

    val <<= 1;
    _MMU->wb(_HL, val);

    check_z(val);
}

/* 0xCB27 SLA A : Shift A left into Carry. LSBit of n set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
void cpu::sla_a()
{
   reset_n();
   reset_h();

   if(_A & 0x80) set_c();
   else reset_c();

   _A <<= 1;

   check_z(_A);
}

/* 0xCB28 SRA B : Shift B right into Carry. MSBit doesn't change.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::sra_b()
{
    reset_n();
    reset_h();

    if(_B & 0x01) set_c();
    else reset_c();

    _B = (qint8)_B >> 1;

    check_z(_B);
}

/* 0xCB29 SRA C : Shift C right into Carry. MSBit doesn't change.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::sra_c()
{
    reset_n();
    reset_h();

    if(_C & 0x01) set_c();
    else reset_c();

    _C = (qint8)_C >> 1;

    check_z(_C);
}

/* 0xCB2A SRA D : Shift D right into Carry. MSBit doesn't change.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::sra_d()
{
    reset_n();
    reset_h();

    if(_D & 0x01) set_c();
    else reset_c();

    _D = (qint8)_D >> 1;

    check_z(_D);
}

/* 0xCB2B SRA E : Shift E right into Carry. MSBit doesn't change.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::sra_e()
{
    reset_n();
    reset_h();

    if(_E & 0x01) set_c();
    else reset_c();

    _E = (qint8)_E >> 1;

    check_z(_E);
}

/* 0xCB2C SRA H : Shift H right into Carry. MSBit doesn't change.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::sra_h()
{
    reset_n();
    reset_h();

    if(_H & 0x01) set_c();
    else reset_c();

    _H = (qint8)_H >> 1;

    check_z(_H);
}

/* 0xCB2D SRA L : Shift L right into Carry. MSBit doesn't change.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::sra_l()
{
    reset_n();
    reset_h();

    if(_L & 0x01) set_c();
    else reset_c();

    _L = (qint8)_L >> 1;

    check_z(_L);
}

/* 0xCB2E SRA (HL) : Shift value pointed by HL right into Carry. MSBit doesn't change.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::sra_hl()
{
    quint8 val = _MMU->rb(_HL);
    reset_n();
    reset_h();

    if(val & 0x01) set_c();
    else reset_c();

    val = (qint8)val >> 1;
    _MMU->wb(_HL, val);

    check_z(val);
}

/* 0xCB2F SRA A : Shift A right into Carry. MSBit doesn't change.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::sra_a()
{
    reset_n();
    reset_h();

    if(_A & 0x01) set_c();
    else reset_c();

    _A = (qint8)_A >> 1;

    check_z(_A);
}



/* 0xCB30 SWAP B : Swap nybbles in B
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::swap_b()
{
    reset_n();
    reset_h();
    reset_c();

    _B = ((_B & 0xF0) >> 4) + ((_B & 0xF) << 4);

    check_z(_B);
}

/* 0xCB31 SWAP C : Swap nybbles in C
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::swap_c()
{
    reset_n();
    reset_h();
    reset_c();

    _C = ((_C & 0xF0) >> 4) + ((_C & 0xF) << 4);

    check_z(_C);
}

/* 0xCB32 SWAP D : Swap nybbles in D
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::swap_d()
{
    reset_n();
    reset_h();
    reset_c();

    _D = ((_D & 0xF0) >> 4) + ((_D & 0xF) << 4);

    check_z(_D);
}

/* 0xCB33 SWAP E : Swap nybbles in E
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::swap_e()
{
    reset_n();
    reset_h();
    reset_c();

    _E = ((_E & 0xF0) >> 4) + ((_E & 0xF) << 4);

    check_z(_E);
}

/* 0xCB34 SWAP H : Swap nybbles in H
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::swap_h()
{
    reset_n();
    reset_h();
    reset_c();

    _H = ((_H & 0xF0) >> 4) + ((_H & 0xF) << 4);

    check_z(_H);
}

/* 0xCB35 SWAP L : Swap nybbles in L
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::swap_l()
{
    reset_n();
    reset_h();
    reset_c();

    _L = ((_L & 0xF0) >> 4) + ((_L & 0xF) << 4);

    check_z(_L);
}

/* 0xCB36 SWAP (HL) : Swap nybbles in value pointed by HL
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::swap_hl()
{
    quint8 val = _MMU->rb(_HL);

    reset_n();
    reset_h();
    reset_c();

    val = ((val & 0xF0) >> 4) + ((val & 0xF) << 4);
    _MMU->wb(_HL, val);

    check_z(val);
}

/* 0xCB37 SWAP A : Swap nybbles in A
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
void cpu::swap_a()
{
    reset_n();
    reset_h();
    reset_c();

    _A = ((_A & 0xF0) >> 4) + ((_A & 0xF) << 4);

    check_z(_A);
}

/* 0xCB38 SRL B : Shift B right into Carry. MSBit of B set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::srl_b()
{
    reset_n();
    reset_h();

    if(_B & 0x01) set_c();
    else reset_c();

    _B >>= 1;

    check_z(_B);
}

/* 0xCB39 SRL C : Shift C right into Carry. MSBit of C set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::srl_c()
{
    reset_n();
    reset_h();

    if(_C & 0x01) set_c();
    else reset_c();

    _C >>= 1;

    check_z(_C);
}

/* 0xCB3A SRL D : Shift D right into Carry. MSBit of D set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::srl_d()
{
    reset_n();
    reset_h();

    if(_D & 0x01) set_c();
    else reset_c();

    _D >>= 1;

    check_z(_D);
}

/* 0XCB3B SRL E : Shift E right into Carry. MSBit of E set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::srl_e()
{
    reset_n();
    reset_h();

    if(_E & 0x01) set_c();
    else reset_c();

    _E >>= 1;

    check_z(_E);
}

/* 0xCB3C SRL H : Shift H right into Carry. MSBit of H set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::srl_h()
{
    reset_n();
    reset_h();

    if(_H & 0x01) set_c();
    else reset_c();

    _H >>= 1;

    check_z(_H);
}

/* 0xCB3D SRL L : Shift L right into Carry. MSBit of L set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::srl_l()
{
    reset_n();
    reset_h();

    if(_L & 0x01) set_c();
    else reset_c();

    _L >>= 1;

    check_z(_L);
}

/* 0xCB3E SRL (HL) : Shift value pointed by HL right into Carry. MSBit of value set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::srl_hl()
{
    quint8 val = _MMU->rb(_HL);
    reset_n();
    reset_h();

    if(val & 0x01) set_c();
    else reset_c();

    val >>= 1;
    _MMU->wb(_HL, val);

    check_z(val);
}

/* 0xCB3F SRL A : Shift A right into Carry. MSBit of A set to 0.
 *
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
void cpu::srl_a()
{
    reset_n();
    reset_h();

    if(_A & 0x01) set_c();
    else reset_c();

    _A >>= 1;

    check_z(_A);
}



/* 0xCB40 BIT 0, B : Test bit 0 of B
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_0_b()
{
    reset_n();
    set_h();

    check_z(_B & 0x01);
}

/* 0xCB41 BIT 0, C : Test bit 0 of C
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_0_c()
{
    reset_n();
    set_h();

    check_z(_C & 0x01);
}

/* 0xCB42 BIT 0, D : Test bit 0 of D
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_0_d()
{
    reset_n();
    set_h();

    check_z(_D & 0x01);
}

/* 0xCB43 BIT 0, E : Test bit 0 of E
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_0_e()
{
    reset_n();
    set_h();

    check_z(_E & 0x01);
}

/* 0xCB44 BIT 0, H : Test bit 0 of H
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_0_h()
{
    reset_n();
    set_h();

    check_z(_H & 0x01);
}

/* 0xCB45 BIT 0, L : Test bit 0 of L
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_0_l()
{
    reset_n();
    set_h();

    check_z(_L & 0x01);
}

/* 0xCB46 BIT 0, (HL) : Test bit 0 of value pointed by HL
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_0_hl()
{
    reset_n();
    set_h();

    check_z(_MMU->rb(_HL) & 0x01);
}

/* 0xCB47 BIT 0, A : Test bit 0 of A
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_0_a()
{
    reset_n();
    set_h();

    check_z(_A & 0x01);
}

/* 0xCB48 BIT 1, B : Test bit 1 of B
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_1_b()
{
    reset_n();
    set_h();

    check_z(_B & (1 << 1));
}

/* 0xCB49 BIT 1, C : Test bit 1 of C
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_1_c()
{
    reset_n();
    set_h();

    check_z(_C & (1 << 1));
}

/* 0xCB4A BIT 1, D : Test bit 1 of D
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_1_d()
{
    reset_n();
    set_h();

    check_z(_D & (1 << 1));
}

/* 0xCB4B BIT 1, E : Test bit 1 of E
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_1_e()
{
    reset_n();
    set_h();

    check_z(_E & (1 << 1));
}

/* 0xCB4C BIT 1, H : Test bit 1 of H
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_1_h()
{
    reset_n();
    set_h();

    check_z(_H & (1 << 1));
}

/* 0xCB4D BIT 1, L : Test bit 1 of L
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_1_l()
{
    reset_n();
    set_h();

    check_z(_L & (1 << 1));
}

/* 0xCB4E BIT 1, (HL) : Test bit 1 of value pointed by HL
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_1_hl()
{
    reset_n();
    set_h();

    check_z(_MMU->rb(_HL) & (1 << 1));
}

/* 0xCB4F BIT 1, A : Test bit 1 of A
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_1_a()
{
    reset_n();
    set_h();

    check_z(_A & (1 << 1));
}



/* 0xCB50 BIT 2, B : Test bit 2 of B
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_2_b()
{
    reset_n();
    set_h();

    check_z(_B & (1 << 2));
}

/* 0xCB51 BIT 2, C : Test bit 2 of C
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_2_c()
{
    reset_n();
    set_h();

    check_z(_C & (1 << 2));
}

/* 0xCB52 BIT 2, D : Test bit 2 of D
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_2_d()
{
    reset_n();
    set_h();

    check_z(_D & (1 << 2));
}

/* 0xCB53 BIT 2, E : Test bit 2 of E
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_2_e()
{
    reset_n();
    set_h();

    check_z(_E & (1 << 2));
}

/* 0xCB54 BIT 2, H : Test bit 2 of H
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_2_h()
{
    reset_n();
    set_h();

    check_z(_H & (1 << 2));
}

/* 0xCB55 BIT 2, L : Test bit 2 of L
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_2_l()
{
    reset_n();
    set_h();

    check_z(_L & (1 << 2));
}

/* 0xCB56 BIT 2, (HL) : Test bit 2 of value pointed by HL
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_2_hl()
{
    reset_n();
    set_h();

    check_z(_MMU->rb(_HL) & (1 << 2));
}

/* 0xCB57 BIT 2, A : Test bit 2 of A
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_2_a()
{
    reset_n();
    set_h();

    check_z(_A & (1 << 2));
}

/* 0xCB58 BIT 3, B : Test bit 3 of B
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_3_b()
{
    reset_n();
    set_h();

    check_z(_B & (1 << 3));
}

/* 0xCB59 BIT 3, C : Test bit 3 of C
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_3_c()
{
    reset_n();
    set_h();

    check_z(_C & (1 << 3));
}

/* 0xCB5A BIT 3, D : Test bit 3 of D
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_3_d()
{
    reset_n();
    set_h();

    check_z(_D & (1 << 3));
}

/* 0xCB5B BIT 3, E : Test bit 3 of E
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_3_e()
{
    reset_n();
    set_h();

    check_z(_E & (1 << 3));
}

/* 0xCB5C BIT 3, H : Test bit 3 of H
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_3_h()
{
    reset_n();
    set_h();

    check_z(_H & (1 << 3));
}

/* 0xCB5D BIT 3, L : Test bit 3 of L
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_3_l()
{
    reset_n();
    set_h();

    check_z(_L & (1 << 3));
}

/* 0xCB5E BIT 3, (HL) : Test bit 3 of value pointed by HL
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_3_hl()
{
    reset_n();
    set_h();

    check_z(_MMU->rb(_HL) & (1 << 3));
}

/* 0xCB5F BIT 3, A : Test bit 3 of A
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_3_a()
{
    reset_n();
    set_h();

    check_z(_A & (1 << 3));
}



/* 0xCB60 BIT 4, B : Test bit 4 of B
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_4_b()
{
    reset_n();
    set_h();

    check_z(_B & (1 << 4));
}

/* 0xCB61 BIT 4, C : Test bit 4 of C
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_4_c()
{
    reset_n();
    set_h();

    check_z(_C & (1 << 4));
}

/* 0xCB62 BIT 4, D : Test bit 4 of D
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_4_d()
{
    reset_n();
    set_h();

    check_z(_D & (1 << 4));
}

/* 0xCB63 BIT 4, E : Test bit 4 of E
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_4_e()
{
    reset_n();
    set_h();

    check_z(_E & (1 << 4));
}

/* 0xCB64 BIT 4, H : Test bit 4 of H
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_4_h()
{
    reset_n();
    set_h();

    check_z(_H & (1 << 4));
}

/* 0xCB65 BIT 4, L : Test bit 4 of L
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_4_l()
{
    reset_n();
    set_h();

    check_z(_L & (1 << 4));
}

/* 0xCB66 BIT 4, (HL) : Test bit 4 of value pointed by HL
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_4_hl()
{
    reset_n();
    set_h();

    check_z(_MMU->rb(_HL) & (1 << 4));
}

/* 0xCB67 BIT 4, A : Test bit 4 of A
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_4_a()
{
    reset_n();
    set_h();

    check_z(_A & (1 << 4));
}

/* 0xCB68 BIT 5, B : Test bit 5 of B
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_5_b()
{
    reset_n();
    set_h();

    check_z(_B & (1 << 5));
}

/* 0xCB69 BIT 5, C : Test bit 5 of C
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_5_c()
{
    reset_n();
    set_h();

    check_z(_C & (1 << 5));
}

/* 0xCB6A BIT 5, D : Test bit 5 of D
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_5_d()
{
    reset_n();
    set_h();

    check_z(_D & (1 << 5));
}

/* 0xCB6B BIT 5, E : Test bit 5 of E
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_5_e()
{
    reset_n();
    set_h();

    check_z(_E & (1 << 5));
}

/* 0xCB6C BIT 5, H : Test bit 5 of H
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_5_h()
{
    reset_n();
    set_h();

    check_z(_H & (1 << 5));
}

/* 0xCB6D BIT 5, L : Test bit 5 of L
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_5_l()
{
    reset_n();
    set_h();

    check_z(_L & (1 << 5));
}

/* 0xCB6E BIT 5, (HL) : Test bit 5 of value pointed by HL
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_5_hl()
{
    reset_n();
    set_h();

    check_z(_MMU->rb(_HL) & (1 << 5));
}

/* 0xCB6F BIT 5, A : Test bit 5 of A
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_5_a()
{
    reset_n();
    set_h();

    check_z(_A & (1 << 5));
}



/* 0xCB70 BIT 6, B : Test bit 6 of B
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_6_b()
{
    reset_n();
    set_h();

    check_z(_B & (1 << 6));
}

/* 0xCB71 BIT 6, C : Test bit 6 of C
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_6_c()
{
    reset_n();
    set_h();

    check_z(_C & (1 << 6));
}

/* 0xCB72 BIT 6, D : Test bit 6 of D
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_6_d()
{
    reset_n();
    set_h();

    check_z(_D & (1 << 6));
}

/* 0xCB73 BIT 6, E : Test bit 6 of E
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_6_e()
{
    reset_n();
    set_h();

    check_z(_E & (1 << 6));
}

/* 0xCB74 BIT 6, H : Test bit 6 of H
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_6_h()
{
    reset_n();
    set_h();

    check_z(_H & (1 << 6));
}

/* 0xCB75 BIT 6, L : Test bit 6 of L
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_6_l()
{
    reset_n();
    set_h();

    check_z(_L & (1 << 6));
}

/* 0xCB76 BIT 6, (HL) : Test bit 6 of value pointed by HL
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_6_hl()
{
    reset_n();
    set_h();

    check_z(_MMU->rb(_HL) & (1 << 6));
}

/* 0xCB77 BIT 6, A : Test bit 6 of A
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_6_a()
{
    reset_n();
    set_h();

    check_z(_A & (1 << 6));
}

/* 0xCB78 BIT 7, B : Test bit 7 of B
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_7_b()
{
    reset_n();
    set_h();

    check_z(_B & (1 << 7));
}

/* 0xCB79 BIT 7, C : Test bit 7 of C
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_7_c()
{
    reset_n();
    set_h();

    check_z(_C & (1 << 7));
}

/* 0xCB7A BIT 7, D : Test bit 7 of D
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_7_d()
{
    reset_n();
    set_h();

    check_z(_D & (1 << 7));
}

/* 0xCB7B BIT 7, E : Test bit 7 of E
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_7_e()
{
    reset_n();
    set_h();

    check_z(_E & (1 << 7));
}

/* 0xCB7C BIT 7, H : Test bit 7 of H
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_7_h()
{
    reset_n();
    set_h();

    check_z(_H & (1 << 7));
}

/* 0xCB7D BIT 7, L : Test bit 7 of L
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_7_l()
{
    reset_n();
    set_h();

    check_z(_L & (1 << 7));
}

/* 0xCB7E BIT 7, (HL) : Test bit 7 of value pointed by HL
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_7_hl()
{
    reset_n();
    set_h();

    check_z(_MMU->rb(_HL) & (1 << 7));
}

/* 0xCB7F BIT 7, A : Test bit 7 of A
 *
 * Flags affected:
 * Z - Set if bit is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
void cpu::bit_7_a()
{
    reset_n();
    set_h();

    check_z(_A & (1 << 7));
}



/* 0xCB80 RES 0, B : Reset bit 0 of B
 *
 * Flags affected:
 * None
 */
void cpu::res_0_b()
{
    _B &= 0xFE;
}

/* 0xCB81 RES 0, C : Reset bit 0 of C
 *
 * Flags affected:
 * None
 */
void cpu::res_0_c()
{
    _C &= 0xFE;
}

/* 0xCB82 RES 0, D : Reset bit 0 of D
 *
 * Flags affected:
 * None
 */
void cpu::res_0_d()
{
    _D &= 0xFE;
}

/* 0xCB83 RES 0, E : Reset bit 0 of E
 *
 * Flags affected:
 * None
 */
void cpu::res_0_e()
{
    _E &= 0xFE;
}

/* 0xCB84 RES 0, H : Reset bit 0 of H
 *
 * Flags affected:
 * None
 */
void cpu::res_0_h()
{
    _H &= 0xFE;
}

/* 0xCB85 RES 0, L : Reset bit 0 of L
 *
 * Flags affected:
 * None
 */
void cpu::res_0_l()
{
    _L &= 0xFE;
}

/* 0xCB86 RES 0, (HL) : Reset bit 0 of value pointed by HL
 *
 * Flags affected:
 * None
 */
void cpu::res_0_hl()
{
    quint8 val = _MMU->rb(_HL);
    val &= 0xFE;
    _MMU->wb(_HL, val);
}

/* 0xCB87 RES 0, A : Reset bit 0 of A
 *
 * Flags affected:
 * None
 */
void cpu::res_0_a()
{
    _A &= 0xFE;
}

/* 0xCB88 RES 1, B : Reset bit 1 of B
 *
 * Flags affected:
 * None
 */
void cpu::res_1_b()
{
    _B &= 0xFD;
}

/* 0xCB89 RES 1, C : Reset bit 1 of C
 *
 * Flags affected:
 * None
 */
void cpu::res_1_c()
{
    _C &= 0xFD;
}

/* 0xCB8A RES 1, D : Reset bit 1 of D
 *
 * Flags affected:
 * None
 */
void cpu::res_1_d()
{
    _D &= 0xFD;
}

/* 0xCB8B RES 1, E : Reset bit 1 of E
 *
 * Flags affected:
 * None
 */
void cpu::res_1_e()
{
    _E &= 0xFD;
}

/* 0xCB8C RES 1, H : Reset bit 1 of H
 *
 * Flags affected:
 * None
 */
void cpu::res_1_h()
{
    _H &= 0xFD;
}

/* 0xCB8D RES 1, L : Reset bit 1 of L
 *
 * Flags affected:
 * None
 */
void cpu::res_1_l()
{
    _L &= 0xFD;
}

/* 0xCB8E RES 1, (HL) : Reset bit 1 of value pointed by HL
 *
 * Flags affected:
 * None
 */
void cpu::res_1_hl()
{
    quint8 val = _MMU->rb(_HL);
    val &= 0xFD;
    _MMU->wb(_HL, val);
}

/* 0xCB8F RES 1, A : Reset bit 1 of A
 *
 * Flags affected:
 * None
 */
void cpu::res_1_a()
{
    _A &= 0xFD;
}




/* 0xCB90 RES 2, B : Reset bit 2 of B
 *
 * Flags affected:
 * None
 */
void cpu::res_2_b()
{
    _B &= 0xFB;
}

/* 0xCB91 RES 2, C : Reset bit 2 of C
 *
 * Flags affected:
 * None
 */
void cpu::res_2_c()
{
    _C &= 0xFB;
}

/* 0xCB92 RES 2, D : Reset bit 2 of D
 *
 * Flags affected:
 * None
 */
void cpu::res_2_d()
{
    _D &= 0xFB;
}

/* 0xCB93 RES 2, E : Reset bit 2 of E
 *
 * Flags affected:
 * None
 */
void cpu::res_2_e()
{
    _E &= 0xFB;
}

/* 0xCB94 RES 2, H : Reset bit 2 of H
 *
 * Flags affected:
 * None
 */
void cpu::res_2_h()
{
    _H &= 0xFB;
}

/* 0xCB95 RES 2, L : Reset bit 2 of L
 *
 * Flags affected:
 * None
 */
void cpu::res_2_l()
{
    _L &= 0xFB;
}

// 0xCB96 RES 2, (HL)
void cpu::res_2_hl();
// 0xCB97 RES 2, A
void cpu::res_2_a();
// 0xCB98 RES 3, B
void cpu::res_3_b();
// 0xCB99 RES 3, C
void cpu::res_3_c();
// 0xCB9A RES 3, D
void cpu::res_3_d();
// 0xCB9B RES 3, E
void cpu::res_3_e();
// 0xCB9C RES 3, H
void cpu::res_3_h();
// 0xCB9D RES 3, L
void cpu::res_3_l();
// 0xCB9E RES 3, (HL)
void cpu::res_3_hl();
// 0xCB9F RES 3, A
void cpu::res_3_a();
