#
# Single file C library for decoding MPEG1 Video and MP2 Audio Dreamcast preliminary port KallistiOS video pvr no sound
#   Ian micheal
KOS_CFLAGS += -std=c99
TARGET = mpegDC.elf
OBJS = mpegDC.o matrix.o romdisk.o

KOS_LOADER = sudo dc-tool-ip -t 192.168.0.118 -c . -x

KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS) romdisk.o
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -lkosutils -lpng -ljpeg -lkmg -lz -lm $(KOS_LIBS)

romdisk.img:
	$(KOS_GENROMFS) -f $@ -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o $< romdisk $@

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

