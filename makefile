all: main

main: main.cpp
	g++ main.cpp -o main -lev

run:
	./main $(port)

clean:
	-rm -f main.o
	-rm -f main