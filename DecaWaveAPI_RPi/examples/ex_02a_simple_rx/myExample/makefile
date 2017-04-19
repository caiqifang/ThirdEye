PROJPATH = ~/549proj/dw1000_api_rev2p03
DRIVERPATH = ${PROJPATH}/decadriver
PLATFORMPATH = ${PROJPATH}/myPlatform
INCLUDEFLAG = -I ${DRIVERPATH} -I ${PLATFORMPATH}


all:
	gcc -Wall -fpic ${INCLUDEFLAG} main.c ${PLATFORMPATH}/deca_*.c ${DRIVERPATH}/deca_*.c -l bcm2835

