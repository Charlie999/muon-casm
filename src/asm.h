
#ifndef CASM_ASM_H
#define CASM_ASM_H

std::vector<unsigned char> assemble(const std::string&,bool,std::vector<unsigned int>*);
std::vector<unsigned char> ucassemble(const std::vector<std::string>&);
void emulate(const std::vector<std::string>&, unsigned char*, const std::string&, int, bool, bool, bool);

void assemble_resolve_final(std::vector<unsigned int>*);

void ierror0(const std::string& reason, const std::string& insn);
std::vector<std::string> split(const std::string& s, const std::string& delim);
unsigned int decodeint(std::string a, unsigned int i, unsigned int i1, bool b, bool b1);

#endif //CASM_ASM_H
