
#ifndef CASM_INSNS_H
#define CASM_INSNS_H

#define INSN_MOV 0
#define INSN_ADD 1
#define INSN_INA 2
#define INSN_INB 3
#define INSN_JMP 4
#define INSN_CPSW 5

#define INSN_DW 6

#define CPU_NOP 0
#define CPU_JMP 1
#define CPU_MOV 2
#define CPU_EJMP 3
#define CPU_EMOV 4
#define CPU_CPSW 5
#define CPU_IADD 6
#define CPU_INA 7
#define CPU_INB 8

#define UC_NOP  0x00
#define UC_LD   0x01
#define UC_AW   0x02
#define UC_BW   0x03
#define UC_CW   0x04
#define UC_LA   0x05
#define UC_AWI  0x06
#define UC_BWI  0x07
#define UC_PSWC 0x08
#define UC_PCW  0x09
#define UC_ALU  0x0a
#define UC_ALUL 0x0b
#define UC_IE   0x0c
#define UC_END  0x0f

#define UCASM_SETINSN 0xFF

#define VM_HALT 0xFF

#endif //CASM_INSNS_H
