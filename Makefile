build:
	gcc -Wall -Wextra  main.c -o bmp -lm
run:
	./bmp
clean:
	rm -f bmp
