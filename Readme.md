# TP2 - Sistemas Operativos

Trabajo practico de Sistemas Operativos sobre la base x64BareBones/Pure64.

## Integrantes
- Carolina Goldbaum - 65112
- Santiago Fraga - 64346
- Ivonne Choe - 64880

## Instrucciones de compilacion

### Requisitos

- Docker con un contenedor llamado `TPE` que contenga el toolchain del proyecto.
- QEMU (`qemu-system-x86_64`) para ejecutar la imagen.


### Compilar y ejecutar con script

```sh
./compile.sh
```

Este script:

1. Inicia el contenedor Docker `TPE`.
2. Limpia `Toolchain` y el proyecto dentro de `/root/`.
3. Compila el toolchain.
4. Compila el sistema completo.
5. Cambia el duenio de `Image/x64BareBonesImage.qcow2` al usuario actual.
6. Ejecuta `./run.sh`.

### Compilar manualmente

```sh
make
```

Compila bootloader, kernel, userland e imagen final usando el memory manager por defecto (`Kernel/memoryManager.c`).

```sh
make all
```

Equivalente a `make`.

```sh
make buddy
```

Compila la imagen usando el memory manager buddy (`Kernel/memoryManagerBuddy.c`).

```sh
make clean
```

Limpia artefactos generados por bootloader, imagen, kernel y userland.

## Instrucciones de ejecucion

```sh
./run.sh
```

Ejecuta:

```sh
qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512 -audiodev alsa,id=speaker -machine pcspk-audiodev=speaker
```

La shell se inicia automaticamente al bootear el sistema. El prompt es:

```txt
>
```

## Tests de host

Los tests de `Test/` corren fuera del kernel y prueban los memory managers con CuTest.

```sh
cd Test
make run
```

Compila y ejecuta los tests del memory manager por defecto.

```sh
cd Test
make buddy
./MemoryManagerTest.out
```

Compila y ejecuta los mismos tests contra el buddy allocator.

```sh
cd Test
make valgrind
```

Ejecuta los tests bajo Valgrind.

```sh
cd Test
make clean
```

Elimina el binario de tests.

## Comandos de la shell

Los comandos no distinguen mayusculas de minusculas.

| Comando | Parametros | Descripcion |
| --- | --- | --- |
| `help` | ninguno | Lista los comandos disponibles. |
| `time` | ninguno | Muestra fecha y hora obtenidas desde el sistema. |
| `mem` | ninguno | Muestra estado del memory manager: total, usado, libre, allocs/frees correctos y allocs fallidos. |
| `pid` | ninguno | Muestra el PID del proceso actual. |
| `ps` | ninguno | Lista PID, PPID, prioridad, foreground, estado, contador y nombre de cada proceso activo. |
| `memtest` | ninguno | Prueba simple de `alloc/free` con bloques de 64 y 256 bytes. |
| `test_mm` | ninguno | Stress test acotado del memory manager desde userland. Reserva, escribe, verifica y libera bloques. |
| `regs` | ninguno | Muestra el ultimo snapshot de registros guardado. |
| `clear` | ninguno | Limpia la pantalla. |
| `cerodiv` | ninguno | Dispara una excepcion de division por cero. |
| `invalido` | ninguno | Dispara una excepcion de opcode invalido. |
| `cancion` | ninguno | Reproduce una melodia por PC speaker. |
| `loop` | `[prio] [fg]`, `-p <prio>`, `-b` | Crea un proceso `loop`. Prioridad entre `0` y `2`. `-b` lo lanza en background. |
| `kill` | `<pid>` | Mata un proceso por PID. |
| `nice` | `<pid> <prio>` | Cambia prioridad de un proceso. Prioridad valida: `0` a `2`. |
| `block` | `<pid>` | Bloquea un proceso READY/RUNNING o desbloquea uno BLOCKED. |

### Tests disponibles desde la shell

| Test | Parametros | Descripcion |
| --- | --- | --- |
| `memtest` | ninguno | Test manual corto de reserva y liberacion. |
| `test_mm` | ninguno | Test de stress acotado del memory manager. |
| `cerodiv` | ninguno | Verifica manejo de excepcion de division por cero. |
| `invalido` | ninguno | Verifica manejo de excepcion de opcode invalido. |

Los fuentes de tests de catedra estan en `MemoryTest/`, pero no estan integrados como comandos de userland. Ademas, sus wrappers de syscalls (`MemoryTest/syscall.c`) son stubs que devuelven `0`, por lo que no ejercitan el kernel actual.

## Caracteres especiales

### Background

El caracter requerido para background es:

```txt
&
```

Estado actual: el parser lo reconoce y lo consume, pero no lo aplica genericamente a todos los comandos. Para demostrar background con lo implementado, usar:

```txt
loop -b
```

Tambien se puede crear `loop` en background con:

```txt
loop 1 0
```

### Pipes

El caracter requerido para pipes es:

```txt
|
```

Estado actual: no esta implementado el parser de pipes ni la infraestructura de pipes/file descriptors para conectar procesos. Comandos como `cat`, `wc` y `filter` tampoco estan implementados.

## Atajos de teclado

| Atajo | Comportamiento |
| --- | --- |
| `Ctrl+C` | Interrumpe la lectura o mata el proceso foreground registrado por la shell. |
| `Ctrl+D` | Envia EOF a la lectura actual. La shell recibe EOF y vuelve al prompt; no finaliza la shell. |

## Ejemplos de uso

### Memory manager

```txt
mem
memtest
test_mm
mem
```

Demuestra consulta de estado, reservas/liberaciones simples y stress test de memoria.

