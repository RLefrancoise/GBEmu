#ifndef CPU_H
#define CPU_H

#include <QMap>
#include <QString>

#include <Utils.h>

namespace gb
{

class cpu : public utils::patterns::Singleton<cpu>
{
    friend class utils::patterns::Singleton<cpu>;

    enum REGISTERS
    {
        REGISTER_A = 0x0,
        REGISTER_B = 0x1,
        REGISTER_C = 0x2,
        REGISTER_D = 0x3,
        REGISTER_E = 0x4,
        REGISTER_H = 0x5,
        REGISTER_L = 0x6,
        REGISTER_NUMBER = 0x7
    };

    enum DOUBLE_REGISTERS
    {
        REGISTER_BC = 0x0,
        REGISTER_DE = 0x1,
        REGISTER_HL = 0x2
    };

    enum FLAGS
    {
        FLAG_UNK1   = 1,
        FLAG_UNK2   = 1<<1,
        FLAG_UNK3   = 1<<2,
        FLAG_UNK4   = 1<<3,
        FLAG_C      = 1<<4,
        FLAG_H      = 1<<5,
        FLAG_N      = 1<<6,
        FLAG_Z      = 1<<7
    };



    cpu();
    ~cpu();

    bool interpret_opcode();

    quint16 get_pc();

    void write_on_register(DOUBLE_REGISTERS reg, quint16 word);



    typedef void (cpu::*opcode_func)();

private:

    class opcode
    {
    public:
        opcode(QString mnemonic, quint8 length, quint8 cycles, opcode_func exec, quint8 not_exec_cycles = 0);

        QString mnemonic;
        quint8 length;
        quint8 cycles;
        quint8 not_exec_cycles;
        opcode_func exec; // pointer to the function which will execute the opcode
    };

    quint8                      R[REGISTER_NUMBER]                      ; //A, B, C, D, E, H, L
    quint8                      F                                       ; //flag register

    quint16                     SP                                      ; //stack pointer
    quint16                     PC                                      ; //program counter

    QMap<quint8, opcode*>       opcodes_table                           ; //1-byte long opcodes
    QMap<quint8, opcode*>       extended_opcodes_table                  ; //2-bytes long opcodes

    opcode*                     current_opcode                          ;
    quint8                      cycles_counter                          ;

    bool                        STOP                                    ;
    bool                        HALT                                    ;
    bool                        IME                                     ; //interrupts
    bool                        last_opcode_not_executed                ; //for jumps

    //flags functions
    bool z_flag();
    void check_z(quint16 val);
    void set_z();
    void reset_z();

    bool n_flag();
    void set_n();
    void reset_n();

    bool h_flag();
    void check_h_add8(quint8 val1, quint8 val2);
    void check_h_add16(quint16 val1, quint16 val2);
    void check_h_sub8(quint8 val1, quint8 val2);
    void set_h();
    void reset_h();

    bool c_flag();
    void check_c_rl(quint8 reg);
    void check_c_rr(quint8 reg);
    void check_c_add8(quint8 val1, quint8 val2);
    void check_c_add16(quint16 val1, quint16 val2);
    void check_c_sub8(quint8 val1, quint8 val2);
    void set_c();
    void reset_c();

    //opcodes functions

    void not_impl();

    //1-byte opcodes
    void nop();                                         // 0x00 NOP
    void ld_bc_d16();                                   // 0x01 LD BC, d16
    void ld_bc_a();                                     // 0x02 LD (BC), A
    void inc_bc();                                      // 0x03 INC BC
    void inc_b();                                       // 0x04 INC B
    void dec_b();                                       // 0x05 DEC B
    void ld_b_d8();                                     // 0x06 LD B, d8
    void rlca();                                        // 0x07 RLCA
    void ld_a16_sp();                                   // 0x08 LD (a16), SP
    void add_hl_bc();                                   // 0x09 ADD HL, BC
    void ld_a_bc();                                     // 0x0A LD A, (BC)
    void dec_bc();                                      // 0x0B DEC BC
    void inc_c();                                       // 0x0C INC C
    void dec_c();                                       // 0x0D DEC C
    void ld_c_d8();                                     // 0x0E LD C, d8
    void rrca();                                        // 0x0F RRCA

