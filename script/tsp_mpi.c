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
#include <errno.h>
#include <getopt.h>

#define DEFAULT_TOTAL_GLOBAL_REPLICAS 24
#define DEFAULT_INITIAL_TEMP 150.0
#define DEFAULT_TEMP_DECAY 0.80
#define DEFAULT_SWAP_INTERVAL 52
#define DEFAULT_TOTAL_SWAP_ATTEMPTS 5000
#define DEFAULT_TOTAL_STEPS (DEFAULT_SWAP_INTERVAL * DEFAULT_TOTAL_SWAP_ATTEMPTS)
#define DEFAULT_TSP_FILE "data/berlin52.tsp"
#define DETERMINISTIC_SEED 1337UL

typedef struct { double x, y; } City;

typedef struct {
    int total_global_replicas;
    long total_steps;
    int swap_interval;
    double initial_temp;
    double temp_decay;
    int deterministic;
    unsigned long seed;
    int seed_set;
    const char *filename;
} SolverConfig;

/* Globals */
int N_CITIES = 0;
City *cities = NULL;
double *dist_matrix = NULL;

static void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS] [TSPLIB_FILE]\n", prog);
    printf("\nOptions:\n");
    printf("  --replicas N         Total number of global replicas (default: %d)\n", DEFAULT_TOTAL_GLOBAL_REPLICAS);
    printf("  --steps N            Total optimization steps (default: %d)\n", DEFAULT_TOTAL_STEPS);
    printf("  --swap-interval N    Swap interval in steps (default: %d)\n", DEFAULT_SWAP_INTERVAL);
    printf("  --initial-temp X     Initial temperature (default: %.1f)\n", DEFAULT_INITIAL_TEMP);
    printf("  --temp-decay X       Temperature decay factor (default: %.2f)\n", DEFAULT_TEMP_DECAY);
    printf("  --seed N             Base random seed (rank offset is added)\n");
    printf("  --deterministic      Use fixed deterministic base seed (%lu)\n", DETERMINISTIC_SEED);
    printf("  --help               Show this help and exit\n");
    printf("\nTSPLIB_FILE defaults to %s\n", DEFAULT_TSP_FILE);
}

static int parse_long_value(const char *text, long *out) {
    char *endptr = NULL;
    errno = 0;
    long val = strtol(text, &endptr, 10);
    if (errno != 0 || endptr == text || *endptr != '\0') return 0;
    *out = val;
    return 1;
}

static int parse_int_value(const char *text, int *out) {
    long val = 0;
    if (!parse_long_value(text, &val)) return 0;
    if (val < 1 || val > 2147483647L) return 0;
    *out = (int)val;
    return 1;
}

static int parse_double_value(const char *text, double *out) {
    char *endptr = NULL;
    errno = 0;
    double val = strtod(text, &endptr);
    if (errno != 0 || endptr == text || *endptr != '\0') return 0;
    *out = val;
    return 1;
}

static int parse_seed_value(const char *text, unsigned long *out) {
    char *endptr = NULL;
    errno = 0;
    unsigned long val = strtoul(text, &endptr, 10);
    if (errno != 0 || endptr == text || *endptr != '\0') return 0;
    *out = val;
    return 1;
}

static int parse_args(int argc, char **argv, SolverConfig *cfg) {
    static struct option long_options[] = {
        {"replicas", required_argument, 0, 1},
        {"steps", required_argument, 0, 2},
        {"swap-interval", required_argument, 0, 3},
        {"initial-temp", required_argument, 0, 4},
        {"temp-decay", required_argument, 0, 5},
        {"seed", required_argument, 0, 6},
        {"deterministic", no_argument, 0, 7},
        {"help", no_argument, 0, 8},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    opterr = 0;
    while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (c) {
            case 1:
                if (!parse_int_value(optarg, &cfg->total_global_replicas)) {
                    fprintf(stderr, "Invalid --replicas value: %s\n", optarg);
                    return -1;
                }
                break;
            case 2:
                if (!parse_long_value(optarg, &cfg->total_steps) || cfg->total_steps <= 0) {
                    fprintf(stderr, "Invalid --steps value: %s\n", optarg);
                    return -1;
                }
                break;
            case 3:
                if (!parse_int_value(optarg, &cfg->swap_interval)) {
                    fprintf(stderr, "Invalid --swap-interval value: %s\n", optarg);
                    return -1;
                }
                break;
            case 4:
                if (!parse_double_value(optarg, &cfg->initial_temp)) {
                    fprintf(stderr, "Invalid --initial-temp value: %s\n", optarg);
                    return -1;
                }
                break;
            case 5:
                if (!parse_double_value(optarg, &cfg->temp_decay)) {
                    fprintf(stderr, "Invalid --temp-decay value: %s\n", optarg);
                    return -1;
                }
                break;
            case 6:
                if (!parse_seed_value(optarg, &cfg->seed)) {
                    fprintf(stderr, "Invalid --seed value: %s\n", optarg);
                    return -1;
                }
                cfg->seed_set = 1;
                break;
            case 7:
                cfg->deterministic = 1;
                break;
            case 8:
                print_usage(argv[0]);
                return 1;
            case '?':
            default:
                fprintf(stderr, "Unknown option. Use --help for usage.\n");
                return -1;
        }
    }

    if (optind < argc) {
        cfg->filename = argv[optind];
    }

    return 0;
}

