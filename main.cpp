#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "cxxopts/cxxopts.hpp"
#include "asm.h"

std::string infile;
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

enum modes {
    COMPILE,
    EMULATE
};

modes mode = COMPILE;

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
            ("d,debug", "emulator debug mode")
            ("p,emuprint", "transparent terminal printing from emulator (disables normal emulator logging)")
            ("i,input", "set input file", cxxopts::value<std::string>());

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
    else
        mode = COMPILE;

    if (mode == COMPILE) {
        infile.clear();
        infile.append(argsresult["input"].as<std::string>());

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
                if (iline.length() <= 3 || iline.c_str()[0] == ';') {
                    errs++;
                    indata.erase(indata.begin() + i);
                }
            }

            if (errs == 0) san = 0;
        }

        std::vector<unsigned int> out;
        for (auto &i : indata) {
            std::vector<unsigned char> t = assemble(i);
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

        printf("v2.0 raw\n");
        for (unsigned int i : out) {
            printf("%06x\n", i);
        }

        return 0;
    } else if (mode == EMULATE) {
        infile.clear();
        infile.append(argsresult["input"].as<std::string>());

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

        emulate(indata, argsresult.count("debug"), argsresult.count("emuprint"));
    }
}