    void stop();                                        // 0x10 STOP 0
    void ld_de_d16();                                   // 0x11 LD DE, d16
    void ld_de_a();                                     // 0x12 LD (DE), A
    void inc_de();                                      // 0x13 INC DE
    void inc_d();                                       // 0x14 INC D
    void dec_d();                                       // 0x15 DEC D
    void ld_d_d8();                                     // 0x16 LD D, d8
    void rla();                                         // 0x17 RLA
    void jr_r8();                                       // 0x18 JR r8
    void add_hl_de();                                   // 0x19 ADD HL, DE
    void ld_a_de();                                     // 0x1A LD A, (DE)
    void dec_de();                                      // 0x1B DEC DE
    void inc_e();                                       // 0x1C INC E
    void dec_e();                                       // 0x1D DEC E
    void ld_e_d8();                                     // 0x1E LD E, d8
    void rra();                                         // 0x1F RRA

    void jr_nz_r8();                                    // 0x20 JR NZ, r8
    void ld_hl_d16();                                   // 0x21 LD HL, d16
    void ldi_hl_a();                                    // 0x22 LDI (HL), A
    void inc_hl();                                      // 0x23 INC HL
    void inc_h();                                       // 0x24 INC H
    void dec_h();                                       // 0x25 DEC H
    void ld_h_d8();                                     // 0x26 LD H, d8
    void daa();                                         // 0x27 DAA
    void jr_z_r8();                                     // 0x28 JR Z, r8
    void add_hl_hl();                                   // 0x29 ADD HL, HL
    void ldi_a_hl();                                    // 0x2A LDI A, (HL)
    void dec_hl();                                      // 0x2B DEC HL
    void inc_l();                                       // 0x2C INC L
    void dec_l();                                       // 0x2D DEC L
    void ld_l_d8();                                     // 0x2E LD L, d8
    void cpl();                                         // 0x2F CPL

    void jr_nc_r8();                                    // 0x30 JR NC, r8
    void ld_sp_d16();                                   // 0x31 LD SP, d16
    void ldd_hl_a();                                    // 0x32 LDD HL, A
    void inc_sp();                                      // 0x33 INC SP
    void inc_hl_();                                     // 0x34 INC (HL)
    void dec_hl_();                                     // 0x35 DEC (HL)
    void ld_hl_d8();                                    // 0x36 LD (HL), d8
    void scf();                                         // 0x37 SCF
    void jr_c_r8();                                     // 0x38 JR C, r8
    void add_hl_sp();                                   // 0x39 ADD HL, SP
    void ldd_a_hl();                                    // 0x3A LDD A, (HL)
    void dec_sp();                                      // 0x3B DEC SP
    void inc_a();                                       // 0x3C INC A
    void dec_a();                                       // 0x3D DEC A
    void ld_a_d8();                                     // 0x3E LD A, d8
    void ccf();                                         // 0x3F CCF

    void ld_b_b();                                      // 0x40 LD B, B
    void ld_b_c();                                      // 0x41 LD B, C
    void ld_b_d();                                      // 0x42 LD B, D
    void ld_b_e();                                      // 0x43 LD B, E
    void ld_b_h();                                      // 0x44 LD B, H
    void ld_b_l();                                      // 0x45 LD B, L
    void ld_b_hl();                                     // 0x46 LD B, (HL)
    void ld_b_a();                                      // 0x47 LD B, A
    void ld_c_b();                                      // 0x48 LD C, B
    void ld_c_c();                                      // 0x49 LD C, C
    void ld_c_d();                                      // 0x4A LD C, D
    void ld_c_e();                                      // 0x4B LD C, E
    void ld_c_h();                                      // 0x4C LD C, H
    void ld_c_l();                                      // 0x4D LD C, L
    void ld_c_hl();                                     // 0x4E LD C, (HL)
    void ld_c_a();                                      // 0x4F LD C, A

