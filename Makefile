INCLUDES += -I/usr/include/spa-0.2
INCLUDES += -I/usr/include/pipewire-0.3

LIBS += -lpipewire-0.3
LIBS += -lopus
LIBS += -lc
LIBS += -lm
LIBS += -lortp

# all: clean build/sendt build/pipe-rx
all: clean build/rtp_rec build/sendt

build/rtp_rec: src/rtp_rec.c
	@mkdir -p build
	$(CC) $(INCLUDES) -o build/rtp_rec.o -c $<
	$(CC) build/rtp_rec.o $(LIBS) -g -o $@

build/sendt: src/send-tone.c
	@mkdir -p build
	$(CC) $(INCLUDES) -o build/send-tone.o -c $<
	$(CC) build/send-tone.o $(LIBS) -g -o $@

build/pipe-rx: src/pipe-rx.c
	@mkdir -p build
	$(CC) $(INCLUDES) -o build/pipe-rx.o -c $<
	$(CC) build/pipe-rx.o $(LIBS) -o $@

install: build/sendt build/pipe-rx
	@sudo cp build/sendt /usr/bin/sendt
	@sudo cp build/pipe-rx /usr/bin/pipe-rx

clean:
	@rm -rf build
