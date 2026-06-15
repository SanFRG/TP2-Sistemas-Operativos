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

Compila bootloader, kernel, userland e imagen final usando el memory manager por defecto (`Kernel/mm/memoryManager.c`).

```sh
make all
```

Equivalente a `make`.

```sh
make buddy
```

Compila la imagen usando el memory manager buddy (`Kernel/mm/memoryManagerBuddy.c`).

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
| `ps` | ninguno | Lista PID, PPID, prioridad, foreground, estado, stack pointer (SP), base pointer (BP), contador y nombre de cada proceso activo. |
| `memtest` | ninguno | Prueba simple de `alloc/free` con bloques de 64 y 256 bytes. |
| `testmm` | `<maxmem>` | Stress test del memory manager. Pide bloques aleatorios hasta `<maxmem>` bytes, los escribe, verifica y libera en loop. Frenar con `Ctrl+C` o `kill`. |
| `testprio` | `<max_value>` | Lanza 3 procesos que cuentan hasta `<max_value>`; demuestra que la mayor prioridad imprime `DONE!` primero. |
| `testproc` | `<n>` | Crea, mata, bloquea y desbloquea `<n>` procesos (1-16) al azar en loop. Frenar con `Ctrl+C` o `kill`. |
| `testsync` | `<pares> <iteraciones> <use_sem: 0\|1>` | Crea `<pares>` pares de procesos que incrementan/decrementan una variable global; con semaforo el resultado final es 0. |
| `mvar` | `<escritores> <lectores>` | Crea `<escritores>` y `<lectores>` (1 a 8 cada uno) sobre una MVar (1 celda) sincronizada con semaforos. |
| `regs` | ninguno | Muestra el ultimo snapshot de registros guardado. |
| `clear` | ninguno | Limpia la pantalla. |
| `loop` | `[prio]`, `-p <prio>` (con `&` para background) | Crea un proceso `loop`. Prioridad entre `0` y `2` (default `1`). En foreground imprime su PID y contador periodicamente. |
| `kill` | `<pid>` | Mata un proceso por PID. |
| `nice` | `<pid> <prio>` | Cambia prioridad de un proceso. Prioridad valida: `0` a `2`. |
| `block` | `<pid>` | Bloquea un proceso READY/RUNNING o desbloquea uno BLOCKED. |
| `cat` | ninguno | Imprime el stdin tal como lo recibe. |
| `wc` | ninguno | Cuenta la cantidad de lineas del input. |
| `filter` | ninguno | Filtra las vocales del input (las elimina, pasa el resto). |
| `exit` | ninguno | Cierra la shell y vuelve al kernel. |

### Tests de catedra

Los cuatro tests de la catedra (`testmm`, `testprio`, `testproc`, `testsync`) estan portados desde `MemoryTest/` a la API de userland de este TP e integrados como comandos de la shell (ver tabla anterior), en `Userland/UserModule/commands/shell_mem_cmds.c` (`testmm`) y `Userland/UserModule/tests/` (`testprio`, `testproc`, `testsync`). Los fuentes originales de catedra quedan en `MemoryTest/` solo como referencia; sus wrappers de `MemoryTest/syscall.c` son stubs y no se compilan en el kernel.

Ejemplos de invocacion: `testmm 100000`, `testprio 1000000`, `testproc 5`, `testsync 2 1000 1`, `mvar 2 3`.

## Caracteres especiales

### Foreground y background

Por defecto los comandos corren en foreground: la shell se bloquea hasta que el proceso termina. Para terminar un proceso en foreground se usa `Ctrl+C`; la shell detecta el atajo, llama a `kill` sobre el PID foreground y luego hace `waitpid`.

Agregando `&` al final, el comando corre en background: la shell muestra el PID del proceso y devuelve el prompt inmediatamente.

```txt
loop              # foreground: se termina con Ctrl+C
loop &            # background: imprime el PID y vuelve al prompt
testmm &
testsync 100 1 1 &
```

### Pipes

El caracter `|` conecta el stdout del comando izquierdo con el stdin del derecho. El proceso izquierdo escribe en el pipe; el derecho lee de forma bloqueante. Cuando el escritor termina, el lector recibe EOF y finaliza.

```txt
cat | wc               # cat envia lineas a wc, wc cuenta las lineas
cat | filter           # cat envia lineas a filter, filter quita vocales
cat | filter &         # en background: la shell no se bloquea mientras escribis
loop | wc              # loop genera lineas, wc las cuenta
time | cat             # time escribe fecha/hora, cat lo reenvia a pantalla
```

