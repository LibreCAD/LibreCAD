// Thin CLI over jsondump::dumpCadFile (all logic lives in the shared header).
//   libdxfrw_json_dump <in.dwg|in.dxf> [-o out.json]
#include <iostream>
#include <fstream>
#include <string>

#include "libdxfrw_json_dump.h"

int main(int argc, char* argv[]) {
    std::string in;
    std::string outPath;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) outPath = argv[++i];
        else if (in.empty())          in = a;
    }
    if (in.empty()) {
        std::cerr << "usage: libdxfrw_json_dump <in.dwg|in.dxf> [-o out.json]\n";
        return 2;
    }
    if (!outPath.empty()) {
        std::ofstream ofs(outPath);
        if (!ofs) { std::cerr << "cannot open output: " << outPath << "\n"; return 2; }
        return jsondump::dumpCadFile(in, ofs);
    }
    return jsondump::dumpCadFile(in, std::cout);
}
