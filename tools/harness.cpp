#include "common.h"
#include "io/stegano.h"
#include "io/loader.h"
#include "vm/machine.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <cstring>
#include <csignal>
#include <chrono>
#include <unistd.h>
#include "lua/lua.hpp"
namespace r8 = retro8;
r8::Machine *machine;
uint32_t Platform::getTicks() { return 0; }

static volatile sig_atomic_t trace_now = 0;

static void trace_hook(lua_State *L, lua_Debug *) {
    if (!trace_now) return;
    luaL_traceback(L, L, "stuck here", 0);
    printf("%s\n", lua_tostring(L, -1));
    _exit(4);
}

#include <map>
#include <string>
#include <vector>
#include <algorithm>
static std::map<std::string, long> profile_hits;

static void profile_hook(lua_State *L, lua_Debug *ar) {
    lua_Debug info;
    if (!lua_getstack(L, 0, &info) || !lua_getinfo(L, "Sl", &info)) return;
    char key[96];
    snprintf(key, sizeof(key), "line %d (function at %d)", info.currentline, info.linedefined);
    profile_hits[key]++;
}

static void profile_report() {
    std::vector<std::pair<long, std::string>> rows;
    long total = 0;
    for (auto &hit : profile_hits) { rows.push_back({hit.second, hit.first}); total += hit.second; }
    std::sort(rows.rbegin(), rows.rend());
    for (size_t i = 0; i < rows.size() && i < 15; ++i)
        printf("PROFILE %5.1f%% %s\n", 100.0 * rows[i].first / (total ? total : 1), rows[i].second.c_str());
}

static void on_alarm(int) {
    trace_now = 1;
}

int main(int argc, char **argv) {
    if (getenv("P8_TRACE_AFTER")) {
        signal(SIGALRM, on_alarm);
        alarm(atoi(getenv("P8_TRACE_AFTER")));
    }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    machine = new r8::Machine();
    machine->font().load();
    machine->code().loadAPI();
    if (getenv("P8_SAVE_DIR")) lua::cartdataDirectory = getenv("P8_SAVE_DIR");
    if (getenv("P8_TRACE_AFTER")) lua_sethook(machine->code().mainState(), trace_hook, LUA_MASKCOUNT, 1000);
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
        if (!s.load(png, *machine)) { printf("Error on load\nunreadable or uncompilable cart\n"); return 3; }
    } else {
        std::string raw((const char *) data.data(), data.size());
        r8::io::Loader l;
        if (!l.loadRaw(raw, *machine)) { printf("Error on load\nuncompilable cart\n"); return 3; }
    }
    if (frames > 0) {
        if (getenv("P8_PROFILE") && machine->code().cartThread())
            lua_sethook(machine->code().cartThread(), profile_hook, LUA_MASKCOUNT, 1000);
        if (machine->code().hasInit()) machine->code().init();
        const auto started = std::chrono::steady_clock::now();
        for (int i = 0; i < frames; i++) {
            machine->code().frame();
            lua::cartdataFlush();
        }
        if (getenv("P8_TIME")) {
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
            printf("FRAME_MS %.3f\n", ms / frames);
        }
    }
    if (getenv("P8_PROFILE")) profile_report();
    printf("DONE\n");
    return 0;
}