    void ld_d_b();                                      // 0x50 LD D, B
    void ld_d_c();                                      // 0x51 LD D, C
    void ld_d_d();                                      // 0x52 LD D, D
    void ld_d_e();                                      // 0x53 LD D, E
    void ld_d_h();                                      // 0x54 LD D, H
    void ld_d_l();                                      // 0x55 LD D, L
    void ld_d_hl();                                     // 0x56 LD D, (HL)
    void ld_d_a();                                      // 0x57 LD D, A
    void ld_e_b();                                      // 0x58 LD E, B
    void ld_e_c();                                      // 0x59 LD E, C
    void ld_e_d();                                      // 0x5A LD E, D
    void ld_e_e();                                      // 0x5B LD E, E
    void ld_e_h();                                      // 0x5C LD E, H
    void ld_e_l();                                      // 0x5D LD E, L
    void ld_e_hl();                                     // 0x5E LD E, (HL)
    void ld_e_a();                                      // 0x5F LD E, A

    void ld_h_b();                                      // 0x60 LD H, B
    void ld_h_c();                                      // 0x61 LD H, C
    void ld_h_d();                                      // 0x62 LD H, D
    void ld_h_e();                                      // 0x63 LD H, E
    void ld_h_h();                                      // 0x64 LD H, H
    void ld_h_l();                                      // 0x65 LD H, L
    void ld_h_hl();                                     // 0x66 LD H, (HL)
    void ld_h_a();                                      // 0x67 LD H, A
    void ld_l_b();                                      // 0x68 LD L, B
    void ld_l_c();                                      // 0x69 LD L, C
    void ld_l_d();                                      // 0x6A LD L, D
    void ld_l_e();                                      // 0x6B LD L, E
    void ld_l_h();                                      // 0x6C LD L, H
    void ld_l_l();                                      // 0x6D LD L, L
    void ld_l_hl();                                     // 0x6E LD L, (HL)
    void ld_l_a();                                      // 0x6F LD L, A

    void ld_hl_b();                                     // 0x70 LD (HL), B
    void ld_hl_c();                                     // 0x71 LD (HL), C
    void ld_hl_d();                                     // 0x72 LD (HL), D
    void ld_hl_e();                                     // 0x73 LD (HL), E
    void ld_hl_h();                                     // 0x74 LD (HL), H
    void ld_hl_l();                                     // 0x75 LD (HL), L
    void halt();                                        // 0x76 HALT
    void ld_hl_a();                                     // 0x77 LD (HL), A
    void ld_a_b();                                      // 0x78 LD A, B
    void ld_a_c();                                      // 0x79 LD A, C
    void ld_a_d();                                      // 0x7A LD A, D
    void ld_a_e();                                      // 0x7B LD A, E
    void ld_a_h();                                      // 0x7C LD A, H
    void ld_a_l();                                      // 0x7D LD A, L
    void ld_a_hl();                                     // 0x7E LD A, (HL)
    void ld_a_a();                                      // 0x7F LD A, A

    void add_a_b();                                     // 0x80 ADD A, B
    void add_a_c();                                     // 0x81 ADD A, C
    void add_a_d();                                     // 0x82 ADD A, D
    void add_a_e();                                     // 0x83 ADD A, E
    void add_a_h();                                     // 0x84 ADD A, H
    void add_a_l();                                     // 0x85 ADD A, L
    void add_a_hl();                                    // 0x86 ADD A, (HL)
    void add_a_a();                                     // 0x87 ADD A, A
    void adc_a_b();                                     // 0x88 ADC A, B
    void adc_a_c();                                     // 0x89 ADC A, C
    void adc_a_d();                                     // 0x8A ADC A, D
    void adc_a_e();                                     // 0x8B ADC A, E
    void adc_a_h();                                     // 0x8C ADC A, H
    void adc_a_l();                                     // 0x8D ADC A, L
    void adc_a_hl();                                    // 0x8E ADC A, (HL)
    void adc_a_a();                                     // 0x8F ADC A, A

