
#ifndef CASM_ASM_H
#define CASM_ASM_H

typedef unsigned int uint;

typedef struct gotentry_t {
    char fname[256];
    unsigned int ptr;
} gotentry;

struct assembleropts {
    bool quiet;
    bool onlyresolveafter;
    bool dumplabels;
    bool mulink;
};

struct mulink_lookup {
    uint ptr;
    uint mask;
    char name[256];
};

struct label {
    char name[256]{};
    uint ptr{};
};

struct labellookup {
    char name[256]{};
    uint ptr{};
    uint mask{};
};

void assembler_setopts(struct assembleropts);
std::vector<unsigned char> assemble(const std::string&,bool,std::vector<unsigned int>*);
void assembler_org(uint, std::vector<unsigned int>*);
void addgotentries(const std::vector<gotentry>& got);
const std::vector<gotentry>& getgotentries();

std::vector<unsigned char> ucassemble(const std::vector<std::string>&);

void emulate(const std::vector<std::string>&, unsigned char*, const std::string&, int, bool, bool, bool);
void setemulatormem(long,uint);
void emulator_set_mass_storage_reg(uint* arr);

std::vector<struct mulink_lookup> assemble_resolve_final(std::vector<unsigned int>*);
std::vector<struct label> assembler_get_labels();

void ierror0(const std::string& reason, const std::string& insn);
std::vector<std::string> split(const std::string& s, const std::string& delim);
unsigned int decodeint(std::string a, unsigned int i, unsigned int i1, bool b, bool b1);

#endif //CASM_ASM_H
