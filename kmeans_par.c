#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define MAX_ITERATIONS 100

typedef struct {
    double *coordinates;
} Point;

typedef struct {
    double *coordinates;
} Centroid;

typedef struct {
    Point *points;
    int start;
    int end;
    Centroid *centroids;
    int *clusters;
    int k;
    int dimensions;
} ThreadData;


double calculate_distance(Point point, Centroid centroid, int dimensions) {
    double sum_squares = 0.0;
    for (int i = 0; i < dimensions; i++) {
        double diff = point.coordinates[i] - centroid.coordinates[i];
        sum_squares += diff * diff;
    }
    return sqrt(sum_squares);
}


int assign_cluster(Point point, Centroid centroids[], int k, int dimensions) {
    int cluster = 0;
    double min_distance = calculate_distance(point, centroids[0], dimensions);

    for (int i = 1; i < k; i++) {
        double distance = calculate_distance(point, centroids[i], dimensions);
        if (distance < min_distance) {
            min_distance = distance;
            cluster = i;
        }
    }

    return cluster;
}


void *assign_clusters_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    for (int i = data->start; i < data->end; i++) {
        int cluster = assign_cluster(data->points[i], data->centroids, data->k, data->dimensions);
        if (cluster != data->clusters[i]) {
            data->clusters[i] = cluster;
        }
    }

    pthread_exit(NULL);
}


void update_centroids(Point points[], int clusters[], Centroid centroids[], int k, int n, int dimensions) {
    int *cluster_sizes = (int *)malloc(k * sizeof(int));
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < dimensions; j++) {
            centroids[i].coordinates[j] = 0.0;
        }
        cluster_sizes[i] = 0;
    }

    for (int i = 0; i < n; i++) {
        int cluster_id = clusters[i];
        for (int j = 0; j < dimensions; j++) {
            centroids[cluster_id].coordinates[j] += points[i].coordinates[j];
        }
        cluster_sizes[cluster_id]++;
    }

    for (int i = 0; i < k; i++) {
        if (cluster_sizes[i] > 0) {
            for (int j = 0; j < dimensions; j++) {
                centroids[i].coordinates[j] /= cluster_sizes[i];
            }
        }
    }

    free(cluster_sizes);
}


void map(Point points[], int n, int dimensions, Centroid centroids[], int k, int num_threads, int clusters[]) {
    // Création des threads
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];

    // Division des points entre les threads
    int chunk_size = n / num_threads;
    int remainder = n % num_threads;
    int start = 0;
    for (int i = 0; i < num_threads; i++) {
        int end = start + chunk_size;
        if (remainder > 0) {
            end++;
            remainder--;
        }

        // Définition des données pour le thread actuel
        thread_data[i].points = points;
        thread_data[i].start = start;
        thread_data[i].end = end;
        thread_data[i].centroids = centroids;
        thread_data[i].clusters = clusters;
        thread_data[i].k = k;
        thread_data[i].dimensions = dimensions;

        // Création du thread
        pthread_create(&threads[i], NULL, assign_clusters_thread, &thread_data[i]);

        start = end;
    }

    // Attendre la fin de tous les threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}


void reduce(Point points[], int n, int dimensions, Centroid centroids[], int k, int clusters[]) {
    int *cluster_sizes = (int *)calloc(k, sizeof(int)); // Initialisation à zéro

    // Somme des coordonnées des points assignés à chaque cluster
    for (int i = 0; i < n; i++) {
        int cluster_id = clusters[i];
        cluster_sizes[cluster_id]++;
        for (int j = 0; j < dimensions; j++) {
            centroids[cluster_id].coordinates[j] += points[i].coordinates[j];
        }
    }

    // Mise à jour des centroids en calculant la moyenne
    for (int i = 0; i < k; i++) {
        if (cluster_sizes[i] > 0) {
            for (int j = 0; j < dimensions; j++) {
                centroids[i].coordinates[j] /= cluster_sizes[i];
            }
        }
    }

    // Libération de la mémoire allouée pour le comptage des points par cluster
    free(cluster_sizes);
}


void kmeans_seq(Point points[], int n, int dimensions, int k, int max_iterations) {
    Centroid *centroids = (Centroid *)malloc(k * sizeof(Centroid));
    int *clusters = (int *)malloc(n * sizeof(int));

   

    clock_t start_time, end_time;
    start_time = clock();

    int iterations = 0;
    int has_changed = 1;

    while (has_changed && iterations < max_iterations) {
        has_changed = 0;

        // Attribution des points aux clusters
        for (int i = 0; i < n; i++) {
            int cluster = assign_cluster(points[i], centroids, k, dimensions);
            if (cluster != clusters[i]) {
                clusters[i] = cluster;
                has_changed = 1;
            }
        }

        // Mise à jour des centroids
        update_centroids(points, clusters, centroids, k, n, dimensions);

        iterations++;
    }

    end_time = clock();

    
    // Affichage des résultats
    printf("Results after %d iterations:\n", iterations);
    for (int i = 0; i < n; i++) {
        printf("Point (");
        for (int j = 0; j < dimensions; j++) {
            printf("%.2f", points[i].coordinates[j]);
            if (j < dimensions - 1) {
                printf(", ");
            }
        }
        printf(") in Cluster %d\n", clusters[i]);
    }

    // Calcul et affichage du temps d'exécution
    double execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Time_seq: %f seconds\n", execution_time);


    // Libération de la mémoire
    for (int i = 0; i < k; i++) {
        free(centroids[i].coordinates);
    }
    free(centroids);
    free(clusters);
}


