ledc: client.c
	gcc client.c -o ledc

install: ledc
	cp ledc /usr/local/bin

clean:
	rm ledc