    void sub_b();                                       // 0x90 SUB B
    void sub_c();                                       // 0x91 SUB C
    void sub_d();                                       // 0x92 SUB D
    void sub_e();                                       // 0x93 SUB E
    void sub_h();                                       // 0x94 SUB H
    void sub_l();                                       // 0x95 SUB L
    void sub_hl();                                      // 0x96 SUB (HL)
    void sub_a();                                       // 0x97 SUB A
    void sbc_a_b();                                     // 0x98 SBC A, B
    void sbc_a_c();                                     // 0x99 SBC A, C
    void sbc_a_d();                                     // 0x9A SBC A, D
    void sbc_a_e();                                     // 0x9B SBC A, E
    void sbc_a_h();                                     // 0x9C SBC A, H
    void sbc_a_l();                                     // 0x9D SBC A, L
    void sbc_a_hl();                                    // 0x9E SBC A, (HL)
    void sbc_a_a();                                     // 0x9F SBC A, A

    void and_b();                                       // 0xA0 AND B
    void and_c();                                       // 0xA1 AND C
    void and_d();                                       // 0xA2 AND D
    void and_e();                                       // 0xA3 AND E
    void and_h();                                       // 0xA4 AND H
    void and_l();                                       // 0xA5 AND L
    void and_hl();                                      // 0xA6 AND (HL)
    void and_a();                                       // 0xA7 AND A
    void xor_b();                                       // 0xA8 XOR B
    void xor_c();                                       // 0xA9 XOR C
    void xor_d();                                       // 0xAA XOR D
    void xor_e();                                       // 0xAB XOR E
    void xor_h();                                       // 0xAC XOR H
    void xor_l();                                       // 0xAD XOR L
    void xor_hl();                                      // 0xAE XOR (HL)
    void xor_a();                                       // 0xAF XOR A

    void or_b();                                        // 0xB0 OR B
    void or_c();                                        // 0xB1 OR C
    void or_d();                                        // 0xB2 OR D
    void or_e();                                        // 0xB3 OR E
    void or_h();                                        // 0xB4 OR H
    void or_l();                                        // 0xB5 OR L
    void or_hl();                                       // 0xB6 OR (HL)
    void or_a();                                        // 0xB7 OR A
    void cp_b();                                        // 0xB8 CP B
    void cp_c();                                        // 0xB9 CP C
    void cp_d();                                        // 0xBA CP D
    void cp_e();                                        // 0xBB CP E
    void cp_h();                                        // 0xBC CP H
    void cp_l();                                        // 0xBD CP L
    void cp_hl();                                       // 0xBE CP (HL)
    void cp_a();                                        // 0xBF CP A

    void ret_nz();                                      // 0xC0 RET NZ
    void pop_bc();                                      // 0xC1 POP BC
    void jp_nz_a16();                                   // 0xC2 JP NZ, a16
    void jp_a16();                                      // 0xC3 JP a16
    void call_nz_a16();                                 // 0xC4 CALL NZ, a16
    void push_bc();                                     // 0xC5 PUSH BC
    void add_a_d8();                                    // 0xC6 ADD A, d8
    void rst_00h();                                     // 0xC7 RST 00h
    void ret_z();                                       // 0xC8 RET Z
    void ret();                                         // 0xC9 RET
    void jp_z_a16();                                    // 0xCA JP Z, a16
    void prefix_cb();                                   // 0xCB PREFIX CB
    void call_z_a16();                                  // 0xCC CALL Z, a16
    void call_a16();                                    // 0xCD CALL a16
    void adc_a_d8();                                    // 0xCE ADC A, d8
    void rst_08h();                                     // 0xCF RST 08h

    void ret_nc();                                      // 0xD0 RET NC
    void pop_de();                                      // 0xD1 POP DE
    void jp_nc_a16();                                   // 0xD2 JP NC, a16
    //not implemented                                   // 0xD3 NOT IMPL
    void call_nc_a16();                                 // 0xD4 CALL NC, a16
    void push_de();                                     // 0xD5 PUSH DE
    void sub_d8();                                      // 0xD6 SUB d8
    void rst_10h();                                     // 0xD7 RST 10h
    void ret_c();                                       // 0xD8 RET C
    void reti();                                        // 0xD9 RETI
    void jp_c_a16();                                    // 0xDA JP C, a16
    //not implemented                                   // 0xDB NOT IMPL
    void call_c_a16();                                  // 0xDC CALL C, a16
    //not implemented                                   // 0xDD NOT IMPL
    void sbc_a_d8();                                    // 0xDE SBC A, d8
    void rst_18h();                                     // 0xDF RST 18h

