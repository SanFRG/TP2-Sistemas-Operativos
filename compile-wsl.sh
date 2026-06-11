#!/bin/bash
# Compila el proyecto dentro del contenedor Docker TPE y lo corre en QEMU desde
# WSL. Variante de compile.sh pensada para WSL:
#   - No usa "docker exec -it" (sin TTY interactivo).
#   - No hace "sudo chown" del .qcow2: en /mnt/c (DrvFS) no tiene efecto.
#   - Al final llama a run-wsl.sh, que usa -snapshot en vez de ALSA.
set -e

docker start TPE
docker exec TPE make clean -C /root/Toolchain
docker exec TPE make clean -C /root/
docker exec TPE make -C /root/Toolchain
docker exec TPE make -C /root/

./run-wsl.sh
