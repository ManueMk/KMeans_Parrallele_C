produit_matriciel:Produit_matriciel.c 
	gcc -o produit_matriciel Produit_matriciel.c -g -O0 -lm -lpthread -fopenmp 
	
run_produit_scalaire:produit_scalaire
	./produit_matriciel 20 10 25 2

clean:
	rm  produit_matriciel

Resultats.csv issu de l'execution sur toi 3 threads: contient le nombre de lignes de la matrice resultat, le temps sequentiel, temps par bloc, temps par modulo

Resultats2.csv issu de l'execution sur toi 2 threads: contient le nombre de lignes de la matrice resultat, le temps sequentiel, temps par bloc, temps par modulo

plot_script.gp: script gnuplot pour la comparaison sur 3 threads et affiche le resultat dans Comparaison_3_threads
				gnuplot plot_script.gp

plot_script2.gp: script gnuplot pour la comparaison sur 2 threads et affiche le resultat dans Comparaison_2_threads
				gnuplot plot_script2.gp 