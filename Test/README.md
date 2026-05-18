# Unit tests (CuTest)

Tests unitarios del Memory Manager basados en el framework
[CuTest](https://github.com/alejoaquili/c-unit-testing-example), adaptados a la
interfaz real de nuestro kernel (`mm_init` / `mm_alloc` / `mm_free` /
`mm_get_status` de `Kernel/include/memoryManager.h`).

Corren en el **host**, fuera del kernel, para detectar errores temprano. Son
distintos de la carpeta `MemoryTest/` (que prueba el SO ya corriendo desde
Userland).

## Uso

```sh
cd Test
make run        # prueba el memory manager por defecto (memoryManager.c)
make buddy      # compila para el buddy allocator y...
./MemoryManagerTest.out   # ...lo corre
make valgrind   # corre bajo Valgrind (busca memory leaks)
make clean
```

Para probar el buddy en un solo paso: `make buddy && ./MemoryManagerTest.out`.

El binario sale con código distinto de cero si algún test falla.

## Casos cubiertos

- `testInitialStatus` — el estado tras `mm_init` es coherente.
- `testAllocMemory` — `mm_alloc` devuelve memoria.
- `testTwoAllocations` — dos reservas no se solapan.
- `testWriteMemory` — la memoria reservada es escribible/legible.
- `testFreeUpdatesStatus` — `mm_free` actualiza contadores y `used_bytes`.
- `testAllocAfterFree` — la memoria liberada se puede reutilizar.
- `testAllocZeroFails` — `mm_alloc(0)` falla y cuenta el fallo.
- `testOversizedAllocFails` — reservar más que el heap falla y cuenta el fallo.
- `testFreeNullIsIgnored` — `mm_free(NULL)` no rompe ni cuenta un free.
- `testFreeInvalidPointerIsIgnored` — liberar un puntero fuera del heap se ignora.
- `testDoubleFreeIsIgnored` — liberar dos veces el mismo bloque cuenta un solo free.
- `testFreedBlockIsReused` — tras liberar un bloque del medio, se reutiliza esa dirección.
- `testHeapCanBeExhaustedAndReused` — agotar el heap falla limpiamente y, tras liberar todo, se vuelve a poder reservar.
- `testCoalescingReclaimsContiguousSpace` — tras agotar y liberar todo, una reserva grande solo entra si los bloques se fusionaron (coalescing).
- `testAllocationsDoNotCorruptEachOther` — varios bloques con patrones distintos no se pisan entre sí (detecta solapamiento).

## Agregar más tests

1. Escribir la función `void testX(CuTest *const cuTest)` en `MemoryManagerTest.c`.
2. Agregarla al array `MemoryManagerTests[]` e incrementar `TestQuantity`.
