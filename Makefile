LIBDIR=/usr/local/lib
INCDIR=/usr/local/include

CC=gcc
AR=ar
LD=ld
LIBS=-L/usr/X11R6/lib -lGL -lGLU -lglut -lXi -lm
CFLAGS=-Wall -O2
#CFLAGS=-Wall -g -DDEBUG

all: libglc.a libglc.so glcdemo

libglc.a: glc.o
	$(AR) rc libglc.a glc.o

libglc.so: glc.o
	$(LD) -shared -o libglc.so glc.o

glc.o: glc.c glc.h
	$(CC) glc.c -c $(CFLAGS)

glcdemo: libglc.a glcdemo.o
	$(CC) $(LIBS) glcdemo.o -o glcdemo libglc.a

glcdemo.o: glcdemo.c glc.h
	$(CC) glcdemo.c -c $(CFLAGS)

clean:
	rm -f glcdemo *.o *.a *.so

install:
	install -C libglc.a $(LIBDIR)
	install -C libglc.so $(LIBDIR)
	install -C glc.h $(INCDIR)
