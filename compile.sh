docker start TPE
docker exec -it TPE make clean -C /root/Toolchain
docker exec -it TPE make clean -C /root/
docker exec -it TPE make -C /root/Toolchain
docker exec -it TPE make -C /root/
#docker stop TPE
# El .qcow2 lo crea Docker como root; le devolvemos la propiedad al usuario
# para que QEMU lo pueda abrir en modo lectura-escritura.
sudo chown $USER:$USER Image/x64BareBonesImage.qcow2
./run.sh