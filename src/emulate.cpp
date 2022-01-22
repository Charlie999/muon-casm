#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include <fstream>
#include <stdio.h>
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#include <unistd.h>
#else
#include <winsock2.h>
#include <io.h>
#include <conio.h>
#define STDIN_FILENO _fileno(stdin)
#endif

#include "asm.h"
#include "insns.h"
#include "int24.h"


#define uchar unsigned char
#define uint unsigned int

bool uep = false;
bool udbg = false;

unsigned char *ucra = nullptr;

#define log(...) if (!uep) {printf(__VA_ARGS__);}
#define alog(...) {printf(__VA_ARGS__);}
#define dlog(...) if(udbg && !uep) {printf(__VA_ARGS__);}

uint memory[16777216];

void emufinish(int code);

uint PC = 0;

uchar smm = 0;

uchar psw;

bool kb_available() {
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  return (FD_ISSET(0, &fds));
}

void setpswcarry() {
    psw |= 0b1;
}
void setpswequals() {
    psw |= 0b10;
}

#define MAX_CONNECTED_SRAM 0x80000 // 512K of connected SRAM only.

char keyboard[2] = {0,0};

uint load(uint addr) {
    if (addr > 0xFFFFFF) {
	printf("error: cannot load outside memory! addr=0x%08X\n",addr);
	emufinish(1);
    }
	
    uchar smmr = (smm&0xF)<<17;
    if (addr < 0xF00000)
        addr |= smmr;

    if (addr < 0xF00000 && addr > MAX_CONNECTED_SRAM) {
        dlog("write to unconnected memory at 0x%06X\n",addr);
	return 0;
    }

    dlog("LOAD @0x%06X [0x%06X]\n",addr, memory[addr]);

    if (addr == 0xF00001) {
        return keyboard[0];
    } else if (addr == 0xF00002) {
        keyboard[0] = 0;
        return keyboard[1];
    }

    if (addr >= 0xFFF800) {
        uint ucromaddr = addr-0xFFF800;
        return ucra[ucromaddr];
    }
    return memory[addr];
}

void store(uint addr, uint data) {
    if (addr > 0xFFFFFF) {
	printf("error: cannot store outside memory! addr=0x%08X\n",addr);
	emufinish(1);
    }
    uchar smmr = (smm&0xF)<<17;
    if (addr < 0xF00000)
        addr |= smmr;

    if (addr >= 0xFFFFFF) ierror0("Invalid memory write", "[EMULATOR=>store()]");

    dlog("STORE @0x%06X [0x%06X]\n",addr, data);
    if (addr >= 0xFFF800) {
        uint ucromaddr = addr-0xFFF800;
        ucra[ucromaddr] = (data&0xFF);
        printf("WCS WRITE 0x%03X [0x%02X]\n",ucromaddr,ucra[ucromaddr]);
        return;
    } else if (addr == 0xF00000) {
        uchar c = (data&0xFF);
        if (uep)
            printf("%c",c);
        else
            printf("TERMINAL_WRITE: %c\n",c);
        return;
    } else if (addr == 0xF00001) {
        printf("TERMINAL_WRITE_INT: 0x%06X\n",data);
        return;
    } else if (addr == 0xF00002) {
        printf("TERMINAL_WRITE_NUM: %d\n",int24::fromuint(data).operator int());
        return;
    }
    memory[addr] = data&0xFFFFFF;
}

uchar hlt = 0;

int A=0,B=0;

uint iar = 0;

std::string df;

std::vector<std::pair<uint,uint>> controlflow;
bool ucf = false;

uint ops;
uint iters = 0;

long estart;
long eend;

