#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <iostream>
#include "insns.h"

#define ENABLE_LABEL_ENGINE

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
    } else if (strcmp(opc, "sub")==0) {
        return INSN_SUB;
    } else if (strcmp(opc, "and")==0) {
        return INSN_AND;
    } else if (strcmp(opc, "not")==0) {
        return INSN_NOT;
    } else if (strcmp(opc, "or")==0) {
        return INSN_OR;
    } else if (strcmp(opc, "xor")==0) {
        return INSN_XOR;
    } else if (strcmp(opc, "shla")==0) {
        return INSN_SHLA;
    } else if (strcmp(opc, "shra")==0) {
        return INSN_SHRA;
    } else if (strcmp(opc, "ina")==0) {
        return INSN_INA;
    } else if (strcmp(opc, "inb")==0) {
        return INSN_INB;
    } else if (strcmp(opc, "jmp")==0) {
        return INSN_JMP;
    } else if (strcmp(opc, "lda")==0) {
        return INSN_LDA;
    } else if (strcmp(opc, "ldb")==0) {
        return INSN_LDB;
    } else if (strcmp(opc, "elda")==0) {
        return INSN_ELDA;
    } else if (strcmp(opc, "eldb")==0) {
        return INSN_ELDB;
    } else if (strcmp(opc, "ldbi")==0) {
        return INSN_LDBI;
    } else if (strcmp(opc, "ldai")==0) {
        return INSN_LDAI;
    } else if (strcmp(opc, "ota")==0) {
        return INSN_OTA;
    } else if (strcmp(opc, "otb")==0) {
        return INSN_OTB;
    } else if (strcmp(opc, "ldaptr")==0) {
        return INSN_LDAPTR;
    } else if (strcmp(opc, "otaptr")==0) {
        return INSN_OTAPTR;
    } else if (strcmp(opc, "dec")==0) {
        return INSN_DEC;
    } else if (strcmp(opc, "brch")==0) {
        return INSN_BRCH;
    } else if (strcmp(opc, "cmp")==0) {
        return INSN_CMP;
    } else if (strcmp(opc, "dw")==0) {
        return INSN_DW;
    } else if (strcmp(opc, "ds")==0) {
        return INSN_DS;
    } else if (strcmp(opc, "dp")==0) {
        return INSN_DP;
    } else if (strcmp(opc, "cmpbh")==0) {
        return INSN_CMPBH;
    } else if (strcmp(opc, "iret")==0) {
        return INSN_IRET;
    } else if (strcmp(opc, "hcf")==0) {
        return INSN_HCF;
    } else if (strcmp(opc, "ie")==0) {
        return INSN_IE;
    } else if (strcmp(opc, "smm")==0) {
        return INSN_SMM;
    } else if (strcmp(opc, "call")==0) {
        return INSN_CALL;
    } else if (strcmp(opc, "ijmp")==0) {
        return INSN_IJMP;
    } else {
        printf("Unknown instruction [%s]\n",opc);
        exit(1);
    }
}

bool isComma(char ch) { return ch==','; }

#define uint unsigned int

struct label {
    char name[256]{};
    uint ptr{};
};

struct labellookup {
    char name[256]{};
    uint ptr{};
    uint mask{};
};

std::vector<struct label> labelptrs;
std::vector<struct labellookup> labelqueue;

label* getlabel(const std::string& n) {
    for (const struct label& lbl : labelptrs) {
        if (strcmp(lbl.name, n.c_str()) == 0) {
            auto *lblret = (struct label*)malloc(sizeof(struct label));
            strcpy(lblret->name, lbl.name);
            lblret->ptr = lbl.ptr;
            return lblret;
        }
    }
    return nullptr;
}

#define lelog(...) {printf("[labelengine] "); printf(__VA_ARGS__);}