/* Read TSPLIB format (Rank 0 only) */
static int read_tsp_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: failed to open '%s': %s\n", filename, strerror(errno));
        return 0;
    }

    char line[256];
    int reading_coords = 0;
    int idx = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "DIMENSION", 9) == 0) {
            char *p = strchr(line, ':');
            if (p) N_CITIES = atoi(p + 1);
        } else if (strncmp(line, "NODE_COORD_SECTION", 18) == 0) {
            reading_coords = 1;
            if (N_CITIES == 0) N_CITIES = 52;
            cities = (City *)malloc((size_t)N_CITIES * sizeof(City));
            if (!cities) {
                fprintf(stderr, "Error: failed to allocate city buffer for %d cities.\n", N_CITIES);
                fclose(f);
                return 0;
            }
            continue;
        } else if (strncmp(line, "EOF", 3) == 0) {
            break;
        }

        if (reading_coords && idx < N_CITIES) {
            int id;
            double x, y;
            if (sscanf(line, "%d %lf %lf", &id, &x, &y) == 3) {
                cities[idx].x = x;
                cities[idx].y = y;
                idx++;
            }
        }
    }

    fclose(f);

    if (!cities) {
        fprintf(stderr, "Error: '%s' does not contain NODE_COORD_SECTION.\n", filename);
        return 0;
    }

    if (idx != N_CITIES) {
        fprintf(stderr, "Error: expected %d coordinates in '%s', but read %d.\n", N_CITIES, filename, idx);
        free(cities);
        cities = NULL;
        return 0;
    }

    return 1;
}

static int init_dist_matrix(void) {
    size_t n = (size_t)N_CITIES * (size_t)N_CITIES;
    dist_matrix = (double *)malloc(n * sizeof(double));
    if (!dist_matrix) {
        return 0;
    }

    for (int i = 0; i < N_CITIES; i++) {
        for (int j = 0; j < N_CITIES; j++) {
            double dx = cities[i].x - cities[j].x;
            double dy = cities[i].y - cities[j].y;
            dist_matrix[i * N_CITIES + j] = sqrt(dx * dx + dy * dy);
        }
    }
    return 1;
}

/* Optimization: static inline prevents linker errors and is fast */
static inline double get_dist(int i, int j) {
    return dist_matrix[i * N_CITIES + j];
}

static double calc_route_len(int *route) {
    double d = 0.0;
    for (int i = 0; i < N_CITIES; i++) {
        d += get_dist(route[i], route[(i + 1) % N_CITIES]);
    }
    return d;
}

/* --- CORE ALGORITHM --- */

static void metropolis_step(int *route, double T) {
    int a = rand() % N_CITIES;
    int b = rand() % N_CITIES;
    if (a == b) return;
    if (a > b) {
        int t = a;
        a = b;
        b = t;
    }

    int cA = route[a];
    int cA_next = route[(a + 1) % N_CITIES];
    int cB = route[b];
    int cB_next = route[(b + 1) % N_CITIES];

    /* 2-opt Delta Calculation using Lookup Matrix */
    double delta = (get_dist(cA, cB) + get_dist(cA_next, cB_next)) -
                   (get_dist(cA, cA_next) + get_dist(cB, cB_next));

    if (delta < 0 || (T > 1e-9 && ((double)rand() / (double)RAND_MAX) < exp(-delta / T))) {
        int l = a + 1, h = b;
        while (l < h) {
            int temp = route[l];
            route[l] = route[h];
            route[h] = temp;
            l++;
            h--;
        }
    }
}

