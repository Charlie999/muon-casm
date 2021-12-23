#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include <fstream>
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

uint memory[16777215];

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
    uchar smmr = (smm&0xF)<<17;
    addr |= smmr;

    dlog("LOAD @0x%06X [0x%06X]\n",addr, memory[addr]);
    if (addr >= 0xFFF800) {
        uint ucromaddr = addr-0xFFF800;
        return ucra[ucromaddr];
    }
    return memory[addr];
}

void store(uint addr, uint data) {
    uchar smmr = (smm&0xF)<<17;
    addr |= smmr;

    if (addr >= 0xFFFFFF) ierror0("Invalid memory write", "[EMULATOR=>store()]");

    dlog("STORE @0x%06X [0x%06X]\n",addr, data);
    if (addr >= 0xFFF800) {
        uint ucromaddr = addr-0xFFF800;
        ucra[ucromaddr] = (data&0xFF);
        printf("WCS WRITE 0x%03X [0x%02X]\n",ucromaddr,ucra[ucromaddr]);
        return;
    } else if (addr == 0xF00000) {
        uchar a = (data&0xFF0000)>>16;
        uchar b = (data&0xFF00)>>8;
        uchar c = (data&0xFF);
        if (uep)
            printf("%c%c%c",a,b,c);
        else
            printf("TERMINAL_WRITE: %c%c%c\n",a,b,c);
        return;
    } else if (addr == 0xF00001) {
        printf("TERMINAL_WRITE_INT: 0x%06X\n",data);
        return;
    }
    memory[addr] = data;
}

uchar hlt;

uint A,B;

uint iar = 0;

std::string df;

void emufinish(int code) {
    if (df.length() > 0) {
        printf("Writing memory dump to %s\n",df.c_str());
        udbg = false;
        auto *memblob = (unsigned char*)malloc(16777215 * 3);
        for (int i=0;i<16777215;i++) { // slow, I know, but needed for the WCS etc
            uint data = load(i);
            uchar a = (data&0xFF0000)>>16;
            uchar b = (data&0xFF00)>>8;
            uchar c = (data&0xFF);
            memblob[(i*3)] = a;
            memblob[(i*3)+1] = b;
            memblob[(i*3)+2] = c;
        }

        std::ofstream dumpfile(df, std::ios::out | std::ios::binary);
        if (!dumpfile.write(reinterpret_cast<const char *>(memblob), 1677215 * 3)) {
            printf("Error writing dump!\n");
        }
        dumpfile.close();
        printf("Written %d bytes.\n",1677215*3);
    }
    exit(code);
}

unsigned int _do_74181_logical(uchar sel, uint idr, uchar la);
unsigned int _do_74181_arithmetic(uchar sel, uint idr, uchar la);