    void ldh_a8_a();                                    // 0xE0 LDH (a8), A
    void pop_hl();                                      // 0xE1 POP HL
    void ld_c_a_();                                     // 0xE2 LD (C), A
    //not implemented                                   // 0xE3 NOT IMPL
    //not implemented                                   // 0xE4 NOT IMPL
    void push_hl();                                     // 0xE5 PUSH HL
    void and_d8();                                      // 0xE6 AND d8
    void rst_20h();                                     // 0xE7 RST 20h
    void add_sp_r8();                                   // 0xE8 ADD SP, r8
    void jp_hl();                                       // 0xE9 JP (HL)
    void ld_a16_a();                                    // 0xEA LD (a16), A
    //not implemented                                   // 0xEB NOT IMPL
    //not implemented                                   // 0xEC NOT IMPL
    //not implemented                                   // 0xED NOT IMPL
    void xor_d8();                                      // 0xEE XOR d8
    void rst_28h();                                     // 0xEF RST 28h

    void ldh_a_a8();                                    // 0xF0 LDH A, (a8)
    void pop_af();                                      // 0xF1 POP AF
    void ld_a_c_();                                     // 0xF2 LD A, (C)
    void di();                                          // 0xF3 DI
    //not implemented                                   // 0xF4 NOT IMPL
    void push_af();                                     // 0xF5 PUSH AF
    void or_d8();                                       // 0xF6 OR d8
    void rst_30h();                                     // 0xF7 RST 30h
    void ldhl_sp_r8();                                  // 0xF8 LDHL SP, r8
    void ld_sp_hl();                                    // 0xF9 LD SP, HL
    void ld_a_a16();                                    // 0xFA LD A, (a16)
    void ei();                                          // 0xFB EI
    //not implemented                                   // 0xFC NOT IMPL
    //not implemented                                   // 0xFD NOT IMPL
    void cp_d8();                                       // 0xFE CP d8
    void rst_38h();                                     // 0xFF RST 38h

    //2-bytes opcodes
    void rlc_b();                                       // 0xCB00 RLC B
    void rlc_c();                                       // 0xCB01 RLC C
    void rlc_d();                                       // 0xCB02 RLC D
    void rlc_e();                                       // 0xCB03 RLC E
    void rlc_h();                                       // 0xCB04 RLC H
    void rlc_l();                                       // 0xCB05 RLC L
    void rlc_hl();                                      // 0xCB06 RLC (HL)
    void rlc_a();                                       // 0xCB07 RLC A
    void rrc_b();                                       // 0xCB08 RRC B
    void rrc_c();                                       // 0xCB09 RRC C
    void rrc_d();                                       // 0xCB0A RRC D
    void rrc_e();                                       // 0xCB0B RRC E
    void rrc_h();                                       // 0xCB0C RRC H
    void rrc_l();                                       // 0xCB0D RRC L
    void rrc_hl();                                      // 0xCB0E RRC (HL)
    void rrc_a();                                       // 0xCB0F RRC A

    void rl_b();                                        // 0xCB10 RL B
    void rl_c();                                        // 0xCB11 RL C
    void rl_d();                                        // 0xCB12 RL D
    void rl_e();                                        // 0xCB13 RL E
    void rl_h();                                        // 0xCB14 RL H
    void rl_l();                                        // 0xCB15 RL L
    void rl_hl();                                       // 0xCB16 RL (HL)
    void rl_a();                                        // 0xCB17 RL A
    void rr_b();                                        // 0xCB18 RR B
    void rr_c();                                        // 0xCB19 RR C
    void rr_d();                                        // 0xCB1A RR D
    void rr_e();                                        // 0xCB1B RR E
    void rr_h();                                        // 0xCB1C RR H
    void rr_l();                                        // 0xCB1D RR L
    void rr_hl();                                       // 0xCB1E RR (HL)
    void rr_a();                                        // 0xCB1F RR A

