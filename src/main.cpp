#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "../cxxopts/cxxopts.hpp"
#include "asm.h"

std::string infile;
std::string ofile;
std::vector<std::string> indata;
std::vector<unsigned char> outbuf;

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
            ("ucodesplit","split ucode into lower and upper 2K (this file is the upper 2K)", cxxopts::value<std::string>());

    auto argsresult = options.parse(argc, argv);

    if (argsresult.count("help")) {
        std::cout << options.help();
        return 0;
    }

    if (!argsresult.count("input")) {
        std::cout << options.help();
        return 0;
    }

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
                if (iline.length() < 2 || iline.c_str()[0] == ';') {
                    errs++;
                    indata.erase(indata.begin() + i);
                }
            }

            if (errs == 0) san = 0;
        }

        bool movswap = argsresult.count("help")!=0;

        std::vector<unsigned int> out;
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

        if (!argsresult.count("ucrom")) {
            printf("--ucrom required for emulation!\n");
            std::cout << options.help();
            return 0;
        }

        std::string ucfile = argsresult["ucrom"].as<std::string>();

        if (!exists(infile)) {
            printf("file doesn't exist: %s\n", infile.c_str());
            exit(1);
        }

        printf("Reading input file %s\n", infile.c_str());

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

        printf("Reading microcode ROM %s\n",ucfile.c_str());
        auto *ucrom = (unsigned char*)malloc(4096);
        std::ifstream ucs (ucfile, std::ios::in | std::ios::binary);
        if (!ucs.read(reinterpret_cast<char *>(ucrom), 4096)) {
            printf("error reading ucode rom from file\n");
            exit(1);
        }
        ucs.close();

        std::string cdf;
        if (argsresult.count("coredump"))
            cdf = argsresult["coredump"].as<std::string>();
        int maxiters = 0;
        if (argsresult.count("expectediters"))
            maxiters = argsresult["expectediters"].as<int>();
        emulate(indata, ucrom, cdf, maxiters, argsresult.count("debug"), argsresult.count("emuprint"));

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
