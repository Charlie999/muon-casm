#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include "asm.h"
#include "insns.h"

#define uchar unsigned char
#define uint unsigned int

bool uep = false;
bool udbg = false;

unsigned char *ucra = nullptr;

#define log(args...) if (!uep) {printf(args);}
#define alog(args...) {printf(args);}
#define dlog(args...) if(udbg && !uep) {printf(args);}

uint memory[16777216];

uint PC = 0;

uchar smm = 0;

uchar psw;

void setpswcarry() {
    psw |= 0b1;
}
void setpswequals() {
    psw |= 0b10;
}

uint load(uint addr) {
    dlog("LOAD @0x%06X [0x%06X]\n",addr, memory[addr]);
    return memory[addr];
}

void store(uint addr, uint data) {
    if (addr >= 0xFFFFFF) ierror0("Invalid memory write", "[EMULATOR=>store()]");

    dlog("STORE @0x%06X [0x%06X]\n",addr, data);
    if (addr >= 0xFFFE00) {
        uint ucromaddr = addr-0xFFFE00;
        ucra[ucromaddr] = (data&0xFF);
        printf("WCS WRITE 0x%03X [0x%02X]\n",ucromaddr,ucra[ucromaddr]);
        return;
    } else if (addr == 0xF00000) {
        uchar a = (data&0xFF0000)>>16;
        uchar b = (data&0xFF00)>>8;
        uchar c = (data&0xFF);
        printf("TERMINAL_WRITE: %c%c%c\n",a,b,c);
        return;
    }
    memory[addr] = data;
}

uchar hlt;

uint A,B;

uint iar = 0;

void _do_74181_logical(uchar sel, uint idr, uchar la);
void _do_74181_arithmetic(uchar sel, uint idr, uchar la);

void emulate(const std::vector<std::string>& rmem, unsigned char* ucrom, bool dbg, bool ep) {
    udbg = dbg;
    uep = ep;
    ucra = ucrom;
    printf("constructing memory...\n");

    int i = 0;
    for (i=0;i<rmem.size();i++) {
        uint t = std::stoul(rmem.at(i), nullptr, 16);
        dlog("memory @loc=%d : %06X\n",i,t);
        memory[i] = t;
    }
    dlog("populated words 0x%06X - 0x%06X\n",0,i-1);

    while (hlt == 0) {
        uint idr = load(PC);

        uchar opc = (idr & 0xFF0000) >> 16;
        uchar ucr[16];

        uchar la = 0;

        uint at = 0;
        uchar pt = 0;

        uchar pswcf = 0;

        memcpy(ucr, ucrom+(opc*16), 16);

        for (i=0;i<16;i++) {
            dlog("ucop %02X\n",ucr[i]);

            switch (ucr[i]&0xF) {
                case UC_NOP:
                    exit(0);
                    break;
                case UC_AW:
                    if (la) A = idr;
                    else A = idr & 0xFFFF;
                    break;
                case UC_BW:
                    if (la) B = idr;
                    else B = idr & 0xFFFF;
                    break;
                case UC_SMM:
                    smm = idr & 0xF;
                    break;
                case UC_LA:
                    la = 1;
                    PC += 1;
                    idr = load(PC);
                    break;
                case UC_AWI:
                    if (la) at = idr;
                    else at = idr & 0xFFFF;
                    A = load(at);
                    break;
                case UC_BWI:
                    if (la) at = idr;
                    else at = idr & 0xFFFF;
                    B = load(at);
                    break;
                case UC_PSWC:
                    pt = psw&(idr & 0xFF);
                    if (pt!=0) pswcf = 1;
                    break;
                case UC_PCW:
                    PC = iar;
                    break;
                case UC_BCHK:
                    if (la) at = idr;
                    else at = idr & 0xFFFF;
                    if (pswcf==1) PC = at;
                    break;
                case UC_ALU:
                    _do_74181_arithmetic((ucr[i]&0xF0)>>4,idr, la);
                    break;
                case UC_ALUL:
                    _do_74181_logical((ucr[i]&0xF0)>>4,idr, la);
                    break;
                case UC_IE:
                    ierror0("Emulator cannot enable interrupts!\n","[EMULATOR]");
                    break;
                case UC_END:
                    goto ucloopend;
                default:
                    printf("unknown ucode op: %02X\n",ucr[i]);
                    ierror0("Ucode operation not implemented\n","[EMULATOR]");
                    break;
            }

            if (i==15 && (ucr[i]&0xF) == 0) {
                log("prevented infinite ucode loop, emulator exiting.\n");
                exit(0);
            }
        }

        ucloopend:

        PC++;
    }
}

void _do_74181_logical(uchar sel, uint idr, uchar la) {
    uint sp = 0;
    if (la) sp = idr;
    else sp = idr & 0xFFFF;

    switch (sel & 0xF) {
        case 0:
            A = ~A;
            break;
        case 1:
            A = ~(A | B);
            break;
        case 2:
            A = (~A) & B;
            break;
        case 3:
            A = 0;
            break;
        case 4:
            A = ~(A & B);
            break;
        case 5:
            A = ~B;
            break;
        case 6:
            A = A^B;
            break;
        case 7:
            A = A & (~B);
            break;
        case 8:
            A = (~A) | B;
            break;
        case 9:
            A = ~(A ^ B);
            break;
        case 10:
            A = B;
            break;
        case 11:
            A = A&B;
            break;
        case 12:
            A = 0xFFFFFF;
            break;
        case 13:
            A = A | (~B);
            break;
        case 14:
            A = A | B;
            break;
        case 15:
            A = A;
            break;
    }

    store(sp, A);
}


void _do_74181_arithmetic(uchar sel, uint idr, uchar la) {
    uint sp = 0;
    if (la) sp = idr;
    else sp = idr & 0xFFFF;

    switch (sel & 0xF) {
        case 0:
            A = A;
            break;
        case 1:
            A = A + B;
            break;
        case 2:
            A = A + (~B);
            break;
        case 3:
            A = -1;
            break;
        case 4:
            A = A + (A & (~B));
            break;
        case 5:
            A = A + B + (A & (~B));
            break;
        case 6:
            A = A - (B + 1);
            break;
        case 7:
            A = (A & (~B)) - 1;
            break;
        case 8:
            A = A + (A&B);
            break;
        case 9:
            A = A + B;
            break;
        case 10:
            A = (A + (~B)) + (A&B);
            break;
        case 11:
            A = (A&B) - 1;
            break;
        case 12:
            A = A + A;
            break;
        case 13:
            A = (A|B) + A;
            break;
        case 14:
            A = (A|(~B)) + A;
            break;
        case 15:
            A = A-1;
            break;
    }

    if (A>0x7FFFFF) {
        setpswcarry();
        A = 0x7FFFFF;
    }

    if (A==B)
        setpswequals();

    store(sp, A);
}