    void sla_b();                                       // 0xCB20 SLA B
    void sla_c();                                       // 0xCB21 SLA C
    void sla_d();                                       // 0xCB22 SLA D
    void sla_e();                                       // 0xCB23 SLA E
    void sla_h();                                       // 0xCB24 SLA H
    void sla_l();                                       // 0xCB25 SLA L
    void sla_hl();                                      // 0xCB26 SLA (HL)
    void sla_a();                                       // 0xCB27 SLA A
    void sra_b();                                       // 0xCB28 SRA B
    void sra_c();                                       // 0xCB29 SRA C
    void sra_d();                                       // 0xCB2A SRA D
    void sra_e();                                       // 0xCB2B SRA E
    void sra_h();                                       // 0xCB2C SRA H
    void sra_l();                                       // 0xCB2D SRA L
    void sra_hl();                                      // 0xCB2E SRA (HL)
    void sra_a();                                       // 0xCB2F SRA A

    void swap_b();                                      // 0xCB30 SWAP B
    void swap_c();                                      // 0xCB31 SWAP C
    void swap_d();                                      // 0xCB32 SWAP D
    void swap_e();                                      // 0xCB33 SWAP E
    void swap_h();                                      // 0xCB34 SWAP H
    void swap_l();                                      // 0xCB35 SWAP L
    void swap_hl();                                     // 0xCB36 SWAP (HL)
    void swap_a();                                      // 0xCB37 SWAP A
    void srl_b();                                       // 0xCB38 SRL B
    void srl_c();                                       // 0xCB39 SRL C
    void srl_d();                                       // 0xCB3A SRL D
    void srl_e();                                       // 0XCB3B SRL E
    void srl_h();                                       // 0xCB3C SRL H
    void srl_l();                                       // 0xCB3D SRL L
    void srl_hl();                                      // 0xCB3E SRL (HL)
    void srl_a();                                       // 0xCB3F SRL A

    void bit_0_b();                                     // 0xCB40 BIT 0, B
    void bit_0_c();                                     // 0xCB41 BIT 0, C
    void bit_0_d();                                     // 0xCB42 BIT 0, D
    void bit_0_e();                                     // 0xCB43 BIT 0, E
    void bit_0_h();                                     // 0xCB44 BIT 0, H
    void bit_0_l();                                     // 0xCB45 BIT 0, L
    void bit_0_hl();                                    // 0xCB46 BIT 0, (HL)
    void bit_0_a();                                     // 0xCB47 BIT 0, A
    void bit_1_b();                                     // 0xCB48 BIT 1, B
    void bit_1_c();                                     // 0xCB49 BIT 1, C
    void bit_1_d();                                     // 0xCB4A BIT 1, D
    void bit_1_e();                                     // 0xCB4B BIT 1, E
    void bit_1_h();                                     // 0xCB4C BIT 1, H
    void bit_1_l();                                     // 0xCB4D BIT 1, L
    void bit_1_hl();                                    // 0xCB4E BIT 1, (HL)
    void bit_1_a();                                     // 0xCB4F BIT 1, A

    void bit_2_b();                                     // 0xCB50 BIT 2, B
    void bit_2_c();                                     // 0xCB51 BIT 2, C
    void bit_2_d();                                     // 0xCB52 BIT 2, D
    void bit_2_e();                                     // 0xCB53 BIT 2, E
    void bit_2_h();                                     // 0xCB54 BIT 2, H
    void bit_2_l();                                     // 0xCB55 BIT 2, L
    void bit_2_hl();                                    // 0xCB56 BIT 2, (HL)
    void bit_2_a();                                     // 0xCB57 BIT 2, A
    void bit_3_b();                                     // 0xCB58 BIT 3, B
    void bit_3_c();                                     // 0xCB59 BIT 3, C
    void bit_3_d();                                     // 0xCB5A BIT 3, D
    void bit_3_e();                                     // 0xCB5B BIT 3, E
    void bit_3_h();                                     // 0xCB5C BIT 3, H
    void bit_3_l();                                     // 0xCB5D BIT 3, L
    void bit_3_hl();                                    // 0xCB5E BIT 3, (HL)
    void bit_3_a();                                     // 0xCB5F BIT 3, A