void emulate(const std::vector<std::string>& rmem, unsigned char* ucrom, const std::string& dumpfile, int expectediters, bool dbg, bool ep) {
    udbg = dbg;
    uep = ep;
    ucra = ucrom;
    df = dumpfile;
    printf("constructing memory...\n");

    int i = 0;
    for (i=0;i<rmem.size();i++) {
        uint t = std::stoul(rmem.at(i), nullptr, 16);
        dlog("memory @loc=%d : %06X\n",i,t);
        memory[i] = t;
    }
    dlog("populated words 0x%06X - 0x%06X\n",0,i-1);

    uint iters = 0;

    while (hlt == 0) {
        uint idr = load(PC);

        uchar opc = (idr & 0xFF0000) >> 16;
        uchar ucr[16];

        uchar la = 0;

        uint at = 0;
        uchar pt = 0;
        uchar jmp = 0;

        uchar pswcf = 0;

        memcpy(ucr, ucrom+(opc*16), 16);

        if (opc == 0xFF) {
            dlog("hcf end\n");
            emufinish(0);
        }

        for (i=0;i<16;i++) {
            //dlog("ucop %02X\n",ucr[i]);

            switch (ucr[i]&0xF) {
                case UC_NOP:
                    dlog("uc nop\n");
                    break;
                case UC_LD:
                    if (la) at = idr;
                    else at = idr & 0xFFFF;
                    PC = at;
                    dlog("uc ld set pc=0x%06X\n",PC);
                    jmp=1;
                    break;
                case UC_AW:
                    if (la) A = idr;
                    else A = idr & 0xFFFF;
                    dlog("uc aw set a=0x%06X\n",A)
                    break;
                case UC_BW:
                    if (la) B = idr;
                    else B = idr & 0xFFFF;
                    dlog("uc bw set b=0x%06X\n",B)
                    break;
                case UC_SMM:
                    smm = idr & 0xF;
                    dlog("uc smm set smm=0x%01X\n",smm&0xF)
                    break;
                case UC_LA:
                    la = 1;
                    PC += 1;
                    idr = load(PC);
                    dlog("uc la set idr=0x%06X\n",idr)
                    break;
                case UC_AWI:
                    if (la) at = idr;
                    else at = idr & 0xFFFF;
                    A = load(at);
                    dlog("uc awi set a=0x%06X [@0x%06X]\n",A,at)
                    break;
                case UC_BWI:
                    if (la) at = idr;
                    else at = idr & 0xFFFF;
                    B = load(at);
                    dlog("uc bwi set b=0x%06X\n",B)
                    break;
                case UC_PSWC:
                    pt = psw&(idr & 0xFF);
                    if (pt!=0) pswcf = 1;
                    dlog("uc pswc psw=0x%02X set pswcf=%d\n",psw,pswcf);
                    break;
                case UC_PCW:
                    PC = iar;
                    jmp=1;
                    dlog("uc pcw set pc=0x%06X\n",PC);
                    break;
                case UC_BCHK:
                    if (la) at = idr;
                    else at = idr & 0xFFFF;
                    if (pswcf==1) {
                        PC = at;
                        jmp = 1;
                        dlog("uc bchk b=%d set pc=0x%06X\n", pswcf, PC);
                    } else {
                        jmp = 0;
                        dlog("uc bchk b=%d no branch\n",pswcf);
                    }
                    pswcf=0;
                    break;
                case UC_ALU:
                    psw &= 0b11111100;
                    at = _do_74181_arithmetic((ucr[i]&0xF0)>>4,idr, la);
                    dlog("uc alu opc=%d A=0x%06X B=0x%06X set r=0x%06X\n",(ucr[i]&0xF0)>>4,A,B,at);
                    break;
                case UC_ALUL:
                    psw &= 0b11111100;
                    at = _do_74181_logical((ucr[i]&0xF0)>>4,idr, la);
                    dlog("uc alul opc=%d A=0x%06X B=0x%06X  set r=0x%06X\n",(ucr[i]&0xF0)>>4,A,B,at);
                    break;
                case UC_IE:
                    ierror0("Emulator cannot enable interrupts!\n","[EMULATOR]");
                    break;
                case UC_END:
                    dlog("uc end\n");
                    goto ucloopend;
                default:
                    printf("unknown ucode op: %02X\n",ucr[i]);
                    ierror0("Ucode operation not implemented\n","[EMULATOR]");
                    break;
            }

            if (i==15 && (ucr[i]&0xF) == 0) {
                log("prevented infinite ucode loop, emulator exiting.\n");
                emufinish(-1);
            }
        }

        ucloopend:

        if (!jmp) PC++;

        iters++;
        if (iters > expectediters && expectediters > 0) {
            log("exceeded expected iters, emulator exiting.\n");
            emufinish(-1);
        }
    }
}

uint _do_74181_logical(uchar sel, uint idr, uchar la) {
    uint sp = 0;
    if (la) sp = idr;
    else sp = idr & 0xFFFF;

    uint res = 0;

    switch (sel & 0xF) {
        case 0:
            res = ~A;
            break;
        case 1:
            res = ~(A | B);
            break;
        case 2:
            res = (~A) & B;
            break;
        case 3:
            res = 0;
            break;
        case 4:
            res = ~(A & B);
            break;
        case 5:
            res = ~B;
            break;
        case 6:
            res = A^B;
            break;
        case 7:
            res = A & (~B);
            break;
        case 8:
            res = (~A) | B;
            break;
        case 9:
            res = ~(A ^ B);
            break;
        case 10:
            res = B;
            break;
        case 11:
            res = A&B;
            break;
        case 12:
            res = 0xFFFFFF;
            break;
        case 13:
            res = A | (~B);
            break;
        case 14:
            res = A | B;
            break;
        case 15:
            res = A;
            break;
    }

    if (A==B)
        setpswequals();

    store(sp, res);
    return res;
}


uint _do_74181_arithmetic(uchar sel, uint idr, uchar la) {
    uint sp = 0;
    if (la) sp = idr;
    else sp = idr & 0xFFFF;

    uint res = 0;

    switch (sel & 0xF) {
        case 0:
            res = A;
            break;
        case 1:
            res = A + B;
            break;
        case 2:
            res = A + (~B);
            break;
        case 3:
            res = -1;
            break;
        case 4:
            res = A + (A & (~B));
            break;
        case 5:
            res = A + B + (A & (~B));
            break;
        case 6:
            res = A - (B + 1);
            break;
        case 7:
            res = (A & (~B)) - 1;
            break;
        case 8:
            res = A + (A&B);
            break;
        case 9:
            res = A + B;
            break;
        case 10:
            res = (A + (~B)) + (A&B);
            break;
        case 11:
            res = (A&B) - 1;
            break;
        case 12:
            res = A + A;
            break;
        case 13:
            res = (A|B) + A;
            break;
        case 14:
            res = (A|(~B)) + A;
            break;
        case 15:
            res = A-1;
            break;
    }

    if (res>0x7FFFFF) {
        setpswcarry();
        res = 0x7FFFFF;
    }

    if (A==B)
        setpswequals();

    store(sp, res);
    return res;
}