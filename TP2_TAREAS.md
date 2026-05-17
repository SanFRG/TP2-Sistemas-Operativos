# TP2 Sistemas Operativos - Roadmap de Tareas

Este archivo resume que hay que construir para el TP2, en que orden conviene hacerlo y que puntos son importantes en cada etapa.

## 0. Base del Proyecto

### Objetivo
Dejar el proyecto en una base estable antes de agregar memoria, procesos, scheduler, semaforos y pipes.

### Tareas
- Verificar que `make` compile completo dentro del entorno pedido por la catedra.
- Mantener consistentes los numeros de syscalls entre:
  - `Kernel/include/syscall_dispatcher.h`
  - `Kernel/syscall_dispatcher.c`
  - `Userland/UserModule/include/lib.h`
  - `Userland/UserModule/asm/libasm.asm`
- Separar claramente dos conceptos:
  - `SYS_*`: numeros de syscall.
  - `fd`: file descriptors (`0 = stdin`, `1 = stdout`, `2 = stderr`).
- Evitar agregar syscalls duplicadas si una syscall existente puede resolverlo con otro parametro. Ejemplo: no hace falta `SYS_ERROR`; alcanza con `write(2, buffer, len)`.

### Importante
- Cada syscall nueva debe agregarse en kernel, wrapper ASM y header de userland.
- La posicion en `syscall_table` debe coincidir exactamente con el numero `SYS_*`.
- El retorno de syscalls debe quedar en `rax`.
- Los argumentos llegan por registros siguiendo la convencion actual: `rdi`, `rsi`, `rdx`, `rcx`, `r8`, etc.

## 1. Memory Manager Simple

### Objetivo
Implementar reserva y liberacion de memoria fisica/dinamica para que el kernel y los procesos puedan pedir memoria.

### Tareas
- Crear una interfaz comun de memory manager, por ejemplo:
  - `mm_init(start, size)`
  - `mm_alloc(size)`
  - `mm_free(ptr)`
  - `mm_status(status)`
- Implementar primero un memory manager simple con lista de bloques libres.
- Agregar metadata por bloque:
  - tamanio del bloque
  - si esta libre u ocupado
  - puntero al siguiente bloque
- Implementar `alloc` con first-fit o best-fit.
- Implementar split de bloques cuando sobra espacio.
- Implementar `free` marcando bloques como libres.
- Fusionar bloques libres contiguos para evitar fragmentacion excesiva.
- Agregar syscalls:
 TODO: - reservar memoria  
  - liberar memoria   
  - consultar estado de memoria
- Agregar wrappers en userland:
  - `malloc`
  - `free`
  - `mem_status`
- Implementar comando `mem`.



### Importante
- Definir de donde sale el heap del kernel: inicio y tamanio disponible.
- Alinear los bloques, idealmente a 8 o 16 bytes.
- Validar `NULL`, tamanios cero y punteros invalidos.
- `free` no deberia romper la lista si se llama con un puntero erroneo.
- Este modulo es base para procesos, stacks, semaforos y pipes.

## 2. Buddy System

### Objetivo
Implementar el segundo memory manager obligatorio: buddy system.

### Tareas
- Definir ordenes de bloques: potencias de 2.
- Mantener listas libres por orden.
- En `alloc`, redondear el pedido al tamanio buddy mas chico posible.
- Si no hay bloque disponible de ese orden, dividir uno mas grande.
- En `free`, buscar el buddy y fusionar recursivamente si tambien esta libre.
- Permitir elegir el memory manager en tiempo de compilacion.
  - Ejemplo: `make` para el manager simple.
  - Ejemplo: `make buddy` para buddy system.

### Importante
- Ambos managers deben compartir la misma interfaz.
- El resto del kernel no deberia saber cual manager esta usando.
- Al menos uno de los dos debe pasar `test_mm`, pero idealmente ambos.
- Cuidar bien los calculos de direcciones y alineacion.

## 3. Procesos

### Objetivo
Agregar procesos reales con PCB, PID, stack propio, estado y datos necesarios para scheduling.

### Tareas
- Definir una estructura `Process` o `PCB` con:
  - PID
  - nombre
  - estado (`READY`, `RUNNING`, `BLOCKED`, `KILLED`, etc.)
  - prioridad
  - stack base
  - stack pointer
  - base pointer
  - foreground/background
  - parent PID
  - lista o contador de hijos
  - file descriptors
- Implementar creacion de procesos.
- Soportar pasaje de argumentos.
- Implementar finalizacion normal del proceso.
- Implementar matar proceso por PID.
- Implementar listado de procesos para `ps`.
- Agregar syscalls:
  - crear proceso
  - terminar proceso
  - obtener PID actual
  - matar proceso
  - listar procesos
  - esperar hijos

### Importante
- Cada proceso necesita su propio stack.
- La memoria del stack deberia salir del memory manager.
- No mezclar "comando de shell" con "funcion llamada por la shell": los comandos del TP tienen que poder correr como procesos.
- Definir bien que pasa cuando muere un proceso foreground.
- Definir bien que pasa con procesos hijos si muere el padre.

