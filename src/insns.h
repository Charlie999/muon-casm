
#ifndef CASM_INSNS_H
#define CASM_INSNS_H

#define INSN_MOV 0
#define INSN_ADD 1
#define INSN_INA 2
#define INSN_INB 3
#define INSN_JMP 4
#define INSN_CPSW 5

#define INSN_DW 6
#define INSN_DS 7

#define INSN_LDA 8
#define INSN_LDB 9
#define INSN_LDAI 10
#define INSN_LDBI 11
#define INSN_OTA 12
#define INSN_OTB 13
#define INSN_LDAPTR 14
#define INSN_OTAPTR 15
#define INSN_DEC 16
#define INSN_BRCH 17
#define INSN_AND 18
#define INSN_NOT 19
#define INSN_OR 20
#define INSN_XOR 21
#define INSN_SHLA 22
#define INSN_ELDA 23
#define INSN_ELDB 24
#define INSN_SHRA 25
#define INSN_CMP 26
#define INSN_CMPBH 27
#define INSN_IRET 28
#define INSN_IE 29
#define INSN_SMM 30
#define INSN_CALL 31
#define INSN_IJMP 32
#define INSN_DP 33
#define INSN_SUB 34
#define INSN_SCALL 35

#define INSN_HCF 9999

#define CPU_NOP 0
#define CPU_JMP 1
#define CPU_MOV 2
#define CPU_EJMP 3
#define CPU_EMOV 4
#define CPU_LDA 5
#define CPU_LDB 6
#define CPU_LDBI 7
#define CPU_LDAI 8
#define CPU_OTA 9
#define CPU_OTB 10

#define CPU_BRCH   0x0B
#define CPU_IE     0x0E
#define CPU_SMM    0x0F

#define CPU_LDAPTR 0x10
#define CPU_OTAPTR 0x11
#define CPU_ADD    0x12
#define CPU_DEC    0x13
#define CPU_EOTA   0x14
#define CPU_ANDINSN    0x15 // changed because of Linux's headers
#define CPU_NOT    0x16
#define CPU_ORINSN     0x17
#define CPU_XORINSN    0x18
#define CPU_SHLA   0x19
#define CPU_ELDA   0x20
#define CPU_ELDB   0x21
#define CPU_SHRA   0x22
#define CPU_CMP    0x23
#define CPU_CMPBH  0x24
#define CPU_IRET   0x25
#define CPU_IJMP   0x26
#define CPU_ELDAI  0x27
#define CPU_SUB    0x28
#define CPU_EOTB   0x29

#define CPU_CPSW 0xff
#define CPU_INA 0xff
#define CPU_INB 0xff

#define CPU_HCF 0xff

#define UC_NOP  0x00
#define UC_LD   0x01
#define UC_AW   0x02
#define UC_BW   0x03
#define UC_SMM  0x04
#define UC_LA   0x05
#define UC_AWI  0x06
#define UC_BWI  0x07
#define UC_PSWC 0x08
#define UC_PCW  0x09
#define UC_BCHK 0x0a
#define UC_ALU  0x0b
#define UC_ALUL 0x0c
#define UC_IE   0x0d
#define UC_END  0x0f

#define UCASM_SETINSN 0xFF

#define VM_HALT 0xFF

#endif //CASM_INSNS_H
