#!/bin/bash
# Ejecuta la imagen en QEMU desde WSL.
#
# Diferencias con run.sh:
#   - No usa ALSA (-audiodev alsa / pcspk-audiodev): en WSL no hay servidor de
#     audio por defecto y QEMU fallaria. La ventana se muestra via WSLg.
#   - Usa -snapshot: el .qcow2 lo crea Docker como root con permisos 644, y en
#     /mnt/c (DrvFS) no se puede hacer chown/chmod, asi que el usuario solo
#     tiene lectura. Con -snapshot QEMU abre el base en modo lectura y manda las
#     escrituras a un archivo temporal descartable. Este SO no persiste a disco,
#     asi que no se pierde nada.
set -e

IMG="Image/x64BareBonesImage.qcow2"

if [ ! -f "$IMG" ]; then
    echo "No existe $IMG. Compila primero (./compile.sh o make dentro del contenedor)."
    exit 1
fi

qemu-system-x86_64 -hda "$IMG" -m 512 -snapshot
