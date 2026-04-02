// bsp2bin — Standalone CLI tool for converting D1 RPG .bsp to D2 RPG .bin.
// The core logic lives in src/converter/BspToBin.cpp.
//
// Build (standalone, no cmake):
//   g++ -std=c++17 -O2 -I src/converter tools/bsp2bin.cpp src/converter/BspToBin.cpp -o tools/bsp2bin

#include "BspToBin.h"
#include <cstdio>
#include <fstream>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: bsp2bin <input.bsp> [output.bin]\n");
        return 1;
    }

    std::ifstream ifs(argv[1], std::ios::binary);
    if (!ifs) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifs)), {});
    ifs.close();

    auto result = BspToBin::convert(data.data(), (int)data.size());

    // Output path
    std::string outPath;
    if (argc >= 3) {
        outPath = argv[2];
    } else {
        outPath = argv[1];
        auto dot = outPath.rfind('.');
        if (dot != std::string::npos) outPath = outPath.substr(0, dot);
        outPath += ".bin";
    }

    std::ofstream ofs(outPath, std::ios::binary);
    ofs.write((char*)result.binData.data(), result.binData.size());
    ofs.close();

    printf("  %-30s -> %s\n", argv[1], outPath.c_str());
    printf("    polys=%5d  lines=%4d  nodes=%4d  size=%zu -> %zu\n",
           result.numPolys, result.numLines, result.numNodes,
           data.size(), result.binData.size());

    return 0;
}