## 4. Context Switching

### Objetivo
Poder suspender un proceso y continuar otro guardando/restaurando contexto.

### Tareas
- Guardar registros del proceso actual al entrar por timer interrupt.
- Restaurar registros del proximo proceso elegido.
- Integrar el cambio de contexto con IRQ0/timer.
- Implementar `yield` para que un proceso renuncie voluntariamente al CPU.

### Importante
- El context switch es una de las partes mas delicadas.
- Cualquier error de stack o registros puede colgar QEMU sin mensaje claro.
- Conviene empezar con dos procesos simples que impriman algo.
- Primero hacerlo sin prioridades; despues integrar prioridades.

## 5. Scheduler

### Objetivo
Implementar multitasking preemptivo con Round Robin con prioridades.

### Tareas
- Mantener cola(s) de procesos listos.
- Elegir el siguiente proceso segun prioridad.
- Hacer que procesos bloqueados no sean elegidos.
- Hacer que procesos muertos salgan del scheduling.
- Implementar cambio de prioridad (`nice`).
- Implementar bloqueo/desbloqueo (`block`).
- Implementar comando `loop`.
- Implementar comandos:
  - `ps`
  - `kill`
  - `nice`
  - `block`

### Importante
- El scheduler no debe elegir procesos bloqueados o muertos.
- Cuidar starvation: una prioridad alta no deberia hacer imposible correr a las demas, salvo que el algoritmo elegido lo justifique.
- `loop` debe hacer espera activa porque el enunciado lo pide asi.
- Los tests `test_proc` y `test_prio` dependen de esta etapa.

tener en cuenta:1. process_wait no bloquea — process.c:156
El código actual:


int process_wait(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) return -1;
    if (current_pid != -1 && p->parent_pid != current_pid) return -1;
    if (p->state != KILLED && p->state != TERMINATED) { // si el hijo sigue vivo retorno -1
        return -1;
    }
    // ... recién acá libera el stack y limpia el slot
}
El problema conceptual: wait (estilo waitpid de UNIX) significa "el padre se duerme hasta que el hijo termine". Acá, si el hijo todavía está vivo (READY/RUNNING/BLOCKED), la función simplemente devuelve -1 y vuelve enseguida.

Qué obliga a hacer eso del lado del que llama: el padre que quiere esperar a su hijo tiene que escribir algo así:


while (waitpid(hijo) == -1) {
    // el hijo todavía vive... reintento
}
Eso es busy-wait: el padre consume CPU al 100% chequeando una y otra vez, sin producir trabajo útil. El roadmap lo prohíbe explícitamente (etapa 3 "esperar hijos", y la regla general "no busy waiting").

Cómo debería ser: cuando el hijo sigue vivo, process_wait debe:

Poner al proceso padre (el que llama) en estado BLOCKED.
Anotar a qué PID está esperando (necesitás un campo en el PCB, p. ej. int waiting_for_pid;).
Ceder el CPU al scheduler (yield).
Cuando el hijo termina (en su exit/kill), el kernel debe buscar si algún padre estaba bloqueado esperándolo y pasarlo a READY.
O sea, wait solo recolecta (libera stack + limpia slot) cuando el hijo ya murió; si no, bloquea. Esto depende del scheduler, así que no se puede terminar hasta tener la etapa 4–5.

## 6. Semaforos

### Objetivo
Implementar semaforos compartibles entre procesos no relacionados.

### Tareas
- Definir estructura de semaforo:
  - identificador/nombre
  - valor
  - procesos esperando
  - cantidad de aperturas/referencias
- Implementar syscalls:
  - crear semaforo
  - abrir semaforo
  - cerrar semaforo
  - wait/down
  - post/up
- Bloquear procesos cuando hacen `wait` y no hay recursos.
- Desbloquear procesos esperando cuando otro proceso hace `post`.
- Usar alguna instruccion atomica o mecanismo seguro para evitar race conditions.
- Implementar/portar `test_sync`.

### Importante
- No debe haber busy waiting.
- No debe haber race conditions al modificar el valor del semaforo.
- No debe haber deadlocks causados por el propio sistema de semaforos.
- Los semaforos deben poder ser compartidos por procesos que no son padre/hijo si conocen el mismo identificador.

## 7. Pipes e IPC

### Objetivo
Implementar pipes unidireccionales bloqueantes y conectarlos con `read/write`.

### Tareas
- Definir estructura de pipe:
  - identificador/nombre
  - buffer circular
  - indice de lectura
  - indice de escritura
  - cantidad de bytes disponibles
  - procesos bloqueados leyendo
  - procesos bloqueados escribiendo
- Implementar syscalls:
  - crear pipe
  - abrir pipe
  - leer pipe
  - escribir pipe
  - cerrar pipe
- Integrar pipes con file descriptors de procesos.
- Hacer que `read(fd, ...)` y `write(fd, ...)` funcionen igual para terminal o pipe.
- Implementar comandos:
  - `cat`
  - `wc`
  - `filter`

