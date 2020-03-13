callserver: callserver.o
	gcc -m32 -L/usr/dialogic/lib -ldl -lm -lgc -o $@ $^ -std=c++0x

callserver.o: callserver.cpp
	gcc -m32 -g -Wall -c -I/usr/dialogic/inc -Dlint -o $@ $^ -std=c++0x

clean:
	rm -f *.o callserver
