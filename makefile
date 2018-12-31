# directories
GTKBASE= F:/Appli/msys64/mingw32

# listes

C_SRC =
CPP_SRC = mi_parse.cpp spawn_w.cpp futf8.cpp transcript.cpp gui.cpp
EXE = gui.exe

OBJS = $(C_SRC:.c=.o) $(CPP_SRC:.cpp=.o)

# maintenir les libs et includes dans l'ordre alphabetique SVP

# LIBS= `pkg-config --libs gtk+-2.0`
LIBS= -L$(GTKBASE)/lib \
-latk-1.0 \
-lcairo \
-lgdk-win32-2.0 \
-lgdk_pixbuf-2.0 \
-lglib-2.0 \
-lgmodule-2.0 \
-lgobject-2.0 \
-lgtk-win32-2.0 \
-lpango-1.0 \
-lpangocairo-1.0 \
-lpangowin32-1.0 \
# -mwindows
# enlever -mwindows pour avoir la console stdout

# INCS= `pkg-config --cflags gtk+-2.0` -mms-bitfields
INCS= -mms-bitfields \
-I$(GTKBASE)/include/atk-1.0 \
-I$(GTKBASE)/include/cairo \
-I$(GTKBASE)/include/gdk-pixbuf-2.0 \
-I$(GTKBASE)/include/glib-2.0 \
-I$(GTKBASE)/include/gtk-2.0 \
-I$(GTKBASE)/include/pango-1.0 \
-I$(GTKBASE)/lib/glib-2.0/include \
-I$(GTKBASE)/lib/gtk-2.0/include \


# linkage
$(EXE) : $(OBJS)
	g++ -o $(EXE) $(OBJS) $(LIBS)

# compilage
.c.o :
	gcc -Wall $(INCS) -c $<

.cpp.o :
	g++ -Wall $(INCS) -c $<
# other

clean :
	rm *.o

# dependances : 
mi_parse.o : mi_parse.h target.h
spawn_w.o : spawn_w.h
futf8.o : futf8.h
transcript.o : transcript.h futf8.h
gui.o : gui.h mi_parse.h spawn_w.h transcript.h futf8.h target.h
