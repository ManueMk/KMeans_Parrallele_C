#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#define NUM_THREADS 4 // Nombre de threads
#define MAX_ITERATIONS 100 // Nombre maximum d'itérations

int num_threads;
pthread_mutex_t lock_reduce;

// Structure pour un point
typedef struct {
    double *coords; // Coordonnées du point
    int cluster; // Cluster auquel appartient le point
} Point;

// Structure pour un cluster
typedef struct {
    double *centroid; // Coordonnées du centroïde
    double *sum_coords; // Somme des coordonnées des points attribués
    int num_points; // Nombre de points dans le cluster
} Cluster;

typedef struct {
    int cluster_index;
} ThreadArgs;

// Structure pour stocker les données nécessaires à chaque thread
typedef struct {
    int start_index;
    int end_index;
} ThreadData;


// Variables globales
Point *points;
Cluster *clusters;
int num_points, num_clusters, num_dims;
pthread_mutex_t lock;

// Fonction pour initialiser les points aléatoirement
void initialize_points() {
    int i, j;
    for (i = 0; i < num_points; i++) {
        points[i].coords = (double *)malloc(num_dims * sizeof(double));
        for (j = 0; j < num_dims; j++) {
            points[i].coords[j] = (double)rand() / RAND_MAX * 10.0; // Valeurs aléatoires entre 0 et 10
        }
        points[i].cluster = -1; // Initialisation du cluster à -1
    }
}

// Fonction pour initialiser les clusters avec des centroids aléatoires
void initialize_clusters() {
    int i, j;
    for (i = 0; i < num_clusters; i++) {
        clusters[i].centroid = (double *)malloc(num_dims * sizeof(double));
        clusters[i].sum_coords = (double *)calloc(num_dims, sizeof(double));
        for (j = 0; j < num_dims; j++) {
            clusters[i].centroid[j] = (double)rand() / RAND_MAX * 10.0; // Valeurs aléatoires entre 0 et 10
        }
        clusters[i].num_points = 0; // Initialise le nombre de points dans le cluster à 0
    }
}

// Fonction pour calculer la distance entre deux points
double calculate_distance(double *p1, double *p2) {
    double distance = 0.0;
    int i;
    for (i = 0; i < num_dims; i++) {
        distance += pow(p1[i] - p2[i], 2);
    }
    return sqrt(distance);
}


// Fonction pour attribuer chaque point au cluster le plus proche
void *assign_points_to_clusters(void *threadid) {
    long tid = (long)threadid;
    int i, j;
    double min_distance, distance;
    int min_cluster;

    // Calcule le début et la fin de la section traitée par le thread
    //int start = tid * (num_points / num_threads);
    //int end = (tid + 1) * (num_points / num_threads);

    // Pour chaque point attribue au cluster le plus proche
    for (i = tid; i < num_points; i+=num_threads) {
        min_distance = calculate_distance(points[i].coords, clusters[0].centroid);
        min_cluster = 0;
        for (j = 1; j < num_clusters; j++) {
            distance = calculate_distance(points[i].coords, clusters[j].centroid);
            if (distance < min_distance) {
                min_distance = distance;
                min_cluster = j;
            }
        }
        pthread_mutex_lock(&lock);
        points[i].cluster = min_cluster;
        clusters[min_cluster].num_points++;
        // Ajoute les coordonnées du point au cluster
        for (j = 0; j < num_dims; j++) {
            clusters[min_cluster].sum_coords[j] += points[i].coords[j];
        }
        pthread_mutex_unlock(&lock);
    }
    pthread_exit(NULL);
}


// Fonction pour recalculer les centroids des clusters
void calculate_centroids() {
    int i, j;
    for (i = 0; i < num_clusters; i++) {
        // Met à jour les coordonnées du centroïde
        for (j = 0; j < num_dims; j++) {
            clusters[i].centroid[j] = clusters[i].sum_coords[j] / clusters[i].num_points;
        }
        // Réinitialise la somme des coordonnées pour le prochain calcul
        for (j = 0; j < num_dims; j++) {
            clusters[i].sum_coords[j] = 0.0;
        }
        // Réinitialise le nombre de points dans le cluster
        clusters[i].num_points = 0;
    }
}


