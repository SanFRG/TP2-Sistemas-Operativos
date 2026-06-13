/* Syscalls del memory manager: alloc, free y consulta de estado.
 * La syscall_table (syscall_dispatcher.c) las referencia por su declaracion
 * en el header. */
#include <syscall_dispatcher.h>
#include <memoryManager.h>

uint64_t sys_mem_alloc(uint64_t size) {
    return (uint64_t)mm_alloc(size);
}

uint64_t sys_mem_free(uint64_t ptr) {
    mm_free((void *)ptr);
    return 0;
}

uint64_t sys_mem_status(uint64_t status_ptr) {
    if (status_ptr == 0) {
        return (uint64_t)-1;
    }

    mm_get_status((mm_status_t *)status_ptr);
    return 0;
}
