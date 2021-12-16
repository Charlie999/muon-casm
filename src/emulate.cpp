#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include "asm.h"
#include "insns.h"

struct i24{
    unsigned int d : 24;
};

#define I(A) A.d
#define uchar unsigned char
#define uint unsigned int


uint PC = 0;
uchar pcc = 0;

i24 A;
i24 B;

bool uep = false;

void regdump() {
    printf("[PC=0x%06X,A=0x%06X,B=0x%06X] ",PC,I(A),I(B));
}

#define log(args...) if (!uep) {printf(args);}
#define alog(args...) {printf(args);}
#define dlog(args...) if(udbg && !uep) {regdump();printf(args);}

i24 I24(uint a) {
    i24 ret{};
    ret.d = a&0xFFFFFF;
    return ret;
}

i24 memory[16777216];

void write24(i24 a, i24 d) {
    if (I(a) == 0xFF0000) {
        uchar a_ = (I(d)&0xFF0000)>>16;
        uchar b_ = (I(d)&0xFF00)>>8;
        uchar c_ = (I(d)&0xFF);

        if (!uep) {regdump();log("VTERM PRINT: %c%c%c\n",a_,b_,c_);}
        else printf("%c%c%c",a_,b_,c_);
    } else if (I(a) == 0xFF0001) {
        regdump();
        log("VTERM IPRINT: 0x%06X\n",I(d));
    } else {
        memory[I(a)].d = I(d);
    }
}

i24 read24(i24 a) {
    if (I(a) == 0xFFFFFE)
        return I24(0xf0); // tells the OS that this is a VM
    return memory[I(a)];
}

struct PSW_t {
    uchar hlt;
};

PSW_t psw;
bool udbg = false;

void emulate(const std::vector<std::string>& rmem, bool dbg, bool ep) {
    udbg = dbg;
    uep = ep;
    log("constructing memory...\n");

    int i = 0;
    for (i=0;i<rmem.size();i++) {
        uint t = std::stoul(rmem.at(i), nullptr, 16);
        dlog("memory @loc=%d : %06X\n",i,t);
        memory[i] = I24(t);
    }
    dlog("populated words 0x%06X - 0x%06X\n",0,i-1);

    while (psw.hlt == 0) {
        i24 v = read24(I24(PC));

        uchar opc = (I(v)&0xFF0000)>>16;
        uchar b = (I(v)&0xFF00)>>8;
        uchar c = (I(v)&0xFF);

        switch (opc) {
            case VM_HALT: {
                dlog("VM HALT\n");
                psw.hlt = 1;
                break;
            }
            case CPU_NOP: {
                dlog("NOP\n");
                break;
            }
            case CPU_JMP: {
                uint addr = (b<<8) | c;
                dlog("JMP  \tto 0x%06X\n",addr);
                PC = addr;
                pcc = 1;
                break;
            }
            case CPU_MOV: {
                PC++;
                uint addr = (b<<8) | c;
                i24 o = read24(I24(PC)); //memory[PC];
                PC--; // for logging
                dlog("MOV  \tfrom 0x%06X to 0x%06X\n",addr, I(o));
                PC++;
                write24(o, read24(I24(addr))); //memory[I(o)] = memory[addr];
                break;
            }
            case CPU_EJMP: {
                PC++;
                i24 o = read24(I24(PC)); //memory[PC];
                PC--; // for logging
                dlog("EJMP \tto 0x%06X\n",I(o));
                PC = I(o);
                pcc = 1;
                break;
            }
            case CPU_IADD: {
                uint aa = (b<<8) | c;
                i24 ba = read24(I24(++PC));
                i24 ca = read24(I24(++PC));
                PC-=2;
                uint av = I(read24(I24(aa))); //I(memory[aa]);
                uint bv = I(read24(ba)); //I(memory[I(ba)]);
                A=I24(av);
                B=I24(bv);
                dlog("IADD \tA=0x%06X[loc=0x%06X] B=0x%06X[loc=0x%06X] C=0x%06X[loc=0x%06X]\n",av,aa,bv,I(ba),av+bv,I(ca));
                PC+=2;
                write24(ca, I24((av+bv)&0xFFFFFF)); //memory[I(ca)] = I24((av+bv)&0xFFFFFF);
                break;
            }
            case CPU_INA: {
                uint sa = (b<<8) | c;

                write24(I24(sa), A);
                A = I24(I(A) + 1);

                dlog("INA  \tSA=0x%06X\n",sa);

                break;
            }
            case CPU_INB: {
                uint sa = (b<<8) | c;

                write24(I24(sa), A);
                B = I24(I(B) + 1);

                dlog("INB  \tSA=0x%06X\n",sa);

                break;
            }
            default: {
                regdump();
                alog("illegal instruction: 0x%06X\n",I(v));
                exit(1);
            }
        }

        //std::this_thread::sleep_for(std::chrono::milliseconds(250));

        if (!pcc) PC++;
        else pcc=0;
    }
}