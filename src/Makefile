TARGET:=SpeedTestC

OBJ:=Speedtest.o SpeedtestConfig.o SpeedtestServers.o url.o http.o

CFLAGS+= -g -std=c99 -Wall

LIBS:=-lm

CC:=gcc

all:$(TARGET)

%.o:%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET):$(OBJ)
	$(CC) -o $@ $(OBJ) $(LIBS)

clean:
	rm -f *.o $(TARGET)