Para probarlo interactivamente con `cat | wc` o `cat | filter`: escribi varias lineas y presiona `Ctrl+D`; `cat` cierra el pipe y el comando derecho procesa lo recibido (`wc` imprime la cantidad de lineas, `filter` muestra el texto sin vocales).

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
testmm
mem
```

Demuestra consulta de estado, reservas/liberaciones simples y stress test de memoria.

### Procesos y listado

```txt
ps
loop &
ps
```

Crea un proceso `loop` en background (con `&`) y permite verlo en la tabla de procesos.

### Prioridades

```txt
loop -p 0 &
loop -p 2 &
ps
nice <pid> 1
ps
```

Demuestra creacion de procesos con distintas prioridades y cambio posterior de prioridad.

### Bloqueo y desbloqueo

```txt
loop &
ps
block <pid>
ps
block <pid>
ps
```

El primer `block` pasa el proceso a BLOCKED. El segundo lo desbloquea y vuelve a READY.

### Semaforos y sincronizacion

```txt
testsync 2 1000 0
testsync 2 1000 1
```

El primer comando ejecuta la prueba sin semaforo y puede mostrar condicion de carrera. El segundo usa un semaforo nombrado para proteger la variable compartida; el valor final esperado es 0 (los incrementos y decrementos se cancelan).

### Kill y recoleccion

```txt
loop &
ps
kill <pid>
ps
```

Mata un proceso. Si el proceso matado es hijo de la shell, el comando hace `waitpid` para liberar su stack y su slot de PCB.

### MVar (productor/consumidor)

```txt
mvar 2 3
```

Lanza 2 escritores y 3 lectores que comparten una MVar de una sola celda. Los semaforos `empty`/`full` garantizan que cada valor escrito sea leido exactamente una vez antes de escribir el siguiente. Cada proceso imprime con un color distinto. Frenar matando los procesos con `kill` o `Ctrl+C`.

### EOF

En el prompt, presionar `Ctrl+D`. La shell interpreta EOF y vuelve a mostrar el prompt.

## Requerimientos implementados

Resumen de cobertura de la consigna. El detalle de uso de cada item esta en las secciones anteriores.

- **Memory manager**: implementacion por defecto (free-list) y buddy allocator seleccionable con `make buddy`, ambos con la interfaz `mm_alloc` / `mm_free` / `mm_get_status`. Tests unitarios de host para los dos.
- **Procesos**: creacion con PCB (PID, PPID, stack propio, estado, prioridad, file descriptors), context switch, scheduler Round Robin ponderado por prioridad y proceso idle.
- **`waitpid`** bloqueante: el padre queda BLOCKED hasta que el hijo termina.
- **Semaforos** nombrados (`sem_open`, `sem_close`, `sem_wait`, `sem_post`) con lock atomico basado en `xchg` y bloqueo sin busy waiting.
- **Pipes** bloqueantes con buffer circular (`pipe_open`, `pipe_read`, `pipe_write`) y cierre automatico por exit, con redireccion de `read`/`write` por file descriptors.
- **Shell**: pipelines `cmd1 | cmd2`, ejecucion en background con `&`, y `Ctrl+C` / `Ctrl+D`.
- **Tests de catedra** (`testmm`, `testprio`, `testproc`, `testsync`) portados e integrados como comandos.

## Requerimientos faltantes o parcialmente implementados

- No hay requerimientos faltantes: todos los requerimientos obligatorios del enunciado estan implementados
- Parcial: los pipes conectan exactamente 2 procesos por linea (`p1 | p2`); no se soporta encadenar mas de dos (`p1 | p2 | p3`)

## Limitaciones

- La redireccion por pipe solo se aplica a stdin (`fd[0]`) y stdout (`fd[1]`); `fd[2]` (stderr) nunca se redirige, asi que los mensajes de error siempre van a pantalla.
- `kill` no permite matar el proceso actual ni el proceso idle.
- Los procesos en estado `KILLED` no se listan en `ps`.
- Un proceso `TERMINATED` o `KILLED` cuyo padre vivo nunca llama a `waitpid` puede quedar ocupando un slot. Si el padre muere, el scheduler intenta recolectar huerfanos terminados.
- `Ctrl+D` no termina la shell; solo devuelve EOF para la lectura actual.
- Los pipes son unidireccionales y conectan solo 2 procesos por linea de comando.

## Citas y uso de IA

### Codigo de base y de terceros

- El proyecto parte de la base **x64BareBones / Pure64 / BMFS** provista por la catedra (bootloader y esqueleto de kernel/userland).
- Los tests `testmm`, `testprio`, `testproc` y `testsync` estan adaptados de los tests provistos por la catedra (`MemoryTest/`) a la API de userland de este TP.

### Uso de inteligencia artificial

Se utilizo un asistente de IA para generar un plan de tareas inicial y para entender conceptos que no nos habian quedado del todo claros.