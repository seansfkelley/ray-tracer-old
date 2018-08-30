# Use -O1 for normal compilation, -O9 for major test builds.

PNGLIBS=-lpng -lpngwriter -lfreetype
RANDOMCPPDIR=randomc

CXX=g++
CPPFLAGS=-g `freetype-config --cflags` -Wall -O0
LIBS=-L/usr/local/lib $(PNGLIBS) -lboost_thread -lboost_serialization -lboost_system -lz
NAME=rt
OBJ=kdtree.o photonmap.o light.o shapes.o raytracer.o localworkerthread.o networkworkerthread.o processinput.o client.o server.o zlibstring.o main.o $(RANDOMCPPDIR)/mersenne.o $(RANDOMCPPDIR)/mother.o $(RANDOMCPPDIR)/sfmt.o

$(NAME): $(OBJ)
	$(CXX) $(CPPFLAGS) $(OBJ) -o $(NAME) $(LIBS)

.c.o:
	$(CXX) $(CPPFLAGS) -c $<  -o $@

clean:
	rm *.o

cleanall:
	rm *.o $(RANDOMCPPDIR)/*.o