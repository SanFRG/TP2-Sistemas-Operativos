include Makefile.inc

USER_DATA=0001-userDataModule.bin

all: userModule userDataModule

userModule:
	cd UserModule; make

userDataModule:
	printf "This is user data." >> $(USER_DATA) && dd if=/dev/zero bs=1 count=1 >> $(USER_DATA)

clean:
	cd UserModule; make clean
	rm -rf *.bin


.PHONY: userModule all clean