void emufinish(int code) {
    eend = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    long etime = eend-estart;

    double opspersec = ((double)ops/(double)((double)etime/1000));
    double insnspersec = ((double)iters/(double)((double)etime/1000));
    double mhz = opspersec/1000000.0;

    printf("Emulator done after %d instructions/%d micro-ops, took %ld milliseconds at a rate of %f uops/s (%f insns/s)\n",iters,ops,etime,opspersec,insnspersec);
    printf("speed approx %fMHz\n",mhz);
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
    if (ucf) {
        printf("Writing control flow to ./controlflow.txt ...\n");
        std::ofstream dumpfile("controlflow.txt", std::ios::out);
        for (auto p : controlflow) {
            char line[256];
            int len = snprintf(line, 255, "%06X [%06X]\n",p.first,p.second);
            dumpfile.write(line,len);
        }
        dumpfile.close();
    }
    exit(code);
}

unsigned int _do_74181_logical(uchar sel, uint idr, uchar la);
unsigned int _do_74181_arithmetic(uchar sel, uint idr, uchar la);

unsigned int lim24(unsigned int res);

bool ie = false;
bool timerready = false;

void icheck() {
    if (!ie) return;

    if (ie && ops==262144) {
        dlog("timer interrupt\n");
        timerready = true;
    }

    uchar iid = 0;
    bool interrupt = false;

    if (timerready) {
        timerready = false;
        interrupt = true;
        iid = 4;
    }
    
    if (keyboard[0] && !interrupt) {
        interrupt = true;
        iid = 8;
    }

    if (interrupt) {
        dlog("interrupt id=%02X\n",iid);
        iar = PC;
        psw &= 0b11;
        psw |= (iid&0b111111)<<2;
        PC = 0;
        ie = false;
    }
}

void setemulatormem(long p,uint m) {
    if (p>16777216) {
        printf("cannot write to 0x%lX!\n",p);
        exit(1);
    }
    memory[p] = m%0xFFFFFF;
}

void emulate(const std::vector<std::string>& rmem, unsigned char* ucrom, const std::string& dumpfile, int expectediters, bool dbg, bool ep, bool cf) {
    udbg = dbg;
    uep = ep;
    ucra = ucrom;
    df = dumpfile;
    ucf = cf;
    printf("constructing memory...\n");

    int i = 0;

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
    if (!rmem.empty()) {
        for (i = 0; i < rmem.size(); i++) {
            uint t = std::stoul(rmem.at(i), nullptr, 16);
            //dlog("memory @loc=%d : %06X\n",i,t);
            memory[i] = t;
        }
        dlog("populated words 0x%06X - 0x%06X\n", 0, i - 1);
    }
#endif

    printf("starting emulation..\n");
    estart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    while (hlt == 0) {
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
        if (kb_available() && !keyboard[0]) {
            read(STDIN_FILENO, &keyboard[1], 1);
            keyboard[0] = 1;
	}
#else
	if (kbhit() && !keyboard[0]) {
            read(STDIN_FILENO, &keyboard[1], 1);
            keyboard[0] = 1;
	}   
#endif
        icheck();
        fetchstart:

        uint idr = load(PC);

        if (cf) {
            std::pair<uint,uint> p(PC, idr);
            controlflow.push_back(p);
        }

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

        if (opc == 0xFE) {
            uint pct = PC;
            dlog("vm interrupt wait [ie=%d,pct=0x%06X]...\n",ie,pct);
            while (pct == PC && ie) {
                icheck();
                ops++;
            }
            if (ie) goto fetchstart;
            goto ucloopend;
        }

        for (i=0;i<16;i++) {
            //dlog("ucop %02X\n",ucr[i]);
            ops++;

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
                    dlog("uc alu opc=%d A=0x%06X[%d] B=0x%06X[%d] set r=0x%06X\n",(ucr[i]&0xF0)>>4,int24::fromuint(A).limit(),int(int24::fromuint(A)),int24::fromuint(B).limit(),int(int24::fromuint(B)),at);
                    break;
                case UC_ALUL:
                    psw &= 0b11111100;
                    at = _do_74181_logical((ucr[i]&0xF0)>>4,idr, la);
                    dlog("uc alul opc=%d A=0x%06X[%d] B=0x%06X[%d]  set r=0x%06X\n",(ucr[i]&0xF0)>>4,A,A,B,B,at);
                    break;
                case UC_IE:
                    ie = true;
                    dlog("uc ie\n");
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

            if (ie && ops==262144) {
                dlog("timer interrupt\n");
                timerready = true;
            }
        }

        ucloopend:

        if (!jmp) PC++;

        ops++;

        if (ie && ops==262144) {
            dlog("timer interrupt\n");
            timerready = true;
        }

        iters++;
        if (iters > expectediters && expectediters > 0) {
            log("exceeded expected iters, emulator exiting.\n");
            emufinish(-1);
        }
    }
}

