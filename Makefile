# Makefile for the gpio driver.
PROG=   gpiodriver
SRCS=   gpiodriver.c

FILES=${PROG}.conf
FILESNAME=${PROG}
FILESDIR= /etc/system.conf.d

DPADD+= ${LIBVTREEFS} ${LIBFSDRIVER} ${LIBSYS} ${LIBGPIO} ${LIBCLKCONF} ${LIBTIMERS}
LDADD+= -lvtreefs -lfsdriver -lsys -lgpio -lclkconf -ltimers

# This is a system driver.
CPPFLAGS+= -D_SYSTEM=1

MAN=

.include <minix.service.mk>
