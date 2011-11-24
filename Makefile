VERSION = 0.1

# CUSTOMIZATION
ArduinoSimDir     :=   # Arduino APIs compilable under Linux       
ArduinoUserLibDir :=   # Your sketchbook folder


Hfiles := $(wildcard *.h) $(wildcard $(ArduinoSimDir)/avr/*.h) $(wildcard $(ArduinoSimDir)/*.h) $(wildcard $(ArduinoSimDir)/*/*.h)
Cfiles := $(wildcard *.cpp) $(wildcard *.c) $(wildcard *.pde)
Ofiles := $(patsubst %.pde,%.o,$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(Cfiles))))

PdeFile := $(wildcard *.pde)

SimBin  :=  $(patsubst %.pde,%,$(PdeFile))


ArduinoLibDir := $(ArduinoSimDir)/libraries

ArduinoLibsInc := $(addprefix -I,$(wildcard $(ArduinoLibDir)/*))


CF := -c -DSIM -DSIM__AVR_ATmega328P__ -g -O0 -I. -I$(ArduinoUserLibDir)/lightuino5 -I$(ArduinoSimDir) $(ArduinoLibsInc)
LF := -lrt

default: $(SimBin)

%: %.pde $(Ofiles) $(ArduinoSimDir)/libArduinoSim.a
	g++ $(LF) -o $@ $(Ofiles) $(ArduinoSimDir)/libArduinoSim.a

%.o: %.cpp $(Hfiles)
	g++ $(CF) -o $@ $<

%.o: %.c $(Hfiles)
	g++ $(CF) -o $@ $<

%.o: %.pde $(Hfiles)
	g++ $(CF) -x c++ -o $@ $<
