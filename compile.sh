docker start TPE
docker exec -it TPE make clean -C /root/Toolchain
docker exec -it TPE make clean -C /root/
docker exec -it TPE make -C /root/Toolchain
docker exec -it TPE make -C /root/
#docker stop TPE
./run.sh