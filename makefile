kmeans_par:kmeans_par.c
	gcc -o kmeans_par kmeans_par.c -pthread -lm

run:kmeans_par
	./kmeans_par 20000 5 5 4 3
	

clean:
	rm kmeans_par
