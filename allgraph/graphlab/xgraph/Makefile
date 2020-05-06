CROSS =
CXX = $(CROSS)g++
CFLAGS =
LDFLAGS =
X =
LIBS = -lGL -lGLU -lX11

all: xgraph glgraph$(X)

ivecmath.o: ivecmath.c ivecmath.h
	cc -c $<

xgraph.o: xgraph.c ivecmath.h
	cc -c $<

xgraph: xgraph.o ivecmath.o
	cc $^ -lX11 -o $@

xmain.o: xmain.cpp GfxOpenGL.h
	$(CXX) $(CFLAGS) -c $< -o $@

GfxOpenGL.o: GfxOpenGL.cpp GfxOpenGL.h
	$(CXX) $(CFLAGS) -c $< -o $@

glgraph$(X): xmain.o GfxOpenGL.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

mostlyclean:
	rm -f ivecmath.o xgraph.o
	rm -f GfxOpenGL.o xmain.o

clean: mostlyclean
	rm -f xgraph
	rm -f glgraph$(X)
