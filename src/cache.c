#include "cache.h"
#include <unistd.h>

#ifdef _WIN32
// use GlobalMemoryStatusEx on windows
#else 
MemoryInfo get_memory_info() {
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (MemoryInfo) {
        .total_memory = sysconf(_SC_PHYS_PAGES) * page_size,
        .page_size = page_size,
        .available_memory = sysconf(_SC_AVPHYS_PAGES) * page_size
    };
}
#endif
