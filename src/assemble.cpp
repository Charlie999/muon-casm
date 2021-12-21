#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
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
    } else if (strcmp(opc, "ds")==0) {
        return INSN_DS;
    }  else {
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
    std::string s;
    if (a.length()>=2) {
#ifdef ENABLE_LABEL_ENGINE
        std::string t(a);
        t.erase(std::remove(t.begin(),t.end(),','), t.end());
        if (t.c_str()[0] == '{' && t.c_str()[t.length()-1] == '}' && enablelabels) {
            std::string lblname(t.c_str()+1);
            lblname.pop_back();
            lelog("get label %s\n",lblname.c_str());
            label *lbl = getlabel(lblname);
            if (!lbl) {
                lelog("label %s not found\n",lblname.c_str());
                if (!lookuplabels)
                    return 0xFFFFFFE0;
                lelog("label lookup queued for %s\n",lblname.c_str());
                labellookup llk;
                strncpy(llk.name,lblname.c_str(),256);
                llk.ptr = _ptr;
                llk.mask = imask;
                labelqueue.push_back(llk);
                return 0;
            }
            lelog("got label ptr for %s: 0x%06X\n",lblname.c_str(),lbl->ptr);
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
            if (insn.size() != 4)
                ierror0("invalid instruction format",insnraw);
            uint a = decodeint(insn.at(1),ptr,0x00FFFF,true);
            uint b = decodeint(insn.at(2),ptr+1, 0xFFFFFF,true);
            uint c = decodeint(insn.at(3),ptr+1, 0xFFFFFF,true);

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