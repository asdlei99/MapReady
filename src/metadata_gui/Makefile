include ../../make_support/system_rules

TARGET = mdv
SRCS = mdv.c

# Unfortunately compilation is completely different on windows
ifeq ($(SYS),win32)

### Begin Windows Section

GCC = gcc
GTK_INC = $(GTKWIN32_DIR)/include
GTK_LIB = $(GTKWIN32_DIR)/lib

ASF_LIB = ../../lib

INCLUDE = \
	-I$(GTK_INC) \
	-I$(GTK_INC)/glib-2.0 \
	-I$(GTK_INC)/pango-1.0 \
	-I$(GTK_INC)/gtk-2.0 \
	-I$(GTK_INC)/freetype2/freetype \
	-I$(GTK_LIB)/glib-2.0/include \
	-I$(GTK_LIB)/gtk-2.0/include \
	-I$(GTK_INC)/atk-1.0 \
	-I$(GTK_INC)/libglade-2.0 \
	-I$(ASF_INCLUDE_DIR)

LIB = \
	$(LDFLAGS) \
	-L$(GTK_LIB) \
	-L$(GTK_LIB)/glib-2.0 \
	-L$(GTK_LIB)/gtk-2.0 \
	-L$(GTK_LIB)/pango \
	-L$(GTK_LIB)/libglade \
	-llibgthread-2.0 \
	-lintl \
	-liconv \
	-lm \
	-lgtk-win32-2.0 \
	-lgobject-2.0 \
	-llibglade-2.0 \
	-llibgdk_pixbuf-2.0 \
	-lpango-1.0 \
	-L/usr/lib/w32api \
	-lgdi32 \
	-llibgdk-win32-2.0 \
	$(ASF_LIB)/libasf_metadata.a \
	$(ASF_LIB)/asf_meta.a \
	$(ASF_LIB)/libasf_raster.a \
	$(ASF_LIB)/libasf_proj.a \
	$(ASF_LIB)/asf.a \
	$(GSL_LIBS) \
	$(PROJ_LIBS) \
	-lglib-2.0 \
	-lcomdlg32

CFLAGS = -D_REENTRANT -mms-bitfields -Dwin32 -g -Wall

$(TARGET).exe: $(TARGET).c
	$(GCC) -o $(TARGET).exe $(SRCS) $(INCLUDE) $(LIB) $(CFLAGS)
	mv $(TARGET).exe $(BINDIR)
	cp $(TARGET).glade $(SHAREDIR)
#	cp $(TARGET).ico $(BINDIR)
	cp mdv.ico $(SHAREDIR)

clean:
	rm -f $(TARGET).exe core.*

### End Windows Section

else

### Begin Unix Section

CFLAGS += \
	$(GTK_CFLAGS) \
	$(shell /usr/bin/pkg-config --cflags libglade-2.0) \
	$(GSL_CFLAGS) \
	$(PROJ_CFLAGS) \
	$(JPEG_CFLAGS)

LDLIBS = \
	$(LIBDIR)/libasf_metadata.a \
	$(LIBDIR)/asf_meta.a \
	$(LIBDIR)/libasf_raster.a \
	$(LIBDIR)/libasf_proj.a \
	$(LIBDIR)/asf.a \
	$(GSL_LIBS) \
	$(PROJ_LIBS) \
	$(JPEG_LIBS) \
	$(GTK_LIBS) \
	$(shell /usr/bin/pkg-config --libs libglade-2.0) \
	$(shell /usr/bin/pkg-config --libs gthread-2.0) \
	-lm

ifeq ($(SYS),darwin)
LDFLAGS = -dynamic
else
LDFLAGS = -Wl,--export-dynamic
endif

OBJS = $(patsubst %.c, %.o, $(SRCS))

default: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS) $(LDFLAGS)
	mv $(TARGET) $(BINDIR)
	cp $(TARGET).glade $(SHAREDIR)
#	cp $(TARGET).ico $(SHAREDIR)
	cp mdv.ico $(SHAREDIR)

%.c: %.h

clean:
	rm -f *.o $(BINDIR)/$(TARGET)* $(TARGET).txt \
	    $(SHAREDIR)/$(TARGET)*

### End Unix Section

endif