unsigned int decodeint(std::string a, uint _ptr, uint imask, bool lookuplabels, bool enablelabels = true) {
    try {
        std::string s;
        if (a.length() >= 2) {
#ifdef ENABLE_LABEL_ENGINE
            std::string t(a);
            t.erase(std::remove(t.begin(), t.end(), ','), t.end());
            if (t.c_str()[0] == '{' && t.c_str()[t.length() - 1] == '}' && enablelabels) {
                std::string lblname(t.c_str() + 1);
                lblname.pop_back();
                lelog("get label %s\n", lblname.c_str());
                label *lbl = getlabel(lblname);
                if (!lbl) {
                    lelog("label %s not found\n", lblname.c_str());
                    if (!lookuplabels)
                        return 0xFFFFFFE0;
                    lelog("label lookup queued for %s\n", lblname.c_str());
                    labellookup llk;
                    strncpy(llk.name, lblname.c_str(), 256);
                    llk.ptr = _ptr;
                    llk.mask = imask;
                    labelqueue.push_back(llk);
                    return 0;
                }
                lelog("got label ptr for %s: 0x%06X\n", lblname.c_str(), lbl->ptr);
                uint ptr = lbl->ptr;
                free(lbl);
                return ptr;
            }
#endif

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
    } catch (std::exception e) {
        std::cerr << "Number parsing exception for [" << a << "]: \n" << e.what() << std::endl;
        exit(-1);
    }
}

void ierror0(const std::string& reason, const std::string& insn) {
    printf("error %s: invalid insn %s\n", reason.c_str(), insn.c_str());
    exit(1);
}
void ierror1(const std::string& reason, const std::string& operand) {
    printf("error %s: invalid operand %s\n",reason.c_str(),operand.c_str());
    exit(1);
}

uint ptr = 0;

std::vector<unsigned char> assemble(const std::string& insnraw,bool movswap,std::vector<unsigned int>* outdata) {
    std::vector<std::string> insn = split(insnraw," ");
    std::vector<unsigned char> ret;

#ifdef ENABLE_LABEL_ENGINE
    if (insnraw.c_str()[0]==':') {
        std::string lblname(insnraw.c_str()+1);
        lelog("label: %s at ptr=0x%06X\n",lblname.c_str(),ptr);
        label lbl;
        strncpy(lbl.name, std::string(lblname).c_str(), 256);
        lbl.ptr = ptr;
        labelptrs.push_back(lbl);

        lelog("running lookups for label %s\n",lbl.name);
        for (auto llk=labelqueue.begin(); llk!=labelqueue.end();) {
            if (strncmp(llk->name, lbl.name, 256) == 0) {
                label *llkb = getlabel(std::string(llk->name));
                if (!llkb) {
                    lelog("warning: unknown label %s\n", llk->name);
                    continue;
                }

                lelog("applying lookup at insn %s [ptr=0x%X, mask=0x%06X, np=0x%06X]\n", insnraw.c_str(), llk->ptr,
                       llk->mask, llkb->ptr);
                (*outdata)[llk->ptr] = outdata->at(llk->ptr) | (llkb->ptr & llk->mask);

                labelqueue.erase(llk);
            } else {
                llk++;
            }
        }

        return ret;
    }
#endif

    int itype = stoit(insn.at(0));

    switch (itype) {
        case INSN_MOV: {
            if (insn.size() != 3)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);
            uint b = decodeint(insn.at(2),ptr+1, 0xFFFFFF,true);
            if (movswap) {
                uint c = a;
                a = b;
                b = c;
            }
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
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_ADD);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_SUB: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_SUB);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_AND: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_ANDINSN);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_NOT: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_NOT);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_OR: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_ORINSN);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_XOR: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_XORINSN);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_SHLA: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_SHLA);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_SHRA: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_SHRA);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_INA: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_INA);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_INB: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_INB);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_CMP: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);

            ret.push_back(CPU_CMP);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_JMP: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,false);
            if (a <= 0xFFFF) {
                a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);
                ret.push_back(CPU_JMP);
                ret.push_back((a&0xFF00)>>8);
                ret.push_back(a&0xFF);
            } else {
                a = decodeint(insn.at(1),ptr+1,0xFFFFFF,true, true);
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
            uint a = decodeint(insn.at(1), ptr, 0xFFFFFF, true);

            ret.push_back((a&0xFF0000)>>16);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_DS: {
            if (insn.size() < 2)
                ierror0("invalid instruction format",insnraw);
            if (insnraw.length() <= 3)
                ierror0("invalid instruction format",insnraw);

            std::string str(insnraw.c_str()+3);

            int len = ((str.length()%3)>1?((str.length()/3)+1):(str.length()/3) + (str.length()%3)) * 3;
            char sd[len];
            memset(sd, 0, len);
            memcpy(sd, str.c_str(), str.length());
            for (int i=0;i<len;i++)
                ret.push_back(sd[i]);

            break;
        }
        case INSN_DP: {
            if (insn.size() < 2)
                ierror0("invalid instruction format",insnraw);
            if (insnraw.length() <= 3)
                ierror0("invalid instruction format",insnraw);

            std::string str(insnraw.c_str()+3);

            int len = ((str.length()%3)>1?((str.length()/3)+1):(str.length()/3) + (str.length()%3)) * 3;
            char sd[len];
            memset(sd, 0, len);
            memcpy(sd, str.c_str(), str.length());
            for (int i=0;i<len;i++) {
                ret.push_back(0);
                ret.push_back(0);
                ret.push_back(sd[i]);
            }
            ret.push_back(0);
            ret.push_back(0);
            ret.push_back(0);

            break;
        }
        case INSN_LDA: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_LDA);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_LDB: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_LDB);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        /*case INSN_LDAI: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_LDAI);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }*/
        case INSN_LDAI: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,false);
            if (a <= 0xFFFF) {
                a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);
                ret.push_back(CPU_LDAI);
                ret.push_back((a&0xFF00)>>8);
                ret.push_back(a&0xFF);
            } else {
                a = decodeint(insn.at(1),ptr+1,0xFFFFFF,true, true);
                ret.push_back(CPU_ELDAI);
                ret.push_back(0);
                ret.push_back(0);

                ret.push_back((a&0xFF0000)>>16);
                ret.push_back((a&0xFF00)>>8);
                ret.push_back(a&0xFF);
            }

            break;
        }

        case INSN_LDBI: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_LDBI);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_OTA: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,false);
            if (a <= 0xFFFF) {
                a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);
                ret.push_back(CPU_OTA);
                ret.push_back((a&0xFF00)>>8);
                ret.push_back(a&0xFF);
            } else {
                a = decodeint(insn.at(1),ptr+1,0xFFFFFF,true, true);
                ret.push_back(CPU_EOTA);
                ret.push_back(0);
                ret.push_back(0);

                ret.push_back((a&0xFF0000)>>16);
                ret.push_back((a&0xFF00)>>8);
                ret.push_back(a&0xFF);
            }

            break;

        }
        case INSN_OTB: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_OTB);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_LDAPTR: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_LDAPTR);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            uint sp = ptr+2;
            ret.push_back((sp&0xFF0000)>>16);
            ret.push_back((sp&0xFF00)>>8);
            ret.push_back(sp&0xFF);

            ret.push_back(0);
            ret.push_back(0);
            ret.push_back(0);

            break;
        }
        case INSN_OTAPTR: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_OTAPTR);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            uint sp = ptr+2;
            ret.push_back((sp&0xFF0000)>>16);
            ret.push_back((sp&0xFF00)>>8);
            ret.push_back(sp&0xFF);

            ret.push_back(0);
            ret.push_back(0);
            ret.push_back(0);

            break;
        }
        case INSN_DEC: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_DEC);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_BRCH: {
            if (insn.size() != 3)
                ierror0("invalid instruction format",insnraw);

            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_BRCH);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            uint addr = decodeint(insn.at(2), ptr + 1, 0xFFFFFF, true, true);
            ret.push_back((addr & 0xFF0000) >> 16);
            ret.push_back((addr & 0xFF00) >> 8);
            ret.push_back(addr & 0xFF);

            break;
        }
        case INSN_ELDA: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr+1,0xFFFFFF,true, true);

            ret.push_back(CPU_ELDA);
            ret.push_back(0);
            ret.push_back(0);

            ret.push_back((a&0xFF0000)>>16);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_ELDB: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr+1,0xFFFFFF,true, true);

            ret.push_back(CPU_ELDB);
            ret.push_back(0);
            ret.push_back(0);

            ret.push_back((a&0xFF0000)>>16);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_IRET: {
            if (insn.size() != 1)
                ierror0("invalid instruction format",insnraw);

            ret.push_back(CPU_IRET);
            ret.push_back(0);
            ret.push_back(0);

            break;
        }
        case INSN_CMPBH: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr+2,0xFFFFFF,true, true);

            ret.push_back(CPU_CMPBH);
            ret.push_back(0);
            ret.push_back(0);

            ret.push_back(0);
            ret.push_back(0);
            ret.push_back(2);

            ret.push_back((a&0xFF0000)>>16);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_HCF: {
            if (insn.size() != 1)
                ierror0("invalid instruction format",insnraw);

            ret.push_back(CPU_HCF);
            ret.push_back(0);
            ret.push_back(0);

            break;
        }
        case INSN_IE: {
            if (insn.size() != 1)
                ierror0("invalid instruction format",insnraw);

            ret.push_back(CPU_IE);
            ret.push_back(0);
            ret.push_back(0);

            break;
        }
        case INSN_SMM: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_SMM);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_CALL: {
            if (insn.size() != 3)
                ierror0("invalid instruction format",insnraw);

            uint a = decodeint(insn.at(1),ptr+2,0x00FFFF,true, true);
            uint b = decodeint(insn.at(2),ptr+1,0xFFFFFF,true, true);

            uint after_ptr = ptr+3;
            ret.push_back(CPU_LDB);
            ret.push_back((after_ptr&0xFF00)>>8);
            ret.push_back(after_ptr&0xFF);

            ret.push_back(CPU_OTB);
            ret.push_back((b&0xFF00)>>8);
            ret.push_back(b&0xFF);

            ret.push_back(CPU_JMP);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            break;
        }
        case INSN_IJMP: {
            if (insn.size() != 2)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true, true);

            ret.push_back(CPU_IJMP);
            ret.push_back((a&0xFF00)>>8);
            ret.push_back(a&0xFF);

            ret.push_back(((ptr+2)&0xFF0000)>>16);
            ret.push_back(((ptr+2)&0xFF00)>>8);
            ret.push_back((ptr+2)&0xFF);

            ret.push_back(0);
            ret.push_back(0);
            ret.push_back(0);

            break;
        }
        default: {
            printf("Unknown itype %d\n",itype);
            exit(1);
        }
    }

    ptr += (ret.size()/3);

    return ret;
}

void assemble_resolve_final(std::vector<unsigned int>* outdata) {
#ifdef ENABLE_LABEL_ENGINE
    lelog("resolving all remaining labels...\n");

    lelog("labels: \n")
    for (auto lbl : labelptrs)
        lelog(" => %s\n",lbl.name);

    for (auto llk=labelqueue.begin(); llk!=labelqueue.end();) {

        label* llkb = getlabel(std::string(llk->name));
        if (!llkb) {
            lelog("error: unknown label \"%s\" on final pass\n",llk->name);
            exit(-1);
        }

        lelog("applying final lookup [ptr=0x%X, mask=0x%06X, np=0x%06X]\n",llk->ptr,llk->mask,llkb->ptr);
        (*outdata)[llk->ptr] = outdata->at(llk->ptr) | (llkb->ptr & llk->mask);

        labelqueue.erase(llk);
    }

    lelog("all labels resolved.\n");
#endif
}