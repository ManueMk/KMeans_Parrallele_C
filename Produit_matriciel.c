#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>


int nb_threads = 1;

typedef struct timezone timezone_t;
typedef struct timeval timeval_t;

timeval_t t1, t2;
timezone_t tz;


static struct timeval _t1, _t2;
static struct timezone _tz;
timeval_t t1, t2;
timezone_t tz;

static unsigned long _temps_residuel = 0;
#define top1() gettimeofday(&_t1, &_tz)
#define top2() gettimeofday(&_t2, &_tz)

void init_cpu_time(void)
{
   top1(); top2();
   _temps_residuel = 1000000L * _t2.tv_sec + _t2.tv_usec -
                     (1000000L * _t1.tv_sec + _t1.tv_usec );
}

unsigned long cpu_time(void) /* retourne des microsecondes */
{
   return 1000000L * _t2.tv_sec + _t2.tv_usec -
           (1000000L * _t1.tv_sec + _t1.tv_usec ) - _temps_residuel;
}


// Fonction pour générer une matrice aléatoire
void generer_matrice_aleatoire(int n, int m, int** matrice) {
  // Initialisation de la matrice
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      matrice[i][j] = rand() % 100;
    }
  }
}

// Fonction pour calculer le produit matriciel
int** produit_matriciel(int n, int m, int p, int** A, int** B) {
  // Allocation de la matrice résultat
  int** C = malloc(n * sizeof(int*));
  for (int i = 0; i < n; i++) {
    C[i] = malloc(p * sizeof(int));
  }

  // Calcul du produit matriciel
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < p; j++) {
      for (int k = 0; k < m; k++) {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }

  // Retour de la matrice résultat
  return C;
}


// Fonctions pour calculer le produit matriciel parrallele

pthread_mutex_t mutex;

typedef struct {
    int start;
    int end;
} infos;
// Déclaration des variables
  int n , m , p ;
  int** A, ** B, ** C, ** C2;


//Strategie par blocs
void *produit_matriciel_bloc(void *arg) {
    infos *info = (infos *)arg;

    for (int i = info->start; i < info->end; ++i) {
        for (int j = 0; j < p; ++j) {
            C[i][j] = 0;
            for (int k = 0; k < m; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    pthread_exit(NULL);
}

//strategie par modulo

void *produit_matriciel_modulo(void *arg) {
    int thread_id = *(int *)arg;

    for (int i = thread_id; i < n; i += nb_threads) {
        for (int j = 0; j < p; ++j) {
            C[i][j] = 0;
            for (int k = 0; k < m; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    pthread_exit(NULL);
}

// ajout dans un fichiercsv
void stockerResultatDansCSV(int n, long int time_seq, double time_par_blocs, double time_par_mod) {
    FILE *fichier = fopen("resultats_2.csv", "a"); // Ouvre le fichier en mode "ajout" (append)

    if (fichier == NULL) {
        printf("Impossible d'ouvrir le fichier.");
        return;
    }

   
    fprintf(fichier, "%d, 0.%03ld, %.6f, %.6f\n", n, time_seq, time_par_blocs, time_par_mod);

    fclose(fichier); // Ferme le fichier
}

int main(int argc, char *argv[]) {

    //pthread_t* thread;

	pthread_attr_t attr;
    int rc;
    long t;
	void *status;
    
  

  srand(time(NULL));

  // Déclaration des variables
  int n , m , p ;
  int** A, ** B, ** C, ** C2;

  if(argc < 4)
	{
		printf("Nombre d'argument insuffisant");
		exit(-1);
	}
	else
	{
		n =  atoi(argv[1]);
        m =  atoi(argv[2]);
        p =  atoi(argv[3]);
		nb_threads = atoi(argv[4]);
	}
    srand(time(NULL));
    pthread_t threads[nb_threads];
    infos block_infos[nb_threads];
    int thread_ids[nb_threads];
    struct timeval start, end;

	//thread = (pthread_t*)malloc(sizeof(pthread_t)*nb_threads);

  // Allocation des matrices
  A = malloc(n * sizeof(int*));
  for (int i = 0; i < n; i++) {
    A[i] = malloc(m * sizeof(int));
  }

  B = malloc(m * sizeof(int*));
  for (int i = 0; i < m; i++) {
    B[i] = malloc(p * sizeof(int));
  }

  // Génération des matrices
  generer_matrice_aleatoire(n, m, A);
  generer_matrice_aleatoire(m, p, B);

// ------------------------------------------------------
  // Calcul du produit matriciel sequentiel
  top1();
  C = produit_matriciel(n, m, p, A, B);
  top2();

  unsigned long temps = cpu_time();
  printf("\ntime seq = %ld.%03ldms\n", temps/1000, temps%1000);

 // ---------------------------------------------------------------

// Execution parrallele: Strategie par blocs
   gettimeofday(&start, NULL);

    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < nb_threads; ++i) {
        block_infos[i].start = i * (n/ nb_threads);
        block_infos[i].end = (i + 1) * (n / nb_threads);
        if (i == nb_threads - 1) {
            block_infos[i].end = n;
        }
        pthread_create(&threads[i], NULL, produit_matriciel_bloc, &block_infos[i]);
    }

    for (int i = 0; i < nb_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    printf("time par blocs: %.6f ms\n",
           ((double)((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec))/1000000);
    double time_par_blocs = ((double)((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec))/1000000;
// -----------------------------------------------------------------------------------------------------
    
    // Execution parrallele: strategie modulo
    gettimeofday(&start, NULL);

    for (int i = 0; i < nb_threads; ++i) {
        thread_ids[i] = i%nb_threads;
        pthread_create(&threads[i], NULL, produit_matriciel_modulo, &thread_ids[i]);
    }

    for (int i = 0; i < nb_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    printf("time par modulo: %.6f ms\n",
           ((double)((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec))/1000000);
    double time_par_mod = ((double)((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec))/1000000;

// ---------------------------------------------------------------------------------------------------

    stockerResultatDansCSV(n, temps, time_par_blocs, time_par_mod) ;

     // Affichage de la matrice résultat
     printf("La matrice resultat est:\n");
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < p; j++) {
        printf("%d ", C[i][j]);
        
      }
      printf("\n");
      
    }

    
  

  // Libération de la mémoire
  for (int i = 0; i < n; i++) {
    free(A[i]);
  }
  free(A);

  for (int i = 0; i < m; i++) {
    free(B[i]);
  }
  free(B);

  for (int i = 0; i < n; i++) {
    free(C[i]);
  }
  free(C);

  return 0;
}
