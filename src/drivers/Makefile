
CROSS=i386-elf-
CC = $(CROSS)gcc
LD = $(CROSS)ld

CFLAGS = -fPIC -Wall -Wextra -Wno-builtin-declaration-mismatch -Wno-unused-parameter -I ../../include -I ../../include/cc -I ../../arch/x86/include
LFLAGS = 

V = @
Q = @ echo 

srcdir := ./fs/isofs
objdir := ./obj
name = ${shell basename ${srcdir}}

srcs=$(wildcard ${srcdir}/*.c)
objs=$(patsubst ${srcdir}/%.c, ${objdir}/%.o, ${srcs})

all: ${name}.kmo

echo:
	${Q} '  NAME  '${name}
	${Q} '  SRC   '${srcdir}
	${Q} '  OBJ   '${objdir}
	${Q} '  SRCS  '${srcs}
	${Q} '  OBJS  '${objs}

${name}.kmo: ${objs}
	${Q} '    LD  '$@
	${V} ${LD} -shared ${LFLAGS}  ${objs} -o $@ -h $@ 

${objdir}/%.o: $(srcdir)/%.c
	${Q} '    CC  '$<
	@ mkdir -p ${objdir}
	${V} ${CC} ${CFLAGS} -c $< -o $@ 

${objdir}/%.o: %.c
	${Q} '    CC  '$<
	@ mkdir -p ${objdir}
	${V} ${CC} ${CFLAGS} -c $< -o $@ 

clean:
	@ rm -rf obj