void kmeans_par(Point points[], int n, int dimensions, int k, int max_iterations, int num_threads) {
    Centroid *centroids = (Centroid *)malloc(k * sizeof(Centroid));
    int *clusters = (int *)malloc(n * sizeof(int));

    
    clock_t start_time, end_time;
    start_time = clock();

    int iterations = 0;
    int has_changed = 1;

    while (has_changed && iterations < max_iterations) {
        has_changed = 0;

        // Attribution des points aux clusters (parallélisé)
        pthread_t threads[num_threads];
        ThreadData thread_data[num_threads];

        int chunk_size = n / num_threads;
        int remainder = n % num_threads;

        int start = 0;
        for (int i = 0; i < num_threads; i++) {
            int end = start + chunk_size;
            if (remainder > 0) {
                end++;
                remainder--;
            }

            thread_data[i].points = points;
            thread_data[i].start = start;
            thread_data[i].end = end;
            thread_data[i].centroids = centroids;
            thread_data[i].clusters = clusters;
            thread_data[i].k = k;
            thread_data[i].dimensions = dimensions;

            pthread_create(&threads[i], NULL, assign_clusters_thread, &thread_data[i]);

            start = end;
        }

        // Attendre la fin de tous les threads
        for (int i = 0; i < num_threads; i++) {
           pthread_join(threads[i], NULL);
        }

        // Mise à jour des centroids
        update_centroids(points, clusters, centroids, k, n, dimensions);

        // Vérification si les clusters ont changé
        for (int i = 0; i < n; i++) {
            int new_cluster = assign_cluster(points[i], centroids, k, dimensions);
            if (new_cluster != clusters[i]) {
                clusters[i] = new_cluster;
                has_changed = 1;
            }
        }

        iterations++;
    }

    end_time = clock();

   

   
     // Calcul et affichage du temps d'exécution
    double execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Time_par: %f seconds\n", execution_time);

    // Libération de la mémoire
    for (int i = 0; i < k; i++) {
        free(centroids[i].coordinates);
    }
    free(centroids);
    free(clusters);
}


void kmeans_map(Point points[], int n, int dimensions, int k, int max_iterations, int num_threads) {
    Centroid *centroids = (Centroid *)malloc(k * sizeof(Centroid));
    int *clusters = (int *)malloc(n * sizeof(int));

    

    clock_t start_time, end_time;
    start_time = clock();

    int iterations = 0;
    int has_changed = 1;

    while (has_changed && iterations < max_iterations) {
        has_changed = 0;

        // Attribution des points aux clusters (MapReduce)
        map(points, n, dimensions, centroids, k, num_threads, clusters);

        // Mise à jour des centroids (MapReduce)
        reduce(points, n, dimensions, centroids, k, clusters);

        // Vérification si les clusters ont changé
        for (int i = 0; i < n; i++) {
            int new_cluster = assign_cluster(points[i], centroids, k, dimensions);
            if (new_cluster != clusters[i]) {
                clusters[i] = new_cluster;
                has_changed = 1;
            }
        }

        iterations++;
    }

    end_time = clock();

    // Calcul et affichage du temps d'exécution
    double execution_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Time_map: %f seconds\n", execution_time);

    // Libération de la mémoire
    for (int i = 0; i < k; i++) {
        free(centroids[i].coordinates);
    }
    free(centroids);
    free(clusters);
}


int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s <num_points> <dimensions> <num_clusters> <num_iterations> <num_threads>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]); // Nombre de points
    int dimensions = atoi(argv[2]); // Nombre de dimensions
    int k = atoi(argv[3]); // Nombre de clusters
    int max_iterations = atoi(argv[4]); // Nombre maximal d'itérations
    int num_threads = atoi(argv[5]); // Nombre de threads

    // Génération aléatoire des points
    Point *points = (Point *)malloc(n * sizeof(Point));
    for (int i = 0; i < n; i++) {
        points[i].coordinates = (double *)malloc(dimensions * sizeof(double));
        for (int j = 0; j < dimensions; j++) {
            points[i].coordinates[j] = (double)rand() / RAND_MAX * 10.0; // Valeurs aléatoires entre 0 et 10
        }
    }

    

    // Exécution de l'algorithme K-means
    kmeans_seq(points, n, dimensions, k, max_iterations);


    kmeans_par(points, n, dimensions, k, max_iterations, num_threads);


    kmeans_map(points, n, dimensions, k, max_iterations, num_threads);

    // Libération de la mémoire
    for (int i = 0; i < n; i++) {
        free(points[i].coordinates);
    }
    free(points);

    return 0;
}
