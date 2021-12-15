
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

#define VM_HALT 0xFF

#endif //CASM_INSNS_H