static void log_mpi_error(int rank, const char *context, int err_code) {
    char err_str[MPI_MAX_ERROR_STRING];
    int err_len = 0;
    MPI_Error_string(err_code, err_str, &err_len);
    fprintf(stderr, "Rank %d: MPI swap error in %s: %s\n", rank, context, err_str);
}

int main(int argc, char **argv) {
    SolverConfig cfg;
    cfg.total_global_replicas = DEFAULT_TOTAL_GLOBAL_REPLICAS;
    cfg.total_steps = DEFAULT_TOTAL_STEPS;
    cfg.swap_interval = DEFAULT_SWAP_INTERVAL;
    cfg.initial_temp = DEFAULT_INITIAL_TEMP;
    cfg.temp_decay = DEFAULT_TEMP_DECAY;
    cfg.deterministic = 0;
    cfg.seed = 0;
    cfg.seed_set = 0;
    cfg.filename = DEFAULT_TSP_FILE;

    int parse_status = parse_args(argc, argv, &cfg);
    if (parse_status == 1) {
        return 0;
    }
    if (parse_status != 0) {
        return 1;
    }

    MPI_Init(&argc, &argv);

    int rank, size;
    int exit_code = 0;
    int abort_code = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int *routes_flat = NULL;
    int **routes = NULL;
    double *temps = NULL;
    int *my_best_route = NULL;
    int *winner_route = NULL;
    int *swap_tmp = NULL;
    int *swap_buf = NULL;

    if (cfg.total_global_replicas <= 0) {
        if (rank == 0) fprintf(stderr, "Error: --replicas must be a positive integer.\n");
        exit_code = 1;
        goto cleanup;
    }

    if (cfg.swap_interval <= 0) {
        if (rank == 0) fprintf(stderr, "Error: --swap-interval must be a positive integer.\n");
        exit_code = 1;
        goto cleanup;
    }

    if (cfg.total_steps <= 0) {
        if (rank == 0) fprintf(stderr, "Error: --steps must be a positive integer.\n");
        exit_code = 1;
        goto cleanup;
    }

    if (cfg.initial_temp < 0.0) {
        if (rank == 0) fprintf(stderr, "Error: --initial-temp must be non-negative.\n");
        exit_code = 1;
        goto cleanup;
    }

    if (cfg.temp_decay <= 0.0) {
        if (rank == 0) fprintf(stderr, "Error: --temp-decay must be > 0.\n");
        exit_code = 1;
        goto cleanup;
    }

    /* --- 1. FILE READING (Rank 0) --- */
    if (rank == 0) {
        if (!read_tsp_file(cfg.filename)) {
            abort_code = 1;
            goto cleanup;
        }
    }

    MPI_Bcast(&N_CITIES, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0) {
        cities = (City *)malloc((size_t)N_CITIES * sizeof(City));
        if (!cities) {
            fprintf(stderr, "Rank %d: failed to allocate city buffer for %d cities.\n", rank, N_CITIES);
            abort_code = 1;
            goto cleanup;
        }
    }

    MPI_Bcast(cities, N_CITIES * (int)sizeof(City), MPI_BYTE, 0, MPI_COMM_WORLD);

    if (!init_dist_matrix()) {
        fprintf(stderr, "Rank %d: failed to allocate distance matrix for %d cities.\n", rank, N_CITIES);
        abort_code = 1;
        goto cleanup;
    }

    {
        unsigned long base_seed;
        if (cfg.deterministic) {
            base_seed = DETERMINISTIC_SEED;
        } else if (cfg.seed_set) {
            base_seed = cfg.seed;
        } else {
            base_seed = (unsigned long)time(NULL) ^ (unsigned long)rank;
        }
        srand((unsigned int)(base_seed + (unsigned long)rank));
    }

    /* --- 2. STRONG SCALING SETUP --- */
    if (cfg.total_global_replicas % size != 0) {
        if (rank == 0) {
            fprintf(stderr, "Error: replicas (%d) must be divisible by MPI processes (%d).\n",
                    cfg.total_global_replicas, size);
        }
        exit_code = 1;
        goto cleanup;
    }

    int my_replicas = cfg.total_global_replicas / size;
    int my_start_idx = rank * my_replicas;

    routes_flat = (int *)malloc((size_t)my_replicas * (size_t)N_CITIES * sizeof(int));
    routes = (int **)malloc((size_t)my_replicas * sizeof(int *));
    temps = (double *)malloc((size_t)my_replicas * sizeof(double));
    my_best_route = (int *)malloc((size_t)N_CITIES * sizeof(int));
    winner_route = (int *)malloc((size_t)N_CITIES * sizeof(int));
    swap_tmp = (int *)malloc((size_t)N_CITIES * sizeof(int));
    swap_buf = (int *)malloc((size_t)N_CITIES * sizeof(int));

    if (!routes_flat || !routes || !temps || !my_best_route || !winner_route || !swap_tmp || !swap_buf) {
        fprintf(stderr, "Rank %d: memory allocation failed while creating solver buffers.\n", rank);
        abort_code = 1;
        goto cleanup;
    }

    double my_best_dist = 1e15;

    for (int i = 0; i < my_replicas; i++) {
        routes[i] = &routes_flat[i * N_CITIES];
        for (int c = 0; c < N_CITIES; c++) routes[i][c] = c;

        /* Random Shuffle */
        for (int k = 0; k < N_CITIES; k++) {
            int r = rand() % N_CITIES;
            int t = routes[i][k];
            routes[i][k] = routes[i][r];
            routes[i][r] = t;
        }

        int g_idx = my_start_idx + i;
        if (g_idx == cfg.total_global_replicas - 1) temps[i] = 0.0;
        else temps[i] = cfg.initial_temp * pow(cfg.temp_decay, g_idx);
    }

    /* --- 3. MAIN LOOP --- */
    double start_time = MPI_Wtime();

    for (long step = 0; step < cfg.total_steps; step++) {
        /* Evolve */
        for (int i = 0; i < my_replicas; i++) {
            metropolis_step(routes[i], temps[i]);
        }

        /* Update Local Best */
        for (int i = 0; i < my_replicas; i++) {
            double d = calc_route_len(routes[i]);
            if (d < my_best_dist) {
                my_best_dist = d;
                memcpy(my_best_route, routes[i], (size_t)N_CITIES * sizeof(int));
            }
        }

        /* Swap Phase */
        if (step % cfg.swap_interval == 0) {

            /* A. Internal Swaps (Safe) */
            for (int i = 0; i < my_replicas - 1; i++) {
                int g_r = my_start_idx + i;
                if ((step / cfg.swap_interval) % 2 == 0 && g_r % 2 != 0) continue;
                if ((step / cfg.swap_interval) % 2 != 0 && g_r % 2 == 0) continue;

                double e1 = calc_route_len(routes[i]);
                double e2 = calc_route_len(routes[i + 1]);
                double delta = (1.0 / (temps[i] + 1e-9) - 1.0 / (temps[i + 1] + 1e-9)) * (e1 - e2);

                if (((double)rand() / (double)RAND_MAX) < exp(delta)) {
                    memcpy(swap_tmp, routes[i], (size_t)N_CITIES * sizeof(int));
                    memcpy(routes[i], routes[i + 1], (size_t)N_CITIES * sizeof(int));
                    memcpy(routes[i + 1], swap_tmp, (size_t)N_CITIES * sizeof(int));
                }
            }

            /* B. MPI Boundary Swaps (Deadlock Free Logic) */
            if (size > 1) {
                int mpi_err = MPI_SUCCESS;

                /* Step 1: Handle LEFT Partner (Receive then Send) */
                if (rank > 0) {
                    int left = rank - 1;
                    int left_last_g = (left + 1) * my_replicas - 1;
                    int expect = 0;
                    if ((step / cfg.swap_interval) % 2 == 0 && left_last_g % 2 == 0) expect = 1;
                    if ((step / cfg.swap_interval) % 2 != 0 && left_last_g % 2 != 0) expect = 1;

                    if (expect) {
                        double my_E = calc_route_len(routes[0]);
                        double partner_E;
                        mpi_err = MPI_Sendrecv(&my_E, 1, MPI_DOUBLE, left, 0,
                                               &partner_E, 1, MPI_DOUBLE, left, 0,
                                               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        if (mpi_err != MPI_SUCCESS) {
                            log_mpi_error(rank, "left energy exchange", mpi_err);
                            abort_code = 1;
                            goto cleanup;
                        }

                        int do_swap = 0;
                        mpi_err = MPI_Recv(&do_swap, 1, MPI_INT, left, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        if (mpi_err != MPI_SUCCESS) {
                            log_mpi_error(rank, "left swap decision receive", mpi_err);
                            abort_code = 1;
                            goto cleanup;
                        }

                        if (do_swap) {
                            mpi_err = MPI_Sendrecv(routes[0], N_CITIES, MPI_INT, left, 2,
                                                   swap_buf, N_CITIES, MPI_INT, left, 2,
                                                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            if (mpi_err != MPI_SUCCESS) {
                                log_mpi_error(rank, "left route swap", mpi_err);
                                abort_code = 1;
                                goto cleanup;
                            }
                            memcpy(routes[0], swap_buf, (size_t)N_CITIES * sizeof(int));
                        }
                    }
                }

                /* Step 2: Handle RIGHT Partner (Initiate Logic) */
                if (rank < size - 1) {
                    int right = rank + 1;
                    int my_last = my_replicas - 1;
                    int my_last_g = my_start_idx + my_last;

                    int init = 0;
                    if ((step / cfg.swap_interval) % 2 == 0 && my_last_g % 2 == 0) init = 1;
                    if ((step / cfg.swap_interval) % 2 != 0 && my_last_g % 2 != 0) init = 1;

                    if (init) {
                        double my_E = calc_route_len(routes[my_last]);
                        double partner_E;
                        mpi_err = MPI_Sendrecv(&my_E, 1, MPI_DOUBLE, right, 0,
                                               &partner_E, 1, MPI_DOUBLE, right, 0,
                                               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        if (mpi_err != MPI_SUCCESS) {
                            log_mpi_error(rank, "right energy exchange", mpi_err);
                            abort_code = 1;
                            goto cleanup;
                        }

                        int do_swap = 0;
                        double my_T = temps[my_last];
                        double partner_T = (my_last_g + 1 == cfg.total_global_replicas - 1)
                                               ? 0.0
                                               : cfg.initial_temp * pow(cfg.temp_decay, my_last_g + 1);
                        double delta = (1.0 / (my_T + 1e-9) - 1.0 / (partner_T + 1e-9)) * (my_E - partner_E);

                        if (((double)rand() / (double)RAND_MAX) < exp(delta)) do_swap = 1;

                        mpi_err = MPI_Send(&do_swap, 1, MPI_INT, right, 1, MPI_COMM_WORLD);
                        if (mpi_err != MPI_SUCCESS) {
                            log_mpi_error(rank, "right swap decision send", mpi_err);
                            abort_code = 1;
                            goto cleanup;
                        }

                        if (do_swap) {
                            mpi_err = MPI_Sendrecv(routes[my_last], N_CITIES, MPI_INT, right, 2,
                                                   swap_buf, N_CITIES, MPI_INT, right, 2,
                                                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            if (mpi_err != MPI_SUCCESS) {
                                log_mpi_error(rank, "right route swap", mpi_err);
                                abort_code = 1;
                                goto cleanup;
                            }
                            memcpy(routes[my_last], swap_buf, (size_t)N_CITIES * sizeof(int));
                        }
                    }
                }
            }
        }
    }

    /* --- 4. FINALIZE & SAVE --- */
    double end_time = MPI_Wtime();

    /* Find Global Best */
    struct {
        double val;
        int rank;
    } loc_data = {my_best_dist, rank}, glob_data;
    MPI_Allreduce(&loc_data, &glob_data, 1, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);

    /* Winner Rank sends the Route to Rank 0 */
    if (rank == glob_data.rank) {
        memcpy(winner_route, my_best_route, (size_t)N_CITIES * sizeof(int));
    }

    /* Broadcast winning route to Rank 0 (and everyone else) */
    MPI_Bcast(winner_route, N_CITIES, MPI_INT, glob_data.rank, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Simulation Time: %.4f s | Global Best: %.4f (Found by Rank %d)\n",
               end_time - start_time, glob_data.val, glob_data.rank);

        FILE *f = fopen("solution.txt", "w");
        if (!f) {
            fprintf(stderr, "Error: failed to write solution.txt: %s\n", strerror(errno));
            exit_code = 1;
            goto cleanup;
        }
        fprintf(f, "%.4f\n", glob_data.val);
        fclose(f);

        FILE *fr = fopen("my_route.txt", "w");
        if (!fr) {
            fprintf(stderr, "Error: failed to write my_route.txt: %s\n", strerror(errno));
            exit_code = 1;
            goto cleanup;
        }
        for (int i = 0; i < N_CITIES; i++) fprintf(fr, "%d\n", winner_route[i]);
        fclose(fr);
    }

cleanup:
    free(routes_flat);
    free(routes);
    free(temps);
    free(cities);
    free(dist_matrix);
    free(my_best_route);
    free(winner_route);
    free(swap_tmp);
    free(swap_buf);

    if (abort_code != 0) {
        MPI_Abort(MPI_COMM_WORLD, abort_code);
        return abort_code;
    }

    MPI_Finalize();
    return exit_code;
}
