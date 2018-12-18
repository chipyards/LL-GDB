# makefile pour MinGW

CCOPT = -Wall
C_SRC =
CPP_SRC = mi_parse.cpp appli.cpp
EXE = appli

OBJS = $(C_SRC:.c=.o) $(CPP_SRC:.cpp=.o)

# linkage
$(EXE) : $(OBJS)
	g++ -o $(EXE) $(OBJS)

# compilage
.c.o :
	gcc $(CCOPT) -c $<

.cpp.o :
	g++ $(CCOPT) -c $<
# other

clean :
	rm *.o; rm *.exe

# dependances : 
mi_parse.o : mi_parse.h
appli.o : mi_parse.h
