PROJECT_DIR:= $(dir $(lastword $(MAKEFILE_LIST)))

all : flash

CH32V003FUN:= ${HOME}/.local/include/ch32v003fun/ch32v003fun
TARGET:=EPV
# CH32V003_lib_i2c:=${HOME}/.local/include/CH32V003_lib_i2c
ch32v003_lib_shift_register:=${HOME}/projects/ch32v003/libs/ch32v003_lib_shift_register/

EXTRA_CFLAGS+=-std=gnu23

#libraries
# include ${CH32V003_lib_i2c}/CH32V003_lib_i2c.mk
include ${ch32v003_lib_shift_register}/ch32v003_lib_shift_register.mk

include ${CH32V003FUN}/ch32v003fun.mk

flash : cv_flash
clean : cv_clean