unsigned int lim24(unsigned int res) {
    return res&0xFFFFFF;
}

uint _do_74181_logical(uchar sel, uint idr, uchar la) {
    uint sp = 0;
    if (la) sp = idr;
    else sp = idr & 0xFFFF;

    uint res = 0;
    uint ra=A, rb=B;

    switch (sel & 0xF) {
        case 0:
            res = ~ra;
            break;
        case 1:
            res = ~(ra | rb);
            break;
        case 2:
            res = (~ra) & rb;
            break;
        case 3:
            res = 0;
            break;
        case 4:
            res = ~(ra & rb);
            break;
        case 5:
            res = ~rb;
            break;
        case 6:
            res = ra^rb;
            break;
        case 7:
            res = ra & (~rb);
            break;
        case 8:
            //res = (~ra) | rb;
            res = (ra>>1) | ((ra&1)<<23);
            break;
        case 9:
            res = ~(ra ^ rb);
            //printf("CMP 0x%06X==0x%06X  ==> 0x%06X\n",ra,rb,res);
            break;
        case 10:
            res = rb;
            break;
        case 11:
            res = ra&rb;
            break;
        case 12:
            res = 0xFFFFFF;
            break;
        case 13:
            res = ra | (~rb);
            break;
        case 14:
            res = ra | rb;
            break;
        case 15:
            res = ra;
            break;
    }

    if (lim24(res) == 0xFFFFFF)
        setpswequals();

    store(sp, res);
    return res;
}


uint _do_74181_arithmetic(uchar sel, uint idr, uchar la) {
    uint sp = 0;
    if (la) sp = idr;
    else sp = idr & 0xFFFF;

    int24 res = 0;
    int24 ra, rb;

    ra.m_Internal[2] = (A&0xFF0000)>>16;
    ra.m_Internal[1] = (A&0xFF00)>>8;
    ra.m_Internal[0] = (A&0xFF);

    rb.m_Internal[2] = (B&0xFF0000)>>16;
    rb.m_Internal[1] = (B&0xFF00)>>8;
    rb.m_Internal[0] = (B&0xFF);

    switch (sel & 0xF) {
        case 0:
            res = ra;
            break;
        case 1:
            res = ra + rb;
            break;
        case 2:
            res = ra + (~rb);
            break;
        case 3:
            res = -1;
            break;
        case 4:
            res = ra + (ra & (~rb));
            break;
        case 5:
            res = ra + rb + (ra & (~rb));
            break;
        case 6:
            res = ra - rb;
            res = res - int24(1);
            break;
        case 7:
            res = (ra & (~rb)) - 1;
            break;
        case 8:
            res = ra + (ra&rb);
            break;
        case 9:
            res = ra + rb;
            break;
        case 10:
            res = (ra + (~rb)) + (ra&rb);
            break;
        case 11:
            res = (ra&rb) - 1;
            break;
        case 12:
            res = ra + ra;
            break;
        case 13:
            res = (ra|rb) + ra;
            break;
        case 14:
            res = (ra|(~rb)) + ra;
            break;
        case 15:
            res = ra-1;
            break;
    }

    if (res>0x7FFFFF) {
        setpswcarry();
        res = 0x7FFFFF;
    }

    if (res.limit() == 0xFFFFFF)
        setpswequals();

    store(sp, res.limit());
    return res.limit();
}
