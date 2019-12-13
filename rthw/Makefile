TARGETS = rtest dig7
all: $(TARGETS)

rtest: rtest.c
	cc -o $@ $^ -lrt -pthread

dig7: dig7.c
	cc -o $@ $^ -lrt

clean:
	rm -f $(TARGETS)
