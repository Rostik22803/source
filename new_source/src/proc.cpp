#include "proc.h"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/syscall.h>

#ifndef __NR_process_vm_readv
#define __NR_process_vm_readv  270
#define __NR_process_vm_writev 271
#endif

pid_t    g_pid = 0;
uint64_t g_il2cppBase = 0;

pid_t proc_find_pid(const char *package) {
    DIR *dir = opendir("/proc");
    if (!dir) return 0;
    pid_t found = 0;
    struct dirent *e;
    while ((e = readdir(dir)) != nullptr) {
        int id = atoi(e->d_name);
        if (id <= 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", id);
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        char cmd[256] = {0};
        size_t n = fread(cmd, 1, sizeof(cmd) - 1, f);
        fclose(f);
        if (n == 0) continue;
        // cmdline разделён нулями; первый аргумент = пакет.
        if (strcmp(cmd, package) == 0) { found = id; break; }
    }
    closedir(dir);
    return found;
}

uint64_t proc_module_base(const char *module) {
    if (g_pid <= 0) return 0;
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", (int)g_pid);
    FILE *maps = fopen(path, "r");
    if (!maps) return 0;

    // 1) найти исполняемый сегмент модуля -> по его file offset вычислить базу.
    uint64_t xpStart = 0, xpOff = 0;
    char line[1024];
    while (fgets(line, sizeof(line), maps)) {
        if (!strstr(line, module)) continue;
        if (!strstr(line, "r-xp")) continue;
        uint64_t s = 0, en = 0, off = 0;
        if (sscanf(line, "%lx-%lx %*s %lx", &s, &en, &off) >= 2) {
            xpStart = s; xpOff = off;
            break;
        }
    }

    if (!xpStart) {
        // fallback — самый нижний маппинг модуля.
        rewind(maps);
        uint64_t lowest = 0;
        while (fgets(line, sizeof(line), maps)) {
            if (!strstr(line, module)) continue;
            uint64_t s = 0, en = 0;
            if (sscanf(line, "%lx-%lx", &s, &en) == 2 && s)
                if (!lowest || s < lowest) lowest = s;
        }
        fclose(maps);
        return lowest;
    }

    uint64_t candidate = xpStart - xpOff;
    rewind(maps);

    // 2) offset-0 маппинг, ближайший к candidate (на случай левых mmap того же файла).
    uint64_t best = 0, bestDiff = ~0ULL;
    while (fgets(line, sizeof(line), maps)) {
        if (!strstr(line, module)) continue;
        uint64_t s = 0, en = 0, off = 0;
        if (sscanf(line, "%lx-%lx %*s %lx", &s, &en, &off) < 2) continue;
        if (off != 0) continue;
        uint64_t diff = s > candidate ? s - candidate : candidate - s;
        if (diff < bestDiff) { bestDiff = diff; best = s; }
    }
    fclose(maps);

    if (!best || bestDiff > 0x1000000ULL) return candidate;
    return best;
}

static bool pvm(uint64_t addr, void *buf, size_t size, bool write) {
    if (g_pid <= 0 || addr < 0x10000 || size == 0) return false;
    struct iovec local[1], remote[1];
    local[0].iov_base = buf;
    local[0].iov_len  = size;
    remote[0].iov_base = (void *)addr;
    remote[0].iov_len  = size;
    long n = syscall(write ? __NR_process_vm_writev : __NR_process_vm_readv,
                     (long)g_pid, local, 1, remote, 1, 0ul);
    return n == (long)size;
}

bool proc_read(uint64_t addr, void *buf, size_t size)  { return pvm(addr, buf, size, false); }
bool proc_write(uint64_t addr, const void *buf, size_t size) {
    return pvm(addr, const_cast<void *>(buf), size, true);
}

std::string rd_string(uint64_t strObj, size_t maxLen) {
    if (!valid_ptr(strObj)) return std::string();
    int32_t len = rd<int32_t>(strObj + 0x10);
    if (len <= 0 || len > (int32_t)maxLen) return std::string();
    // UTF-16 -> UTF-8 (упрощённо: BMP, latin/кириллица).
    char16_t utf16[128];
    if (len > 127) len = 127;
    if (!proc_read(strObj + 0x14, utf16, sizeof(char16_t) * len)) return std::string();
    std::string out;
    out.reserve(len);
    for (int i = 0; i < len; i++) {
        char16_t c = utf16[i];
        if (c < 0x80) {
            out += (char)c;
        } else if (c < 0x800) {
            out += (char)(0xC0 | (c >> 6));
            out += (char)(0x80 | (c & 0x3F));
        } else {
            out += (char)(0xE0 | (c >> 12));
            out += (char)(0x80 | ((c >> 6) & 0x3F));
            out += (char)(0x80 | (c & 0x3F));
        }
    }
    return out;
}
