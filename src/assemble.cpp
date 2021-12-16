#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include "insns.h"

std::vector<std::string> split(const std::string& s, const std::string& delim) {
    std::vector<std::string> ret;

    auto start = 0U;
    auto end = s.find(delim);
    while (end != std::string::npos)
    {
        ret.push_back(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);
    }

    ret.push_back(s.substr(start, end));
    return ret;
}

int stoit(const std::string& opcr) {
    std::string opc_ = std::string(opcr);
    std::transform(opc_.begin(), opc_.end(), opc_.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    char opc[opc_.length()+1];
    opc[opc_.length()] = 0;
    strcpy(opc, opc_.c_str());
    if (strcmp(opc, "mov")==0) {
        return INSN_MOV;
    } else if (strcmp(opc, "add")==0) {
        return INSN_ADD;
    } else if (strcmp(opc, "ina")==0) {
        return INSN_INA;
    } else if (strcmp(opc, "inb")==0) {
        return INSN_INB;
    } else if (strcmp(opc, "jmp")==0) {
        return INSN_JMP;
    } else if (strcmp(opc, "cpsw")==0) {
        return INSN_CPSW;
    } else if (strcmp(opc, "dw")==0) {
        return INSN_DW;
    } else if (opc[0] == ':') {
        return INSN_DW;
    } else {
        printf("Unknown instruction [%s]\n",opc);
        exit(1);
    }
}

unsigned int decodeint(std::string a) {
    std::string s;
    if (a.length()>=2) {
        char sr[a.length() - 2];
        sr[a.length() - 2] = 0;
        memcpy(sr, a.c_str() + 2, a.length() - 2);
        s = std::string(sr);
    }
    if (a[0] == '0' && a[1] == 'x')
        return std::stoul(s, nullptr, 16);
    if (a[0] == '0' && a[1] == 'b')
        return std::stoul(s, nullptr, 2);
    return std::stoul(a, nullptr, 10);
}

void ierror0(const std::string& reason, const std::string& insn) {
    printf("error %s: invalid insn %s\n", reason.c_str(), insn.c_str());
    exit(1);
}
void ierror1(const std::string& reason, const std::string& operand) {
    printf("error %s: invalid operand %s\n",reason.c_str(),operand.c_str());
    exit(1);
}

#define uint unsigned int

std::vector<unsigned char> assemble(const std::string& insnraw) {
    std::vector<std::string> insn = split(insnraw," ");

    int itype = stoit(insn.at(0));

    std::vector<unsigned char> ret;

    switch (itype) {
        case INSN_MOV: {
            if (insn.size() != 3)
                ierror0("invalid instruction format",insnraw);
            uint b = decodeint(insn.at(1));
            uint a = decodeint(insn.at(2));
            if (a <= 0xFFFF) {
                ret.push_back(CPU_MOV);
                ret.push_back((a&0xFF00)>>8);
                ret.push_back(a&0xFF);

                ret.push_back((b&0xFF0000)>>16);
                ret.push_back((b&0xFF00)>>8);
                ret.push_back(b&0xFF);
            } else {
                ret.push_back(CPU_EMOV);
                ret.push_back(0);
                ret.push_back(0);

                ret.push_back((a&0xFF0000)>>16);
                ret.push_back((a&0xFF00)>>8);
                ret.push_back(a&0xFF);

                ret.push_back((b&0xFF0000)>>16);
                ret.push_back((b&0xFF00)>>8);
                ret.push_back(b&0xFF);
            }
            break;
        }
        case INSN_ADD: {
            if (insn.size() != 4)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1));
            uint b = decodeint(insn.at(2));
            uint c = decodeint(insn.at(3));

            ret.push_back(CPU_IADD);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            ret.push_back((b&0xFF0000)>>16);
            ret.push_back((b&0xFF00)>>8);
            ret.push_back(b&0xFF);

            ret.push_back((c&0xFF0000)>>16);
            ret.push_back((c&0xFF00)>>8);
            ret.push_back(c&0xFF);

            break;
        }
        case INSN_INA: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1));

            ret.push_back(CPU_INA);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_INB: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1));

            ret.push_back(CPU_INB);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_JMP: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1));
            if (a <= 0xFFFF) {
                ret.push_back(CPU_JMP);
                ret.push_back((a&0xFF00)>>8);
                ret.push_back(a&0xFF);
            } else {
                ret.push_back(CPU_EJMP);
                ret.push_back(0);
                ret.push_back(0);

                ret.push_back((a&0xFF0000)>>16);
                ret.push_back((a&0xFF00)>>8);
                ret.push_back(a&0xFF);
            }

            break;
        }
        case INSN_CPSW: {
            ierror0("not implemented",insnraw);
            break;
        }
        case INSN_DW: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1));

            ret.push_back((a&0xFF0000)>>16);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        default: {
            printf("Unknown itype %d\n",itype);
            exit(1);
        }
    }

    return ret;
}