    void bit_4_b();                                     // 0xCB60 BIT 4, B
    void bit_4_c();                                     // 0xCB61 BIT 4, C
    void bit_4_d();                                     // 0xCB62 BIT 4, D
    void bit_4_e();                                     // 0xCB63 BIT 4, E
    void bit_4_h();                                     // 0xCB64 BIT 4, H
    void bit_4_l();                                     // 0xCB65 BIT 4, L
    void bit_4_hl();                                    // 0xCB66 BIT 4, (HL)
    void bit_4_a();                                     // 0xCB67 BIT 4, A
    void bit_5_b();                                     // 0xCB68 BIT 5, B
    void bit_5_c();                                     // 0xCB69 BIT 5, C
    void bit_5_d();                                     // 0xCB6A BIT 5, D
    void bit_5_e();                                     // 0xCB6B BIT 5, E
    void bit_5_h();                                     // 0xCB6C BIT 5, H
    void bit_5_l();                                     // 0xCB6D BIT 5, L
    void bit_5_hl();                                    // 0xCB6E BIT 5, (HL)
    void bit_5_a();                                     // 0xCB6F BIT 5, A

    void bit_6_b();                                     // 0xCB70 BIT 6, B
    void bit_6_c();                                     // 0xCB71 BIT 6, C
    void bit_6_d();                                     // 0xCB72 BIT 6, D
    void bit_6_e();                                     // 0xCB73 BIT 6, E
    void bit_6_h();                                     // 0xCB74 BIT 6, H
    void bit_6_l();                                     // 0xCB75 BIT 6, L
    void bit_6_hl();                                    // 0xCB76 BIT 6, (HL)
    void bit_6_a();                                     // 0xCB77 BIT 6, A
    void bit_7_b();                                     // 0xCB78 BIT 7, B
    void bit_7_c();                                     // 0xCB79 BIT 7, C
    void bit_7_d();                                     // 0xCB7A BIT 7, D
    void bit_7_e();                                     // 0xCB7B BIT 7, E
    void bit_7_h();                                     // 0xCB7C BIT 7, H
    void bit_7_l();                                     // 0xCB7D BIT 7, L
    void bit_7_hl();                                    // 0xCB7E BIT 7, (HL)
    void bit_7_a();                                     // 0xCB7F BIT 7, A

    void res_0_b();                                     // 0xCB80 RES 0, B
    void res_0_c();                                     // 0xCB81 RES 0, C
    void res_0_d();                                     // 0xCB82 RES 0, D
    void res_0_e();                                     // 0xCB83 RES 0, E
    void res_0_h();                                     // 0xCB84 RES 0, H
    void res_0_l();                                     // 0xCB85 RES 0, L
    void res_0_hl();                                    // 0xCB86 RES 0, (HL)
    void res_0_a();                                     // 0xCB87 RES 0, A
    void res_1_b();                                     // 0xCB88 RES 1, B
    void res_1_c();                                     // 0xCB89 RES 1, C
    void res_1_d();                                     // 0xCB8A RES 1, D
    void res_1_e();                                     // 0xCB8B RES 1, E
    void res_1_h();                                     // 0xCB8C RES 1, H
    void res_1_l();                                     // 0xCB8D RES 1, L
    void res_1_hl();                                    // 0xCB8E RES 1, (HL)
    void res_1_a();                                     // 0xCB8F RES 1, A

    void res_2_b();                                     // 0xCB90 RES 2, B
    void res_2_c();                                     // 0xCB91 RES 2, C
    void res_2_d();                                     // 0xCB92 RES 2, D
    void res_2_e();                                     // 0xCB93 RES 2, E
    void res_2_h();                                     // 0xCB94 RES 2, H
    void res_2_l();                                     // 0xCB95 RES 2, L
    void res_2_hl();                                    // 0xCB96 RES 2, (HL)
    void res_2_a();                                     // 0xCB97 RES 2, A
    void res_3_b();                                     // 0xCB98 RES 3, B
    void res_3_c();                                     // 0xCB99 RES 3, C
    void res_3_d();                                     // 0xCB9A RES 3, D
    void res_3_e();                                     // 0xCB9B RES 3, E
    void res_3_h();                                     // 0xCB9C RES 3, H
    void res_3_l();                                     // 0xCB9D RES 3, L
    void res_3_hl();                                    // 0xCB9E RES 3, (HL)
    void res_3_a();                                     // 0xCB9F RES 3, A
};

}

#endif // CPU_H
