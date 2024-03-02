CC = gcc

all:ask1

ask1: src/ask1.c
	$(CC) src/ask1.c -o ask1 -g -pthread

clean:
	@ -rm -f ask1 pagerank.csv
