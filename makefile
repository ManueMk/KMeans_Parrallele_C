map_kmeans:map_kmeans.c
	gcc -o map_kmeans map_kmeans.c -pthread -lm

run:map_kmeans
	./map_kmeans 5000 10 6 4
	

clean:
	rm map_kmeans
