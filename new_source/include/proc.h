#ifndef NEW_SOURCE_PROC_H
#define NEW_SOURCE_PROC_H

#include <cstdint>
#include <string>
#include <sys/types.h>

// Глобальное состояние процесса игры.
extern pid_t g_pid;
extern uint64_t g_il2cppBase;   // реальная база libil2cpp.so (ELF base)

// Найти pid процесса по имени пакета (скан /proc — это НЕ скан памяти,
// просто перебор каталогов /proc/<pid>/cmdline).
pid_t proc_find_pid(const char *package);

// База модуля в адресном пространстве игры (по /proc/pid/maps).
// Возвращает базу того кластера, где лежит r-xp сегмент (истинный ELF base).
uint64_t proc_module_base(const char *module);

// process_vm_readv / process_vm_writev обёртки.
bool proc_read(uint64_t addr, void *buf, size_t size);
bool proc_write(uint64_t addr, const void *buf, size_t size);

// Шаблоны чтения/записи.
template<typename T>
inline T rd(uint64_t addr) {
    T v{};
    if (addr < 0x10000) return v;
    proc_read(addr, &v, sizeof(T));
    return v;
}

template<typename T>
inline bool wr(uint64_t addr, const T &v) {
    if (addr < 0x10000) return false;
    return proc_write(addr, &v, sizeof(T));
}

// Чтение managed-строки il2cpp (Il2CppString: length@0x10, utf16 chars@0x14).
std::string rd_string(uint64_t strObj, size_t maxLen = 64);

// Грубая валидация указателя в куче игры.
inline bool valid_ptr(uint64_t p) {
    return p > 0x10000000ULL && p < 0x8000000000ULL;
}

#endif // NEW_SOURCE_PROC_H
