#
# Single file C library for decoding MPEG1 Video and MP2 Audio Dreamcast preliminary port KallistiOS video pvr no sound
#   Ian micheal
KOS_CFLAGS += -std=c99
TARGET = pl_mpegDC.elf
OBJS = pl_mpegDC.o  romdisk.o

KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA)  -lkosutils -lpng -ljpeg -lkmg -lz -lm $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

