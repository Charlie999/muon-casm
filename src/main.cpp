#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#ifdef __linux__
#include <uv.h>
#endif
#include <unistd.h>
#include "../cxxopts/cxxopts.hpp"
#include "asm.h"

std::string ofile;

std::vector<unsigned char> outbuf;

#define UCODE_URL "/muon-3/ucode.bin"
#define UCODE_URL_HOST "storage.googleapis.com"

inline bool exists(const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}

static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

enum pmodes {
    COMPILE,
    EMULATE,
    UCODE
};
enum omodes {
    HEX,
    BINARY
};

pmodes mode = COMPILE;
omodes omode = HEX;

int getdoublenlptr(const char *buf);

int gethttpcl(char buf[10240]);

void processincludes(const std::string& iname, std::vector<std::string> &indata);
void preprocessfile(std::vector<std::string> &indata);
void readfile(const std::string& infile, std::vector<std::string> &indata);

int main(int argc, char** argv) {

    printf("CASM MUON-III assembler/emulator (C) Charlie 2021\n");

    //po::options_description desc("Options");
    cxxopts::Options options("casm", "MUON-III assembler/emlator");

    /*desc.add_options()
            ("help", "show help message")
            ("emulate", "set mode to emulate")
            ("debug", "emulator debug mode")
            ("emuprint", "transparent terminal printing from emulator (disables normal emulator logging)")
            ("input", po::value<std::string>(), "set input file");*/

    options.add_options()
            ("h,help", "show help message")
            ("e,emulate", "set mode to emulate")
            ("u,ucode","set mode to ucode compile")
            ("d,debug", "emulator debug mode")
            ("p,emuprint", "transparent terminal printing from emulator (disables normal emulator logging)")
            ("i,input", "set input file", cxxopts::value<std::string>())
            ("o,output", "set output file", cxxopts::value<std::string>())
            ("binary","output ref in binary format")
            ("movswap","swap mov operands")
            ("ucrom","ucode ROM for emulation", cxxopts::value<std::string>())
            ("coredump","(optional) save a memory dump from the emulator upon exit", cxxopts::value<std::string>())
            ("expectediters","stop the emulator (with an error) to prevent infinite loops", cxxopts::value<int>())
            ("controlflow","dump the control flow for a program in the emulator")
            ("fetchucode","fetch the latest microcode ROM from Jenkins")
            ("ucodesplit","split ucode into lower and upper 2K (this file is the upper 2K)", cxxopts::value<std::string>())
            ("gotin","import GOT file for use in code", cxxopts::value<std::string>())
            ("gotout","output exported GOT file", cxxopts::value<std::string>())
            ("org","address that zero will be translated to in the assembler", cxxopts::value<int>())
            ("dumpfixedinclude","dump the include-processed source file");

    auto argsresult = options.parse(argc, argv);

    if (argsresult.count("help")) {
        std::cout << options.help();
        return 0;
    }

    if (!argsresult.count("input")) {
        std::cout << options.help();
        return 0;
    }
    std::vector<std::string> indata;
    std::string infile;

    if (argsresult.count("emulate"))
        mode = EMULATE;
    else if (argsresult.count("ucode"))
        mode = UCODE;
    else
        mode = COMPILE;

    if (argsresult.count("binary"))
        omode = BINARY;
    else
        omode = HEX;

    if (mode == COMPILE) {
        infile.clear();
        infile.append(argsresult["input"].as<std::string>());

        if (argsresult.count("output")) {
            ofile.clear();
            ofile.append(argsresult["output"].as<std::string>());
        }

        readfile(infile, indata);
        preprocessfile(indata);
        processincludes(infile,indata);

        if (argsresult.count("dumpfixedinclude")) {
         for (auto &l : indata) {
          printf("[dumpfixedinclude]: %s\n",l.c_str());
         }
        }

        std::vector<gotentry> gotin;
        if (argsresult.count("gotin")) {
            std::vector<std::string> gotlines;
            readfile(argsresult["gotin"].as<std::string>(), gotlines);
            // GOT file format:
            // [<fptr>]<fname>
            // e.g.
            // [00C000]examplefunction
            for (const auto& l : gotlines) {
                uint gotptr = 0;
                char gotname[256];
                int r = sscanf(l.c_str(), "[%X]%[^\n]s", &gotptr, gotname);
                if (r != 2) {
                    printf("[gotengine] error: invalid got line %s\n",l.c_str());
                    exit(1);
                }
                printf("[gotengine] imported GOT entry %s [addr=0x%06X]\n",gotname,gotptr);
                gotentry ge;
                ge.ptr = gotptr;
                strncpy(ge.fname, gotname, 255);
                gotin.push_back(ge);
            }
        }
        addgotentries(gotin);

        bool movswap = argsresult.count("help")!=0;

        std::vector<unsigned int> out;

        if (argsresult.count("org"))
            assembler_org(argsresult["org"].as<int>(), &out);

        for (auto &i : indata) {
            std::vector<unsigned char> t = assemble(i, movswap, &out);
            int q = 0;
            for (unsigned char &j : t) {
                if ((q % 3) == 0) {
                    unsigned int tt = 0;
                    unsigned char a = t.at(q);
                    unsigned char b = t.at(q + 1);
                    unsigned char c = t.at(q + 2);
                    tt |= a << 16;
                    tt |= b << 8;
                    tt |= c;
                    out.push_back(tt);
                }
                q++;
            }
        }
        assemble_resolve_final(&out);

        if (argsresult.count("gotout")) {
            std::string gn = argsresult["gotout"].as<std::string>();
            printf("[gotengine] saving exported GOT as %s\n",gn.c_str());
            std::ofstream wf(gn);
            char buf[300];
            for (auto ge : getgotentries()) {
                snprintf(buf, 299, "[%06X]%s\n",ge.ptr,ge.fname);
                wf.write(buf,strlen(buf));
            }
            wf.close();
            printf("[gotengine] saved GOT\n");
        }

        if (argsresult.count("org")) {
            int org = argsresult["org"].as<int>();
	    int os = out.size()-org;
            out.erase(out.begin(), out.begin() + org);
 	    out.resize(os);
 	    out.shrink_to_fit();
        }

        if (omode == HEX && ofile.length() == 0) {
            printf("v2.0 raw\n");
            for (unsigned int i : out) {
                printf("%06x\n", i);
            }
        } else if (omode == HEX) {
            std::ofstream wf(ofile, std::ios::out);
            char tmp[10];
            snprintf(tmp,10,"v2.0 raw\n");
            wf.write(tmp,strlen(tmp));
            for (unsigned int i : out) {
                snprintf(tmp,8,"%06X\n",i);
                wf.write(tmp,strlen(tmp));
            }
            wf.close();
            printf("Written to %s\n",ofile.c_str());
        } else if (omode == BINARY) {
            std::ofstream wf(ofile, std::ios::out | std::ios::binary);
            for (unsigned int i : out) {
                unsigned char a = (i&0xFF0000)>>16;
                unsigned char b = (i&0xFF00)>>8;
                unsigned char c = (i&0xFF);
                wf.write(reinterpret_cast<const char *>(&a), 1);
                wf.write(reinterpret_cast<const char *>(&b), 1);
                wf.write(reinterpret_cast<const char *>(&c), 1);
            }
            wf.close();
            printf("Written to %s\n",ofile.c_str());
        }

        return 0;
    } else if (mode == EMULATE) {
        infile.clear();
        infile.append(argsresult["input"].as<std::string>());

        if (!argsresult.count("ucrom") && !argsresult.count("fetchucode")) {
            printf("--ucrom required for emulation!\n");
            std::cout << options.help();
            return 0;
        }

        std::string ucfile;
        if (argsresult.count("ucrom"))
            ucfile = argsresult["ucrom"].as<std::string>();

        if (!exists(infile)) {
            printf("file doesn't exist: %s\n", infile.c_str());
            exit(1);
        }

        printf("Reading input file %s\n", infile.c_str());


        if (omode == BINARY) {
            std::ifstream inf(infile.c_str(),std::ios::binary);
            unsigned char m[3];
            if (!indata.empty())
                indata.erase(indata.begin(), indata.end());
            long c = 0;
            while (!inf.fail() && !inf.eof()) {
                inf.read((char*)m, 3);
                unsigned int assembled = (m[0] << 16) | (m[1] << 8) | m[2];
                setemulatormem(c, assembled);
                c++;
            }
            inf.close();
            printf("read %ld words\n",c);
        } else {
            std::ifstream inf(infile.c_str());
            std::string line;
            while (std::getline(inf, line)) {
                std::istringstream iss(line);
                if (line.length() <= 1)
                    continue;
                if (line.c_str()[0] == 'v')
                    continue;
                indata.push_back(line);
            }
            inf.close();
        }

        auto *ucrom = (unsigned char *) malloc(4096);
        memset(ucrom, 0, 4096);

        if (argsresult.count("fetchucode")) {
#ifdef __linux__
            printf("WARNING: --fetchucode is just a development convenience! Using this is not recommended.\n");
            printf("fetching ucode from http://%s%s\n",UCODE_URL_HOST,UCODE_URL);
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                printf("socket creation failed\n");
                exit(-1);
            }
            struct sockaddr_in serv{};
            serv.sin_family = AF_INET;
            serv.sin_port = htons(80);
            const char* addr;
            addrinfo hints, *res;
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            if (getaddrinfo(UCODE_URL_HOST, nullptr, &hints, &res) != 0) {
                printf("cannot resolve %s\n",UCODE_URL_HOST);
                exit(-1);
            }
            addr = inet_ntoa(((struct sockaddr_in*)res->ai_addr)->sin_addr);
            if (inet_pton(AF_INET, addr, &serv.sin_addr)<=0) {
                printf("invalid address\n");
                exit(-1);
            }
            if (connect(sockfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
                printf("connection to %s:80 failed\n",inet_ntoa(serv.sin_addr));
                exit(-1);
            }
            printf("connected to %s:80\n",inet_ntoa(serv.sin_addr));
            char buf[10240];
            int plen = snprintf(buf,10240,"GET %s HTTP/1.0\nHost: %s\nUser-Agent: casm/1\n\n",UCODE_URL,UCODE_URL_HOST);
            send(sockfd, buf, plen, 0);
            int ptr = 0;
            plen = read(sockfd, buf, 10240);
            if (plen<0) {
                printf("read error [%d]\n",plen);
                close(sockfd);
                exit(-1);
            }
            ptr += plen;
            if (strncmp(buf, "HTTP/1.0 200 OK", 15) != 0) {
                printf("HTTP error: [%s]\n",buf);
                close(sockfd);
                exit(-1);
            }
            while (plen > 0) {
                plen = read(sockfd, buf+ptr, 10240-ptr);
                if (plen < 0) {
                    printf("read error [%d]\n",plen);
                    close(sockfd);
                    exit(-1);
                }
                ptr += plen;
            }
            close(sockfd);
            int cl = gethttpcl(buf);
            if (cl<=0) {
                printf("invalid content length\n");
            }
            auto* ucs = (unsigned char*)(buf + getdoublenlptr(buf));
            if (cl != 4096 && cl != 2048) {
                printf("invalid uc rom length [%d]\n",cl);
                exit(-1);
            }
            memcpy(ucrom, ucs, cl);
            printf("ucode rom i0 bytes: %02X %02X %02X %02X %02X %02X %02X %02X\n",ucrom[0],ucrom[1],ucrom[2],ucrom[3],ucrom[4],ucrom[5],ucrom[6],ucrom[7]);
            printf("ucode rom i1 bytes: %02X %02X %02X %02X %02X %02X %02X %02X\n",ucrom[16],ucrom[17],ucrom[18],ucrom[19],ucrom[20],ucrom[21],ucrom[22],ucrom[23]);
            printf("fetched %d bytes into ucode rom\n",cl);
#else
            printf("error: feature only supported on Linux\n");
            exit(-1);
#endif

        } else {
            printf("Reading microcode ROM %s\n", ucfile.c_str());
            std::ifstream ucs(ucfile, std::ios::in | std::ios::binary);
            if (!ucs.read(reinterpret_cast<char *>(ucrom), 4096)) {
                printf("error reading ucode rom from file\n");
                exit(1);
            }
            ucs.close();
        }

        std::string cdf;
        if (argsresult.count("coredump"))
            cdf = argsresult["coredump"].as<std::string>();
        int maxiters = 0;
        if (argsresult.count("expectediters"))
            maxiters = argsresult["expectediters"].as<int>();
        emulate(indata, ucrom, cdf, maxiters, argsresult.count("debug"), argsresult.count("emuprint"), argsresult.count("controlflow"));

        return 0;
    } else if (mode == UCODE) {
        infile.clear();
        infile.append(argsresult["input"].as<std::string>());

        if (argsresult.count("output")) {
            ofile.clear();
            ofile.append(argsresult["output"].as<std::string>());
        } else {
            printf("Must specify output file for ucode mode\n");
            std::cout << options.help();
            exit(0);
        }

        if (!exists(infile)) {
            printf("file doesn't exist: %s\n", infile.c_str());
            exit(1);
        }

        printf("Reading input file %s\n", infile.c_str());

        std::ifstream inf(infile.c_str());

        std::string line;
        while (std::getline(inf, line)) {
            std::istringstream iss(line);
            indata.push_back(line);
        }

        int san = 1;
        while (san) {
            int errs = 0;

            for (int i = 0; i < indata.size(); i++) {
                std::string iline = trim(indata.at(i));
                if (iline.length() <= 1 || iline.c_str()[0] == ';') {
                    errs++;
                    indata.erase(indata.begin() + i);
                }
            }

            if (errs == 0) san = 0;
        }

        std::vector<unsigned char> out = ucassemble(indata);
        std::string splitfile;

        bool split = argsresult.count("ucodesplit");
        if (split) {
            splitfile.clear();
            splitfile.append(argsresult["ucodesplit"].as<std::string>());
        }

        if (omode == HEX) {
            std::ofstream wf(ofile, std::ios::out);
            char tmp[10];
            snprintf(tmp,10,"v2.0 raw\n");
            wf.write(tmp,strlen(tmp));
            if (split) {
                for (int i=0;i<2048;i++) {
                    snprintf(tmp, 8, "%02X\n", out.at(i));
                    wf.write(tmp, strlen(tmp));
                }
                std::ofstream sf(splitfile, std::ios::out);
                snprintf(tmp,10,"v2.0 raw\n");
                sf.write(tmp,strlen(tmp));
                for (int i=2048;i<4096;i++) {
                    snprintf(tmp, 8, "%02X\n", out.at(i));
                    sf.write(tmp, strlen(tmp));
                }
                sf.close();
            } else {
                for (unsigned char i : out) {
                    snprintf(tmp, 8, "%02X\n", i);
                    wf.write(tmp, strlen(tmp));
                }
            }
            wf.close();
            if (split) printf("Written lower 2K to %s and upper 2K to %s\n",ofile.c_str(),splitfile.c_str());
            else printf("Written to %s\n",ofile.c_str());
        } else if (omode == BINARY) {
            std::ofstream wf(ofile, std::ios::out | std::ios::binary);
            for (unsigned char a : out) {
                wf.write(reinterpret_cast<const char *>(&a), 1);
            }
            wf.close();
            printf("Written to %s\n",ofile.c_str());
        }

        return 0;
    }
}