// Fonction principale pour le calcul des clusters
void kmeans() {
    int iter;
    pthread_t threads[num_threads];
    for (iter = 0; iter < MAX_ITERATIONS; iter++) {
        for (long i = 0; i < num_threads; i++) {
            pthread_create(&threads[i], NULL, assign_points_to_clusters, (void *)i);
        }
        for (long i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        calculate_centroids();
    }
}


// Fonction pour attribuer chaque point au cluster le plus proche (map)
void *map(void *threadid) {
    long tid = (long)threadid;
    int i, j;
    double min_distance, distance;
    int min_cluster;

    // Calcule le début et la fin de la section traitée par le thread
    //int start = tid * (num_points / num_threads);
    //int end = (tid + 1) * (num_points / num_threads);

    // Pour chaque point attribue au cluster le plus proche
    for (i = tid; i < num_points; i+=num_threads) {
        min_distance = calculate_distance(points[i].coords, clusters[0].centroid);
        min_cluster = 0;
        for (j = 1; j < num_clusters; j++) {
            distance = calculate_distance(points[i].coords, clusters[j].centroid);
            if (distance < min_distance) {
                min_distance = distance;
                min_cluster = j;
            }
        }
        pthread_mutex_lock(&lock);
        points[i].cluster = min_cluster;
        clusters[min_cluster].num_points++;
        // Ajoute les coordonnées du point au cluster
        for (j = 0; j < num_dims; j++) {
            clusters[min_cluster].sum_coords[j] += points[i].coords[j];
        }
        pthread_mutex_unlock(&lock);
    }
    pthread_exit(NULL);
}


// Fonction Reduce exécutée par chaque thread

void* reduce(void* arg) {
    int thread_index = *((int*)arg);
    int clusters_per_thread = (num_clusters + num_threads - 1) / num_threads;
    int start_cluster = thread_index * clusters_per_thread;
    int end_cluster = (thread_index + 1) * clusters_per_thread;
    if (end_cluster > num_clusters) {
        end_cluster = num_clusters;
    }

    double local_sum_coords[num_dims];

    for (int i = start_cluster; i < end_cluster; i++) {
        for (int j = 0; j < num_dims; j++) {
            local_sum_coords[j] += clusters[i].sum_coords[j];
        }
        clusters[i].num_points = 0;
    }

    pthread_mutex_lock(&lock_reduce);
    for (int i = start_cluster; i < end_cluster; i++) {
        for (int j = 0; j < num_dims; j++) {
            clusters[i].centroid[j] = local_sum_coords[j] / clusters[i].num_points;
            clusters[i].sum_coords[j] = 0.0;
        }
        clusters[i].num_points = 0;
    }
    pthread_mutex_unlock(&lock_reduce);

    pthread_exit(NULL);
}


// Fonction pour afficher les clusters et les points
void print_clusters_and_points() {
    int i, j;
    printf("Clusters:\n");
    for (i = 0; i < num_clusters; i++) {
        printf("Cluster %d: [", i);
        for (j = 0; j < num_dims; j++) {
            printf("%.2f ", clusters[i].centroid[j]);
        }
        printf("]\n");
    }

    printf("\nPoints:\n");
    for (i = 0; i < num_points; i++) {
        printf("Point %d (Cluster %d): [", i, points[i].cluster);
        for (j = 0; j < num_dims; j++) {
            printf("%.2f ", points[i].coords[j]);
        }
        printf("]\n");
    }
}


// Fonction principale pour le calcul des clusters
void kmeans_map() {
    int iter;
    //pthread_t threads[num_clusters];
    ThreadArgs threadArgs[num_clusters];
    pthread_t threads[num_threads];
    for (iter = 0; iter < MAX_ITERATIONS; iter++) {
        for (long i = 0; i < num_threads; i++) {
            pthread_create(&threads[i], NULL, map, (void *)i);
        }
        for (long i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        
        // Création des threads pour la mise à jour des centroids
        pthread_t reduceThreads[num_clusters];
        for (int i = 0; i < num_clusters; i++) {
            int cluster_index = i;
            pthread_create(&reduceThreads[i], NULL, reduce, (void*)&cluster_index);
        }

        // Attente de la fin de tous les threads de mise à jour des centroids
        for (int i = 0; i < num_clusters; i++) {
            pthread_join(reduceThreads[i], NULL);
        }
        
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <num_points> <num_clusters> <num_dims> <num_threads>\n", argv[0]);
        return 1;
    }

    num_points = atoi(argv[1]);
    num_clusters = atoi(argv[2]);
    num_dims = atoi(argv[3]);
    num_threads = atoi(argv[4]);

    points = (Point *)malloc(num_points * sizeof(Point));
    clusters = (Cluster *)malloc(num_clusters * sizeof(Cluster));

    initialize_points();
    initialize_clusters();

    clock_t start_time, end_time;
    start_time = clock();

    kmeans(); // Appel de la fonction kmeans

    // Calcul et affichage du temps d'exécution
    end_time = clock();

    // Afficher les clusters et les points
    print_clusters_and_points();

    double execution_time_parrallel = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Time_parallel: %f seconds\n", execution_time_parrallel);

    
    start_time = clock();

    kmeans_map(); // Appel de la fonction kmeans

    // Calcul et affichage du temps d'exécution
    end_time = clock();

    

    double execution_time_mapreduce = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Time_mapreduce: %f seconds\n", execution_time_mapreduce);

    // Libérer la mémoire
    for (int i = 0; i < num_points; i++) {
        free(points[i].coords);
    }
    free(points);
    for (int i = 0; i < num_clusters; i++) {
        free(clusters[i].centroid);
        free(clusters[i].sum_coords);
    }
    free(clusters);

    return 0;
}
