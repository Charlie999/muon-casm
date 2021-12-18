#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include "insns.h"
#include "asm.h"

#define uint unsigned int
#define uchar unsigned char

int ucstoit(const std::string& opcr) {
    std::string opc_ = std::string(opcr);
    std::transform(opc_.begin(), opc_.end(), opc_.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    char opc[opc_.length()+1];
    opc[opc_.length()] = 0;
    strcpy(opc, opc_.c_str());
    if (strcmp(opc, "nop")==0) {
        return UC_NOP;
    } else if (strcmp(opc, "ld")==0) {
        return UC_LD;
    } else if (strcmp(opc, "aw")==0 || strcmp(opc, "lda")==0) {
        return UC_AW;
    } else if (strcmp(opc, "bw")==0 || strcmp(opc, "ldb")==0) {
        return UC_BW;
    } else if (strcmp(opc, "smm")==0) {
        return UC_SMM;
    } else if (strcmp(opc, "la")==0) {
        return UC_LA;
    } else if (strcmp(opc, "awi")==0) {
        return UC_AWI;
    } else if (strcmp(opc, "bwi")==0) {
        return UC_BWI;
    } else if (strcmp(opc, "pswc")==0) {
        return UC_PSWC;
    } else if (strcmp(opc, "pcw")==0) {
        return UC_PCW;
    } else if (strcmp(opc, "bchk")==0) {
        return UC_BCHK;
    } else if (strcmp(opc, "alu")==0) {
        return UC_ALU;
    } else if (strcmp(opc, "alul")==0) {
        return UC_ALUL;
    } else if (strcmp(opc, "ie")==0) {
        return UC_IE;
    } else if (strcmp(opc, "end")==0) {
        return UC_END;
    } else if (opc[0] == ':') {
        return UCASM_SETINSN;
    } else {
        printf("Unknown instruction [%s]\n",opc);
        exit(1);
    }
}

uint curinsn = 0;
uchar ucount[256] = {0};

std::vector<unsigned char> ucassemble(const std::vector<std::string>& insns) {
    uchar ucrom[4096] = {0};
    memset(ucrom, 0xf, 4096);

    for (const auto& l : insns) {
        std::vector<std::string> parts = split(l, " ");

        if (parts.size() != 1 && parts.size() != 2)
            ierror0("too many or too little parts", l);

        int i = ucstoit(parts.at(0));
        //printf("ucstoit(%s): %d\n",parts.at(0).c_str(),i);

        if (i == UCASM_SETINSN) {
            if (l.length()<2)
                ierror0("must specify insn number",l);

            std::string n;
            n.append(l.c_str()+1);
            uint t = decodeint(n);

            if (t > 256)
                ierror0("invalid insn number",l);

            curinsn = t;
        } else if (i == UC_ALU || i == UC_ALUL || i == UC_SMM) {
            if (ucount[curinsn] >= 16)
                ierror0("too many uops",l);

            if (parts.size()!=2)
                ierror0("invalid ALU uop format",l);

            ucrom[(curinsn*16) + ucount[curinsn]] = i | ((decodeint(parts.at(1))&0xF)<<4);
            ucount[curinsn]++;
        } else {
            if (ucount[curinsn] >= 16)
                ierror0("too many uops",l);

            ucrom[(curinsn*16) + ucount[curinsn]] = i;
            ucount[curinsn]++;
            //printf("added uop at 0x%04X [c=%d]\n",(curinsn*16) + ucount[curinsn]-1,ucount[curinsn]);
        }
    }

    std::vector<uchar> ret;
    ret.reserve(4096);
    for (unsigned char & i : ucrom) {
        ret.push_back(i);
    }
    return ret;
}