void readfile(const std::string& infile, std::vector<std::string> &indata) {
    printf("reading input file %s\n", infile.c_str());

    if (!exists(infile)) {
        printf("file doesn't exist: %s\n", infile.c_str());
        exit(1);
    }

    std::ifstream inf(infile.c_str());
    std::string line;
    while (std::getline(inf, line)) {
        std::istringstream iss(line);
        indata.push_back(line);
    }
    inf.close();
}

void preprocessfile(std::vector<std::string> &indata) {
    int san = 1;
    while (san) {
        int errs = 0;

        for (int i = 0; i < indata.size(); i++) {
            std::string iline = trim(indata.at(i));
            if (iline.length() < 2 || iline.c_str()[0] == ';') {
                errs++;
                indata.erase(indata.begin() + i);
            }
        }

        if (errs == 0) san = 0;
    }
}

void processincludes(const std::string& iname, std::vector<std::string> &indata) {
    for (int i=0; i < indata.size(); i++) {
        std::string iline = indata.at(i);
        if (strncmp(iline.c_str(), "$include ", 9) == 0) {
            std::string include = std::string(iline.c_str()+9);
            if (strcmp(include.c_str(),iname.c_str()) == 0) {
                printf("[includes] error: file cannot include itself! [%s => %s]\n",iname.c_str(),include.c_str());
                exit(EXIT_FAILURE);
            }
            printf("[includes] processing include %s => %s\n",iname.c_str(),include.c_str());

            std::vector<std::string> filedata;
            readfile(include, filedata);
            preprocessfile(filedata);
            processincludes(include, filedata);

            indata.insert(indata.begin() + i + 1, ";;;; END INCLUDE (from "+include+")");
            for (unsigned long j = filedata.size(); j>0; j--) {
                std::string l = filedata.at(j-1);
                indata.insert(indata.begin() + i + 1, l);
            }

            indata.at(i) = ";;;; INCLUDED FROM " + include;
        }
    }
}

int gethttpcl(char buf[10240]) {
    for (const auto& line : split(std::string(buf),"\r\n")) {
        if (memcmp(line.c_str(),"Content-Length:",15) == 0) {
            return strtol(line.c_str() + 15,nullptr,10);
        }
    }
    return -1;
}

int getdoublenlptr(const char *buf) {
    for (int i=0;i<10237;i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n')
            return i+4;
    }
    return -1;
}
