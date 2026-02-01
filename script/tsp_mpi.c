/* tsp_mpi.c - Master Version
   Features:
   1. Reads berlin52.tsp dynamically.
   2. True Strong Scaling (Total Replicas = 24).
   3. Precomputed Distance Matrix (O(1) lookup).
   4. Deadlock-free MPI Swaps.
   5. Saves 'my_route.txt' for plotting.
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

// --- LAB 2 PARAMETERS [cite: 87-90] ---
#define TOTAL_GLOBAL_REPLICAS 24 
#define INITIAL_TEMP 150.0
#define TEMP_DECAY 0.80      
#define SWAP_INTERVAL 52      
#define TOTAL_SWAP_ATTEMPTS 5000 
#define TOTAL_STEPS (SWAP_INTERVAL * TOTAL_SWAP_ATTEMPTS)

typedef struct { double x, y; } City;

// Globals
int N_CITIES = 0;
City *cities = NULL;
double *dist_matrix = NULL; 

// --- HELPER FUNCTIONS ---

// Read TSPLIB format (Rank 0 only)
int read_tsp_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;

    char line[256];
    int reading_coords = 0;
    int idx = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "DIMENSION", 9) == 0) {
            char *p = strchr(line, ':');
            if (p) N_CITIES = atoi(p + 1);
        }
        else if (strncmp(line, "NODE_COORD_SECTION", 18) == 0) {
            reading_coords = 1;
            if (N_CITIES == 0) N_CITIES = 52; 
            cities = (City*)malloc(N_CITIES * sizeof(City));
            continue;
        }
        else if (strncmp(line, "EOF", 3) == 0) break;

        if (reading_coords && idx < N_CITIES) {
            int id; double x, y;
            if (sscanf(line, "%d %lf %lf", &id, &x, &y) == 3) {
                cities[idx].x = x;
                cities[idx].y = y;
                idx++;
            }
        }
    }
    fclose(f);
    return (idx == N_CITIES);
}

void init_dist_matrix() {
    dist_matrix = (double*)malloc(N_CITIES * N_CITIES * sizeof(double));
    for(int i=0; i<N_CITIES; i++) {
        for(int j=0; j<N_CITIES; j++) {
            double dx = cities[i].x - cities[j].x;
            double dy = cities[i].y - cities[j].y;
            dist_matrix[i * N_CITIES + j] = sqrt(dx*dx + dy*dy);
        }
    }
}

// Optimization: static inline prevents linker errors and is fast
static inline double get_dist(int i, int j) {
    return dist_matrix[i * N_CITIES + j];
}

double calc_route_len(int *route) {
    double d = 0.0;
    for (int i = 0; i < N_CITIES; i++) {
        d += get_dist(route[i], route[(i + 1) % N_CITIES]);
    }
    return d;
}

// --- CORE ALGORITHM ---

void metropolis_step(int *route, double T) {
    int a = rand() % N_CITIES;
    int b = rand() % N_CITIES;
    if (a == b) return;
    if (a > b) { int t = a; a = b; b = t; }

    int cA = route[a];
    int cA_next = route[(a+1)%N_CITIES];
    int cB = route[b];
    int cB_next = route[(b+1)%N_CITIES];
    
    // 2-opt Delta Calculation using Lookup Matrix
    double delta = (get_dist(cA, cB) + get_dist(cA_next, cB_next)) - 
                   (get_dist(cA, cA_next) + get_dist(cB, cB_next));

    if (delta < 0 || (T > 1e-9 && ((double)rand() / RAND_MAX) < exp(-delta / T))) {
        int l = a + 1, h = b;
        while (l < h) {
            int temp = route[l]; route[l] = route[h]; route[h] = temp;
            l++; h--;
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // --- 1. FILE READING (Rank 0) ---
    if (rank == 0) {
        const char *fname = (argc > 1) ? argv[1] : "berlin52.tsp";
        if (!read_tsp_file(fname)) {
            fprintf(stderr, "Error: Could not read %s\n", fname);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    
    // Broadcast Map Data
    MPI_Bcast(&N_CITIES, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0) cities = (City*)malloc(N_CITIES * sizeof(City));
    MPI_Bcast(cities, N_CITIES * sizeof(City), MPI_BYTE, 0, MPI_COMM_WORLD);
    
    // Precompute Distances Locally
    init_dist_matrix();
    srand(time(NULL) + rank * 999); 

    // --- 2. STRONG SCALING SETUP ---
    // Total work is 24 replicas. Split among P processors.
    if (TOTAL_GLOBAL_REPLICAS % size != 0) {
        if (rank==0) printf("Error: 24 Replicas must be divisible by %d processors.\n", size);
        MPI_Finalize(); return 1;
    }
    
    int my_replicas = TOTAL_GLOBAL_REPLICAS / size;
    int my_start_idx = rank * my_replicas;
    
    // Allocate Local Replicas
    int *routes_flat = (int*)malloc(my_replicas * N_CITIES * sizeof(int));
    int **routes = (int**)malloc(my_replicas * sizeof(int*));
    double *temps = (double*)malloc(my_replicas * sizeof(double));
    
    // Track Best Route Locally
    int *my_best_route = (int*)malloc(N_CITIES * sizeof(int));
    double my_best_dist = 1e15;

    for (int i = 0; i < my_replicas; i++) {
        routes[i] = &routes_flat[i * N_CITIES];
        for (int c = 0; c < N_CITIES; c++) routes[i][c] = c; 
        
        // Random Shuffle
        for(int k=0; k<N_CITIES; k++) {
            int r = rand()%N_CITIES; 
            int t=routes[i][k]; routes[i][k]=routes[i][r]; routes[i][r]=t;
        }

        int g_idx = my_start_idx + i;
        if (g_idx == TOTAL_GLOBAL_REPLICAS - 1) temps[i] = 0.0;
        else temps[i] = INITIAL_TEMP * pow(TEMP_DECAY, g_idx);
    }

    // --- 3. MAIN LOOP ---
    double start_time = MPI_Wtime();

    for (long step = 0; step < TOTAL_STEPS; step++) {
        // Evolve
        for (int i = 0; i < my_replicas; i++) {
            metropolis_step(routes[i], temps[i]);
            
            // Check if this is a new best
            // (Optimization: Only check occasionally or check delta < 0 in metropolis)
            // For simplicity here, we check after batch.
        }
        
        // Update Local Best
        for(int i=0; i<my_replicas; i++) {
            double d = calc_route_len(routes[i]);
            if (d < my_best_dist) {
                my_best_dist = d;
                memcpy(my_best_route, routes[i], N_CITIES * sizeof(int));
            }
        }

        // Swap Phase
        if (step % SWAP_INTERVAL == 0) {
            
            // A. Internal Swaps (Safe)
            for (int i = 0; i < my_replicas - 1; i++) {
                int g_r = my_start_idx + i;
                if ((step/SWAP_INTERVAL)%2 == 0 && g_r%2 != 0) continue;
                if ((step/SWAP_INTERVAL)%2 != 0 && g_r%2 == 0) continue;

                double e1 = calc_route_len(routes[i]);
                double e2 = calc_route_len(routes[i+1]);
                double delta = (1.0/(temps[i]+1e-9) - 1.0/(temps[i+1]+1e-9)) * (e1 - e2);

                if (((double)rand()/RAND_MAX) < exp(delta)) {
                    int tmp[N_CITIES];
                    memcpy(tmp, routes[i], N_CITIES*sizeof(int));
                    memcpy(routes[i], routes[i+1], N_CITIES*sizeof(int));
                    memcpy(routes[i+1], tmp, N_CITIES*sizeof(int));
                }
            }

            // B. MPI Boundary Swaps (Deadlock Free Logic)
            if (size > 1) {
                // Step 1: Handle LEFT Partner (Receive then Send)
                if (rank > 0) {
                    int left = rank - 1;
                    int left_last_g = (left+1)*my_replicas - 1;
                    int expect = 0;
                    if ((step/SWAP_INTERVAL)%2 == 0 && left_last_g%2 == 0) expect = 1;
                    if ((step/SWAP_INTERVAL)%2 != 0 && left_last_g%2 != 0) expect = 1;

                    if (expect) {
                        double my_E = calc_route_len(routes[0]);
                        double partner_E;
                        MPI_Sendrecv(&my_E, 1, MPI_DOUBLE, left, 0, &partner_E, 1, MPI_DOUBLE, left, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        
                        int do_swap = 0;
                        MPI_Recv(&do_swap, 1, MPI_INT, left, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        
                        if (do_swap) {
                            int buf[N_CITIES];
                            MPI_Sendrecv(routes[0], N_CITIES, MPI_INT, left, 2, buf, N_CITIES, MPI_INT, left, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            memcpy(routes[0], buf, N_CITIES*sizeof(int));
                        }
                    }
                }

                // Step 2: Handle RIGHT Partner (Initiate Logic)
                if (rank < size - 1) {
                    int right = rank + 1;
                    int my_last = my_replicas - 1;
                    int my_last_g = my_start_idx + my_last;
                    
                    int init = 0;
                    if ((step/SWAP_INTERVAL)%2 == 0 && my_last_g%2 == 0) init = 1;
                    if ((step/SWAP_INTERVAL)%2 != 0 && my_last_g%2 != 0) init = 1;

                    if (init) {
                        double my_E = calc_route_len(routes[my_last]);
                        double partner_E;
                        MPI_Sendrecv(&my_E, 1, MPI_DOUBLE, right, 0, &partner_E, 1, MPI_DOUBLE, right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        
                        int do_swap = 0;
                        double my_T = temps[my_last];
                        double partner_T = (my_last_g+1 == TOTAL_GLOBAL_REPLICAS-1) ? 0.0 : INITIAL_TEMP * pow(TEMP_DECAY, my_last_g+1);
                        double delta = (1.0/(my_T+1e-9) - 1.0/(partner_T+1e-9)) * (my_E - partner_E);
                        
                        if (((double)rand()/RAND_MAX) < exp(delta)) do_swap = 1;

                        MPI_Send(&do_swap, 1, MPI_INT, right, 1, MPI_COMM_WORLD);
                        
                        if (do_swap) {
                            int buf[N_CITIES];
                            MPI_Sendrecv(routes[my_last], N_CITIES, MPI_INT, right, 2, buf, N_CITIES, MPI_INT, right, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            memcpy(routes[my_last], buf, N_CITIES*sizeof(int));
                        }
                    }
                }
            }
        }
    }

    // --- 4. FINALIZE & SAVE ---
    double end_time = MPI_Wtime();
    
    // Find Global Best
    struct { double val; int rank; } loc_data = {my_best_dist, rank}, glob_data;
    MPI_Allreduce(&loc_data, &glob_data, 1, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);

    // Winner Rank sends the Route to Rank 0
    int *winner_route = (int*)malloc(N_CITIES * sizeof(int));
    if (rank == glob_data.rank) memcpy(winner_route, my_best_route, N_CITIES * sizeof(int));
    
    // Broadcast winning route to Rank 0 (and everyone else)
    MPI_Bcast(winner_route, N_CITIES, MPI_INT, glob_data.rank, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Simulation Time: %.4f s | Global Best: %.4f (Found by Rank %d)\n", 
               end_time - start_time, glob_data.val, glob_data.rank);
        
        FILE *f = fopen("solution.txt", "w");
        fprintf(f, "%.4f\n", glob_data.val);
        fclose(f);
        
        FILE *fr = fopen("my_route.txt", "w");
        for(int i=0; i<N_CITIES; i++) fprintf(fr, "%d\n", winner_route[i]);
        fclose(fr);
    }

    free(routes_flat); free(routes); free(temps); free(cities); free(dist_matrix);
    free(my_best_route); free(winner_route);
    MPI_Finalize();
    return 0;
}
