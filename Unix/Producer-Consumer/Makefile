CC:=gcc
CFLAGS:=-Wall -g -std=c99
LIBS:=-pthread

targets = app
all: $(targets)

app: main.o Makefile
	$(CC) $(CFLAGS) $(LIBS) $(filter %.o,$^) -o $@

clean:
	@rm *.o *~ $(targets) -f

run:
	./app numbers1.txt numbers2.txt

.PHONY: clean