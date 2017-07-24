

# the global debug level
DEB_LEVEL = 0

ccflags = -c -DDEBUG_LEVEL=${DEB_LEVEL} -g -O0 -Werror
ccflagsS = $(ccflags) -DASM_SOURCE=1

ldflags = -lc -lreadline


objectsC_ext2 = fs/vfs.o fs/ext2.o fs/gendrivers.o libs32/klib.o libs32/utils32.o



# headers

headers = *.h

headerskernel = fs/*.h libs/*.h libs32/*.h

headersall = $(headerskernel) 

debugs = 


# ld for ext2.bin

ext2: $(objectsC_ext2)
	gcc  $(ldflags) -o ext2 $(objectsC_ext2)


# shortcuts




# compiler section



# ext2

$(objectsC_ext2): %.o : %.c *.h $(headerskernel)
	gcc $(ccflags) -I$(PWD) -I. -o $@ -c $<


$(objectsS_ext2) : %.o : %.S
	gcc $(ccflagsS) -I. -o $@ -c $<


.PHONY	: clean

clean	:
	rm -f fs/*.o
	rm -f libs32/*.o
	rm -f ext2

.PHONY	: all

all	: ext2


