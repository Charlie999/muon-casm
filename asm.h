
#ifndef CASM_ASM_H
#define CASM_ASM_H

std::vector<unsigned char> assemble(const std::string&);
void emulate(const std::vector<std::string>&, bool, bool);

#endif //CASM_ASM_H