### Procesos y listado

```txt
ps
loop -b
ps
```

Crea un proceso `loop` en background y permite verlo en la tabla de procesos.

### Foreground y Ctrl+C

```txt
loop
```

El proceso corre en foreground. Para terminarlo, presionar `Ctrl+C`. La shell detecta el atajo, llama a `kill` sobre el PID foreground y luego hace `waitpid`.

### Prioridades

```txt
loop -b -p 0
loop -b -p 2
ps
nice <pid> 1
ps
```

Demuestra creacion de procesos con distintas prioridades y cambio posterior de prioridad.

### Bloqueo y desbloqueo

```txt
loop -b
ps
block <pid>
ps
block <pid>
ps
```

El primer `block` pasa el proceso a BLOCKED. El segundo lo desbloquea y vuelve a READY.

### Kill y recoleccion

```txt
loop -b
ps
kill <pid>
ps
```

Mata un proceso. Los procesos hijos terminados se liberan completamente cuando su padre hace `waitpid`; desde la shell esto ocurre para el proceso foreground.

### Excepciones

```txt
cerodiv
invalido
```

Disparan excepciones para validar el manejo de errores de CPU.

### EOF

En el prompt, presionar `Ctrl+D`. La shell interpreta EOF y vuelve a mostrar el prompt.

## Requerimientos implementados

- Memory manager por defecto con interfaz `mm_alloc`, `mm_free` y `mm_get_status`.
- Buddy allocator seleccionable con `make buddy`.
- Syscalls de memoria y comando `mem`.
- Creacion de procesos con PCB, PID, stack propio, estado, prioridad, PPID y file descriptors basicos.
- Context switch y scheduler Round Robin con prioridades por quantum.
- Comandos `ps`, `kill`, `nice`, `block` y `loop`.
- `waitpid` bloqueante para hijos: el padre queda BLOCKED hasta que el hijo termina.
- Proceso idle para cuando no hay procesos READY.
- `Ctrl+C` para interrumpir lectura o matar foreground.
- `Ctrl+D` como EOF de lectura.
- Test `test_mm` en userland.
- Tests unitarios de host para ambos memory managers.

## Requerimientos faltantes o parcialmente implementados

- `&`: parcialmente implementado. Se parsea, pero no lanza cualquier comando en background. Para background real usar `loop -b`.
- Foreground/background: parcialmente implementado. El PCB tiene campo `foreground` y la shell espera un PID foreground, pero no hay manejo generico para todos los comandos.
- `|`: no implementado.
- Pipes bloqueantes: no implementados.
- Redireccion de `read/write` por file descriptors hacia pipes: no implementada. `write` ignora el `fd` y escribe en pantalla.
- Semaforos: no implementados.
- `test_sync`: el fuente esta en `MemoryTest/`, pero no esta integrado como comando y depende de semaforos no implementados.
- `test_proc`: el fuente esta en `MemoryTest/`, pero no esta integrado como comando y sus wrappers de syscall son stubs.
- `test_prio`: el fuente esta en `MemoryTest/`, pero no esta integrado como comando y sus wrappers de syscall son stubs.
- Comandos `cat`, `wc`, `filter`: no implementados.
- Comando `mvar`: no implementado.
- Comando `exit`: aparece en la ayuda, pero no esta registrado en la tabla de comandos y no cierra la shell.
- Pipes encadenados tipo `p1 | p2 | p3`: no implementados.

## Limitaciones

- El sistema no tiene semaforos ni primitivas de sincronizacion de userland.
- No hay IPC por pipes.
- El scheduler usa Round Robin circular; la prioridad modifica el quantum (`prio + 1`), no el orden absoluto de seleccion.
- `kill` no permite matar el proceso actual ni el proceso idle.
- Los procesos en estado `KILLED` no se listan en `ps`.
- Un proceso `TERMINATED` o `KILLED` cuyo padre nunca llama a `waitpid` puede quedar ocupando un slot. Si un padre muere antes que sus hijos, no hay reasignacion a un proceso `init` ni recoleccion automatica completa.
- `Ctrl+D` no termina la shell; solo devuelve EOF para la lectura actual.
- `regs` depende de que exista un snapshot de registros previo; si no lo hay, informa error.
- La entrada de la shell soporta tokenizacion simple por espacios. No hay comillas, escaping ni multiples comandos por linea.
- La compilacion por `compile.sh` asume un contenedor Docker existente llamado `TPE`.
- `run.sh` asume backend de audio ALSA para el PC speaker.



kill ya está bastante implementado: hay syscall, comando kill <pid>, process_kill() marca el proceso como KILLED, y el scheduler deja de elegirlo.

  Lo que falta para que quede “bien cerrado” es principalmente la recolección:

  - Para procesos en background, cmd_kill() mata el proceso pero no llama a waitpid(pid).
  - Entonces el proceso queda en la tabla como KILLED, no aparece en ps porque process_list() lo saltea, pero el slot y el stack pueden quedar ocupados.
  - Para foreground sí está mejor: la shell hace kill_process(fg_pid) y después waitpid(fg_pid).

  El arreglo mínimo sería que cmd_kill() haga:

  if (kill_process(pid) == 0) {
      waitpid(pid);
      ...
  }

  Eso funcionaría si el proceso matado es hijo de la shell, que es el caso normal para procesos creados desde la shell como loop -b.

  Para que kill quede más completo a nivel kernel también faltaría definir qué pasa con hijos del proceso matado: hoy no se reasignan a init ni se recolectan automáticamente. Y por diseño actual
  no se puede matar el proceso actual ni el proceso idle, eso está explícito en process_kill().