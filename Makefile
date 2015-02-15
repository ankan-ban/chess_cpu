HEADERS = bb_consts.h chess.h switches.h randoms.h switches.h timer.h 
OBJECTS = bitboard.o magics.o main.o move_gen.o search.o uci_interface.o utils.o eval.o
OBJECTS_ARM = bitboard.a magics.a main.a move_gen.a search.a uci_interface.a utils.a eval.a

default: chess_cpu

%.o: 	%.cpp $(HEADERS)
	g++ -c $< -o $@ -msse4.2 -Ofast -std=c++11 -pthread -flto -march=native

chess_cpu: $(OBJECTS)
	g++ $(OBJECTS) -o $@ -Ofast -std=c++11 -pthread -flto -march=native
	-rm -f $(OBJECTS)

clean:
	-rm -f $(OBJECTS)
	-rm -f $(OBJECTS_ARM)
	-rm -f chess_cpu
	-rm -f chess_arm

%.a: 	%.cpp $(HEADERS)
	arm-linux-gnueabihf-gcc -static -c $< -o $@ -O2 -std=c++11 -pthread -lstdc++ -flto -march=armv7 -mtune=cortex-a15


chess_arm: $(OBJECTS_ARM)
	arm-linux-gnueabihf-gcc -static $(OBJECTS_ARM) -o $@ -O2 -std=c++11 -pthread -lstdc++ -flto  -march=armv7 -mtune=cortex-a15
	-rm -f $(OBJECTS_ARM)

#these compiler flags work: -O2 -std=c++11 -pthread -lstdc++ -flto  -march=armv7 -mtune=cortex-a15
# ideally we would like -Ofast but anything >= O3 produces wrong code? compiler bug ??
