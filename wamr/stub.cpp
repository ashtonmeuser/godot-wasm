#include <cstdlib>


#include "bh_log.h"

extern "C" {

static uint32 log_verbose_level = BH_LOG_LEVEL_FATAL;

void
bh_log_set_verbose_level(uint32 level)
{
    log_verbose_level = level;
}

void
bh_log(LogLevel log_level, const char *file, int line, const char *fmt, ...)
{
    va_list ap;

    if ((uint32)log_level > log_verbose_level)
        return;

    if (file)
        os_printf("%s, line %d, ", file, line);

    va_start(ap, fmt);
    os_vprintf(fmt, ap);
    va_end(ap);

    os_printf("\n");
}

void * os_malloc(unsigned size)
{
    return std::malloc(size);
}
 
void * os_realloc(void *ptr, unsigned size)
{
    return std::realloc(ptr, size);
}
 
void os_free(void *ptr)
{
    std::free(ptr);
}
 
int os_dumps_proc_mem_info(char *out, unsigned int size)
{
    return -1;
}

void * os_mmap(void *hint, size_t size, int prot, int flags, os_file_handle file)
{
    void *addr;

    if (size >= UINT32_MAX)
        return NULL;

    if ((addr = BH_MALLOC((uint32)size)))
        memset(addr, 0, (uint32)size);

    return addr;
}

void * os_mremap(void *old_addr, size_t old_size, size_t new_size)
{
    return os_mremap_slow(old_addr, old_size, new_size);
}

void os_munmap(void *addr, size_t size)
{
    return BH_FREE(addr);
}

int os_mprotect(void *addr, size_t size, int prot)
{
    return 0;
}


int bh_platform_init()
{
    return 0;
}

void bh_platform_destroy()
{}

int os_printf(const char *format, ...)
{
    int ret = 0;
    va_list ap;

    va_start(ap, format);
#ifndef BH_VPRINTF
    ret += vprintf(format, ap);
#else
    ret += BH_VPRINTF(format, ap);
#endif
    va_end(ap);

    return ret;
}

int os_vprintf(const char *format, va_list ap)
{
#ifndef BH_VPRINTF
    return vprintf(format, ap);
#else
    return BH_VPRINTF(format, ap);
#endif
}

}