### Importante
- Lectura bloqueante: si no hay datos, el proceso lector se bloquea.
- Escritura bloqueante: si el buffer esta lleno, el proceso escritor se bloquea.
- `write(1, ...)` deberia ir a stdout, que puede ser pantalla o pipe.
- `write(2, ...)` deberia ser stderr y normalmente ir a pantalla.
- Esto es necesario para que `p1 | p2` funcione bien.

## 8. Shell

### Objetivo
Convertir la shell en un lanzador real de procesos.

### Tareas
- Parsear comandos con argumentos.
- Ejecutar comandos como procesos.
- Soportar foreground y background.
- Soportar `&` al final para background.
- Soportar un pipe simple: `programa1 | programa2`.
- Soportar `Ctrl+C` para matar el proceso foreground.
- Soportar `Ctrl+D` como EOF.
- Implementar comando `help` con comandos normales y tests.

### Importante
- La shell no deberia bloquear todo el sistema si lanza un proceso en background.
- Si un proceso esta en foreground, la shell debe esperar a que termine o sea interrumpido.
- El enunciado no pide pipes encadenados tipo `p1 | p2 | p3`; alcanza con uno.
- El parser puede ser simple, pero debe manejar espacios y argumentos.

## 9. Aplicaciones y Tests de Userland

### Objetivo
Demostrar cada parte del TP con programas de usuario.

### Comandos obligatorios
- `sh`
- `help`
- `mem`
- `ps`
- `loop`
- `kill`
- `nice`
- `block`
- `cat`
- `wc`
- `filter`
- `mvar`

### Tests obligatorios
- `test_mm`
- `test_proc`
- `test_prio`
- `test_sync`

### Importante
- Los tests deben correr como procesos de usuario, no como built-ins del kernel.
- Deben poder correr en foreground y background.
- `test_mm` debe pasar al menos con uno de los memory managers.
- `test_sync` debe mostrar diferencia entre usar y no usar semaforos.

## 10. MVar

### Objetivo
Implementar el problema de multiples lectores y escritores sobre una variable global estilo MVar.

### Tareas
- Crear proceso principal `mvar`.
- Lanzar escritores y lectores segun parametros.
- Cada escritor espera activamente un tiempo aleatorio.
- Cada escritor espera a que la variable este vacia y escribe un valor unico.
- Cada lector espera activamente un tiempo aleatorio.
- Cada lector espera a que la variable tenga valor, lo consume e imprime.
- Sincronizar correctamente con semaforos.
- El proceso principal debe terminar inmediatamente despues de crear lectores y escritores.

### Importante
- Debe demostrar sincronizacion, prioridades y kills.
- Los ejemplos del enunciado son orientativos para validar comportamiento.
- No usar busy waiting para sincronizar la MVar; solo para las esperas aleatorias que simulan trabajo.

## 11. README y Entrega

### Objetivo
Preparar el repositorio para entrega y defensa.

### Tareas
- Completar README con:
  - instrucciones de compilacion
  - instrucciones de ejecucion
  - descripcion de cada comando
  - parametros de cada comando/test
  - uso de `&`
  - uso de `|`
  - atajos `Ctrl+C` y `Ctrl+D`
  - ejemplos de cada requerimiento
  - limitaciones
  - requerimientos faltantes o parciales
  - citas de codigo externo o uso de IA
- Limpiar warnings.
- Verificar que no haya binarios ni archivos generados versionados.
- Indicar rama y hash del commit de entrega.

### Importante
- La compilacion con `-Wall` no debe reportar warnings.
- Usar la imagen Docker indicada por la catedra.
- `make`, `make all` y `make <memory_manager>` deben ser solo para compilar.
- Otras tareas como correr QEMU o descargar imagen deberian ir en comandos/reglas separadas.

## Orden Recomendado

1. Consistencia de syscalls.
2. Memory manager simple.
3. Syscalls de memoria y comando `mem`.
4. Buddy system.
5. Procesos basicos.
6. Context switch.
7. Scheduler Round Robin.
8. Prioridades, `ps`, `kill`, `nice`, `block`, `loop`.
9. Semaforos.
10. `test_sync`.
11. Pipes y file descriptors.
12. `cat`, `wc`, `filter`.
13. Shell con `&`, `|`, `Ctrl+C`, `Ctrl+D`.
14. `mvar`.
15. README, limpieza y defensa.

## Prioridad si falta tiempo

### Prioridad alta
- Memory manager simple.
- Procesos.
- Scheduler.
- Semaforos.
- Tests obligatorios.
- Comandos basicos: `mem`, `ps`, `kill`, `nice`, `block`, `loop`.

### Prioridad media
- Buddy system.
- Pipes.
- `cat`, `wc`, `filter`.
- Shell con background y pipe.

### Prioridad final
- `mvar`.
- Pulido de README.
- Casos borde y ejemplos extra para defensa.


borrar: Cuando termines de verificar, en kernel.c:

Borrá el bloque DEMO: prueba del cambio de contexto (las funciones test_delay, uint_to_str, test_process, launch_context_demo).
Borrá la línea launch_context_demo(); en main().
Sacá el #include <textConsole.h> si no lo usás en otro lado.