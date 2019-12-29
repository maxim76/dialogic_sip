#callflow: callflow.cpp
#	gcc -m32 -o $@ $^ -I/usr/dialogic/inc -L/usr/dialogic/lib -ldl -lm -lgc
callflow: callflow.o
	gcc -m32 -L/usr/dialogic/lib -ldl -lm -lgc -o $@ $^ 

callflow.o: callflow.cpp
	gcc -m32 -g -Wall -c -I/usr/dialogic/inc -Dlint -o $@ $^

clean:
	rm -f *.o callflow
