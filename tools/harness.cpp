#include "common.h"
#include "io/stegano.h"
#include "io/loader.h"
#include "vm/machine.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <cstring>
namespace r8 = retro8;
r8::Machine *machine;
uint32_t Platform::getTicks() { return 0; }

int main(int argc, char **argv) {
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    machine = new r8::Machine();
    machine->font().load();
    machine->code().loadAPI();
    int frames = argc > 2 ? atoi(argv[2]) : 0;
    if (data.size() > 4 && std::memcmp(data.data(), "\x89PNG", 4) == 0) {
        std::vector<uint8_t> out;
        unsigned long w, h;
        if (Platform::loadPNG(out, w, h, data.data(), data.size(), true) != 0) { printf("PNGFAIL\n"); return 2; }
        uint32_t *buf = new uint32_t[out.size() / 4];
        for (size_t i = 0; i < out.size() / 4; i++)
            buf[i] = out[4 * i] | (out[4 * i + 1] << 8) | (out[4 * i + 2] << 16) | (out[4 * i + 3] << 24);
        r8::io::PngData png = {buf, NULL, out.size() / 4};
        r8::io::Stegano s;
        s.load(png, *machine);
    } else {
        std::string raw((const char *) data.data(), data.size());
        r8::io::Loader l;
        l.loadRaw(raw, *machine);
    }
    if (frames > 0) {
        if (machine->code().hasInit()) machine->code().init();
        for (int i = 0; i < frames; i++) {
            if (machine->code().hasUpdate()) machine->code().update();
            if (machine->code().hasDraw()) machine->code().draw();
        }
    }
    printf("DONE\n");
    return 0;
}
