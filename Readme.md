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
| `test_sync` | `<iteraciones> <use_sem: 0|1>` | Ejecuta una prueba de sincronizacion con o sin semaforo. |
| `cat` | ninguno | Imprime el stdin tal como lo recibe. |
| `wc` | ninguno | Cuenta la cantidad de lineas del input. |
| `filter` | ninguno | Filtra las vocales del input (las elimina, pasa el resto). |
| `exit` | ninguno | Cierra la shell y vuelve al kernel. |

### Tests disponibles desde la shell

| Test | Parametros | Descripcion |
| --- | --- | --- |
| `memtest` | ninguno | Test manual corto de reserva y liberacion. |
| `test_mm` | ninguno | Test de stress acotado del memory manager. |
| `test_sync` | `<iteraciones> <use_sem: 0|1>` | Crea procesos que modifican una variable compartida. Con `use_sem=1` sincroniza con semaforos. |
| `cerodiv` | ninguno | Verifica manejo de excepcion de division por cero. |
| `invalido` | ninguno | Verifica manejo de excepcion de opcode invalido. |

Los fuentes de tests de catedra estan en `MemoryTest/`. El test de sincronizacion integrado en la shell es la version de userland en `Userland/UserModule/test_sync_userland.c`; los wrappers de `MemoryTest/syscall.c` siguen siendo stubs y no ejercitan el kernel actual.

## Caracteres especiales

### Background

El caracter requerido para background es:

```txt
&
```

Estado actual: implementado. Cualquier comando seguido de `&` se ejecuta en background. La shell muestra el PID del proceso y devuelve el prompt inmediatamente.

Ejemplos:

```txt
loop &
test_mm &
test_sync 100 1 &
```

### Pipes

El caracter requerido para pipes es:

```txt
|
```

Conecta stdout del comando izquierdo con stdin del comando derecho. Ejemplos:

```txt
cat | wc               # cat envia lineas a wc, wc cuenta las lineas
cat | filter           # cat envia lineas a filter, filter quita vocales
cat | filter &         # en background: la shell no se bloquea mientras escribis
loop | wc              # loop genera lineas, wc las cuenta
time | cat             # time escribe fecha/hora, cat lo reenvia a pantalla
```

El proceso izquierdo escribe en el pipe; el proceso derecho lee del pipe de forma bloqueante. Cuando el escritor termina, el lector recibe EOF y finaliza.

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

### Pipes

```txt
cat | wc
```

Escribi varias lineas de texto. Al presionar `Ctrl+D`, `cat` termina y cierra el pipe. `wc` imprime la cantidad de lineas recibidas.

```txt
cat | filter
```

Escribi texto con vocales. `filter` elimina todas las vocales y escribe el resto en pantalla. `Ctrl+D` para terminar.

### Semaforos y sincronizacion

```txt
test_sync 1000 0
test_sync 1000 1
```

El primer comando ejecuta la prueba sin semaforo y puede mostrar condicion de carrera. El segundo usa un semaforo nombrado para proteger la variable compartida; el valor final esperado es 0 (los incrementos y decrementos se cancelan).

### Kill y recoleccion

```txt
loop -b
ps
kill <pid>
ps
```

Mata un proceso. Si el proceso matado es hijo de la shell, el comando hace `waitpid` para liberar su stack y su slot de PCB.

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
- Semaforos nombrados con `sem_open`, `sem_close`, `sem_wait` y `sem_post`.
- Bloqueo de procesos en `sem_wait` sin busy waiting cuando no hay recursos disponibles.
- Pipes bloqueantes con buffer circular: `pipe_open`, `pipe_read`, `pipe_write`, cierre automatico por exit.
- Redireccion de `write`/`read` por file descriptors: fd[0]/fd[1] del PCB apuntan a teclado/pantalla (valores 0/1) o a un pipe (valor >= 3).
- Comandos `cat`, `wc` y `filter` que operan sobre stdin/stdout y funcionan en pipelines.
- Soporte de `cmd1 | cmd2` en la shell: crea pipe, lanza ambos procesos con fds correctos y espera a que terminen.
- Soporte de `&` para ejecutar cualquier comando en background.
- Proceso idle para cuando no hay procesos READY.
- `Ctrl+C` para interrumpir lectura o matar foreground.
- `Ctrl+D` como EOF de lectura.
- Test `test_mm` en userland.
- Test `test_sync` en userland.
- Tests unitarios de host para ambos memory managers.

## Requerimientos faltantes o parcialmente implementados

- Pipes encadenados tipo `p1 | p2 | p3`: no implementados (solo un pipe por linea).
- `test_proc`: el fuente esta en `MemoryTest/`, pero no esta integrado como comando y sus wrappers de syscall son stubs.
- `test_prio`: el fuente esta en `MemoryTest/`, pero no esta integrado como comando y sus wrappers de syscall son stubs.
- Comandos `cat`, `wc`, `filter`: no implementados.
- Comando `mvar`: no implementado.
- Pipes encadenados tipo `p1 | p2 | p3`: no implementados.

## Limitaciones

- El buffer de cada pipe es de 256 bytes; escrituras mayores bloquean hasta que el lector consuma espacio.
- Solo se soporta un pipe por linea de comando (`cmd1 | cmd2`). No hay pipes encadenados.
- `stderr` (fd=2) siempre va a pantalla aunque fd[2] este redirigido.
- `PCB.fd[2]` (stderr) puede redirigirse a un pipe internamente, pero los mensajes de error del kernel siempre van a pantalla.
- El scheduler usa Round Robin circular; la prioridad modifica el quantum (`prio + 1`), no el orden absoluto de seleccion.
- `kill` no permite matar el proceso actual ni el proceso idle.
- Los procesos en estado `KILLED` no se listan en `ps`.
- Un proceso `TERMINATED` o `KILLED` cuyo padre vivo nunca llama a `waitpid` puede quedar ocupando un slot. Si el padre muere, el scheduler intenta recolectar huerfanos terminados.
- `Ctrl+D` no termina la shell; solo devuelve EOF para la lectura actual.
- `regs` depende de que exista un snapshot de registros previo; si no lo hay, informa error.
- La entrada de la shell soporta tokenizacion simple por espacios. No hay comillas, escaping ni multiples comandos por linea.
- La compilacion por `compile.sh` asume un contenedor Docker existente llamado `TPE`.
- `run.sh` asume backend de audio ALSA para el PC speaker.
