
# CROSS=i386-elf-
CC = $(CROSS)gcc
LD = $(CROSS)ld

CFLAGS = -fPIC -Wall -Wextra -fno-builtin -Wno-unused-parameter
CFLAGS += -I ${srcdir}/../../include
CFLAGS += -I ${srcdir}/../../include/cc -I ${srcdir}/../../arch/x86/include
LFLAGS =

V = @
Q = @ echo

srcdir := .
objdir := ./obj
bindir := ./bin
libdir := ./lib
driver := fs/isofs
name = ${shell basename ${driver}}

srcs=$(wildcard ${srcdir}/${driver}/*.c)
objs=$(patsubst ${srcdir}/${driver}/%.c, ${objdir}/%.o, ${srcs})

all: ${bindir}/${name}.kmo ${libdir}/${name}.a

echo:
	${Q} '  NAME  '${name}
	${Q} '  SRC   '${srcdir}/${driver}
	${Q} '  OBJ   '${objdir}
	${Q} '  SRCS  '${srcs}
	${Q} '  OBJS  '${objs}

${libdir}/${name}.a: ${objs}
	${Q} '    AR  '$@
	@ mkdir -p ${libdir}
	${V} ${AR} rc $@ ${objs}


${bindir}/${name}.kmo: ${objs}
	${Q} '    LD  '$@
	@ mkdir -p ${bindir}
	${V} ${LD} -shared ${LFLAGS}  ${objs} -o $@ -h $@

${objdir}/%.o: $(srcdir)/${driver}/%.c
	${Q} '    CC  '$<
	@ mkdir -p ${objdir}
	${V} ${CC} ${CFLAGS} -c $< -o $@

${objdir}/%.o: %.c
	${Q} '    CC  '$<
	@ mkdir -p ${objdir}
	${V} ${CC} ${CFLAGS} -c $< -o $@

clean:
	@ rm -rf obj

