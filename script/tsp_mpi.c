/* tsp_mpi.c
 *
 * Distributed parallel-tempering solver for symmetric TSPLIB instances.
 *
 * Current supported edge-weight type:
 *   - EUC_2D, using the TSPLIB nearest-integer distance convention
 *
 * The default scientific configuration remains the original course setup:
 * 24 global replicas and 5000 swap attempts. Use --seed for reproducible runs.
 */

#include <mpi.h>

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TOTAL_GLOBAL_REPLICAS 24
#define INITIAL_TEMP 150.0
#define TEMP_DECAY 0.80
#define SWAP_INTERVAL 52
#define TOTAL_SWAP_ATTEMPTS 5000
#define TOTAL_STEPS (SWAP_INTERVAL * TOTAL_SWAP_ATTEMPTS)

typedef struct {
    double x;
    double y;
} City;

typedef enum {
    EDGE_WEIGHT_UNKNOWN = 0,
    EDGE_WEIGHT_EUC_2D = 1
} EdgeWeightType;

typedef struct {
    const char *instance_path;
    const char *evaluate_tour_path;
    unsigned int seed;
    int seed_was_set;
    int show_help;
} Options;

static int n_cities = 0;
static City *cities = NULL;
static double *dist_matrix = NULL;
static EdgeWeightType edge_weight_type = EDGE_WEIGHT_UNKNOWN;

static void print_usage(const char *program) {
    printf(
        "Usage: %s [instance.tsp] [--seed INTEGER] [--evaluate-tour tour.opt.tour]\n"
        "\n"
        "Examples:\n"
        "  mpirun -np 4 %s data/berlin52.tsp --seed 12345\n"
        "  mpirun -np 1 %s data/berlin52.tsp --evaluate-tour data/berlin52.opt.tour\n",
        program,
        program,
        program
    );
}

static int parse_unsigned(const char *text, unsigned int *value) {
    char *end = NULL;
    errno = 0;
    unsigned long parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed > 0xffffffffUL) {
        return 0;
    }
    *value = (unsigned int)parsed;
    return 1;
}

static int parse_options(int argc, char **argv, Options *options) {
    options->instance_path = "berlin52.tsp";
    options->evaluate_tour_path = NULL;
    options->seed = 0;
    options->seed_was_set = 0;
    options->show_help = 0;

    int instance_was_set = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--seed") == 0) {
            if (i + 1 >= argc || !parse_unsigned(argv[i + 1], &options->seed)) {
                fprintf(stderr, "Invalid or missing value after --seed.\n");
                return 0;
            }
            options->seed_was_set = 1;
            ++i;
        } else if (strcmp(argv[i], "--evaluate-tour") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing path after --evaluate-tour.\n");
                return 0;
            }
            options->evaluate_tour_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            options->show_help = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 0;
        } else if (!instance_was_set) {
            options->instance_path = argv[i];
            instance_was_set = 1;
        } else {
            fprintf(stderr, "Unexpected positional argument: %s\n", argv[i]);
            return 0;
        }
    }
    return 1;
}

static int starts_with(const char *text, const char *prefix) {
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static int read_tsp_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0;
    }

    char line[256];
    int reading_coords = 0;
    int coordinates_read = 0;
    unsigned char *seen = NULL;

    while (fgets(line, sizeof(line), file)) {
        if (starts_with(line, "DIMENSION")) {
            char *separator = strchr(line, ':');
            if (separator) {
                n_cities = atoi(separator + 1);
            }
        } else if (starts_with(line, "EDGE_WEIGHT_TYPE")) {
            char value[64] = {0};
            char *separator = strchr(line, ':');
            if (separator && sscanf(separator + 1, "%63s", value) == 1) {
                if (strcmp(value, "EUC_2D") == 0) {
                    edge_weight_type = EDGE_WEIGHT_EUC_2D;
                } else {
                    fprintf(stderr, "Unsupported EDGE_WEIGHT_TYPE: %s\n", value);
                    fclose(file);
                    return 0;
                }
            }
        } else if (starts_with(line, "NODE_COORD_SECTION")) {
            if (n_cities <= 0) {
                fprintf(stderr, "DIMENSION must appear before NODE_COORD_SECTION.\n");
                fclose(file);
                return 0;
            }
            cities = calloc((size_t)n_cities, sizeof(*cities));
            seen = calloc((size_t)n_cities, sizeof(*seen));
            if (!cities || !seen) {
                fprintf(stderr, "Could not allocate city storage.\n");
                free(cities);
                free(seen);
                cities = NULL;
                fclose(file);
                return 0;
            }
            reading_coords = 1;
            continue;
        } else if (starts_with(line, "EOF")) {
            break;
        }

        if (reading_coords) {
            int id = 0;
            double x = 0.0;
            double y = 0.0;
            if (sscanf(line, "%d %lf %lf", &id, &x, &y) == 3) {
                if (id < 1 || id > n_cities || seen[id - 1]) {
                    fprintf(stderr, "Invalid or duplicate city identifier: %d\n", id);
                    free(seen);
                    fclose(file);
                    return 0;
                }
                cities[id - 1].x = x;
                cities[id - 1].y = y;
                seen[id - 1] = 1;
                ++coordinates_read;
            }
        }
    }

    free(seen);
    fclose(file);

    if (edge_weight_type != EDGE_WEIGHT_EUC_2D) {
        fprintf(stderr, "The instance must declare EDGE_WEIGHT_TYPE: EUC_2D.\n");
        return 0;
    }

    return coordinates_read == n_cities;
}

static double tsplib_euc_2d_distance(City start, City end) {
    const double dx = start.x - end.x;
    const double dy = start.y - end.y;
    return floor(sqrt(dx * dx + dy * dy) + 0.5);
}

static int init_dist_matrix(void) {
    dist_matrix = malloc((size_t)n_cities * (size_t)n_cities * sizeof(*dist_matrix));
    if (!dist_matrix) {
        return 0;
    }

    for (int i = 0; i < n_cities; ++i) {
        for (int j = 0; j < n_cities; ++j) {
            dist_matrix[i * n_cities + j] = tsplib_euc_2d_distance(cities[i], cities[j]);
        }
    }
    return 1;
}

static inline double get_dist(int i, int j) {
    return dist_matrix[i * n_cities + j];
}

static double calc_route_len(const int *route) {
    double distance = 0.0;
    for (int i = 0; i < n_cities; ++i) {
        distance += get_dist(route[i], route[(i + 1) % n_cities]);
    }
    return distance;
}

static int read_tour_file(const char *filename, int *route) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0;
    }

    char line[256];
    int reading_tour = 0;
    int count = 0;
    unsigned char *seen = calloc((size_t)n_cities, sizeof(*seen));
    if (!seen) {
        fclose(file);
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        if (starts_with(line, "TOUR_SECTION")) {
            reading_tour = 1;
            continue;
        }
        if (!reading_tour) {
            continue;
        }
        if (starts_with(line, "EOF")) {
            break;
        }

        int id = 0;
        if (sscanf(line, "%d", &id) != 1) {
            continue;
        }
        if (id == -1) {
            break;
        }
        if (id < 1 || id > n_cities || seen[id - 1] || count >= n_cities) {
            free(seen);
            fclose(file);
            return 0;
        }

        route[count++] = id - 1;
        seen[id - 1] = 1;
    }

    free(seen);
    fclose(file);
    return count == n_cities;
}

static void metropolis_step(int *route, double temperature) {
    int a = rand() % n_cities;
    int b = rand() % n_cities;
    if (a == b) {
        return;
    }
    if (a > b) {
        const int temporary = a;
        a = b;
        b = temporary;
    }

    if (b == a + 1 || (a == 0 && b == n_cities - 1)) {
        return;
    }

    const int city_a = route[a];
    const int city_a_next = route[(a + 1) % n_cities];
    const int city_b = route[b];
    const int city_b_next = route[(b + 1) % n_cities];

    const double delta =
        get_dist(city_a, city_b) +
        get_dist(city_a_next, city_b_next) -
        get_dist(city_a, city_a_next) -
        get_dist(city_b, city_b_next);

    const double random_value = (double)rand() / (double)RAND_MAX;
    if (delta < 0.0 || (temperature > 1.0e-9 && random_value < exp(-delta / temperature))) {
        int low = a + 1;
        int high = b;
        while (low < high) {
            const int temporary = route[low];
            route[low] = route[high];
            route[high] = temporary;
            ++low;
            --high;
        }
    }
}

static void free_problem(void) {
    free(cities);
    free(dist_matrix);
    cities = NULL;
    dist_matrix = NULL;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Options options;
    const int options_ok = parse_options(argc, argv, &options);
    if (!options_ok || options.show_help) {
        if (rank == 0) {
            print_usage(argv[0]);
        }
        MPI_Finalize();
        return options_ok ? 0 : 2;
    }

    int problem_ok = 1;
    if (rank == 0) {
        problem_ok = read_tsp_file(options.instance_path);
        if (!problem_ok) {
            fprintf(stderr, "Could not read a supported TSPLIB instance from %s\n", options.instance_path);
        }
    }

    MPI_Bcast(&problem_ok, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (!problem_ok) {
        MPI_Finalize();
        return 1;
    }

    int edge_weight_code = (int)edge_weight_type;
    MPI_Bcast(&n_cities, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&edge_weight_code, 1, MPI_INT, 0, MPI_COMM_WORLD);
    edge_weight_type = (EdgeWeightType)edge_weight_code;

    if (rank != 0) {
        cities = malloc((size_t)n_cities * sizeof(*cities));
        if (!cities) {
            fprintf(stderr, "Rank %d could not allocate city storage.\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    MPI_Bcast(cities, n_cities * (int)sizeof(*cities), MPI_BYTE, 0, MPI_COMM_WORLD);

    if (!init_dist_matrix()) {
        fprintf(stderr, "Rank %d could not allocate the distance matrix.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (options.evaluate_tour_path) {
        int evaluation_ok = 1;
        if (rank == 0) {
            int *route = malloc((size_t)n_cities * sizeof(*route));
            if (!route || !read_tour_file(options.evaluate_tour_path, route)) {
                fprintf(stderr, "Could not read a valid tour from %s\n", options.evaluate_tour_path);
                evaluation_ok = 0;
            } else {
                const double weight = calc_route_len(route);
                printf("Tour weight (TSPLIB EUC_2D): %.0f\n", weight);
            }
            free(route);
        }

        MPI_Bcast(&evaluation_ok, 1, MPI_INT, 0, MPI_COMM_WORLD);
        free_problem();
        MPI_Finalize();
        return evaluation_ok ? 0 : 1;
    }

    if (TOTAL_GLOBAL_REPLICAS % size != 0) {
        if (rank == 0) {
            fprintf(
                stderr,
                "TOTAL_GLOBAL_REPLICAS (%d) must be divisible by the MPI process count (%d).\n",
                TOTAL_GLOBAL_REPLICAS,
                size
            );
        }
        free_problem();
        MPI_Finalize();
        return 1;
    }

    const unsigned int base_seed =
        options.seed_was_set ? options.seed : (unsigned int)time(NULL);
    srand(base_seed + (unsigned int)rank * 999U);

    if (rank == 0) {
        printf(
            "Instance: %s | EDGE_WEIGHT_TYPE: EUC_2D | Base seed: %u\n",
            options.instance_path,
            base_seed
        );
    }

    const int my_replicas = TOTAL_GLOBAL_REPLICAS / size;
    const int my_start_index = rank * my_replicas;

    int *routes_flat = malloc((size_t)my_replicas * (size_t)n_cities * sizeof(*routes_flat));
    int **routes = malloc((size_t)my_replicas * sizeof(*routes));
    double *temperatures = malloc((size_t)my_replicas * sizeof(*temperatures));
    int *my_best_route = malloc((size_t)n_cities * sizeof(*my_best_route));
    int *winner_route = malloc((size_t)n_cities * sizeof(*winner_route));

    if (!routes_flat || !routes || !temperatures || !my_best_route || !winner_route) {
        fprintf(stderr, "Rank %d could not allocate solver storage.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    double my_best_distance = INFINITY;

    for (int replica = 0; replica < my_replicas; ++replica) {
        routes[replica] = &routes_flat[replica * n_cities];
        for (int city = 0; city < n_cities; ++city) {
            routes[replica][city] = city;
        }

        for (int city = n_cities - 1; city > 0; --city) {
            const int random_index = rand() % (city + 1);
            const int temporary = routes[replica][city];
            routes[replica][city] = routes[replica][random_index];
            routes[replica][random_index] = temporary;
        }

        const int global_index = my_start_index + replica;
        temperatures[replica] =
            global_index == TOTAL_GLOBAL_REPLICAS - 1
                ? 0.0
                : INITIAL_TEMP * pow(TEMP_DECAY, global_index);
    }

    const double start_time = MPI_Wtime();

    for (long step = 0; step < TOTAL_STEPS; ++step) {
        for (int replica = 0; replica < my_replicas; ++replica) {
            metropolis_step(routes[replica], temperatures[replica]);
        }

        for (int replica = 0; replica < my_replicas; ++replica) {
            const double distance = calc_route_len(routes[replica]);
            if (distance < my_best_distance) {
                my_best_distance = distance;
                memcpy(my_best_route, routes[replica], (size_t)n_cities * sizeof(*my_best_route));
            }
        }

        if (step % SWAP_INTERVAL == 0) {
            const long exchange_phase = step / SWAP_INTERVAL;

            for (int replica = 0; replica < my_replicas - 1; ++replica) {
                const int global_replica = my_start_index + replica;
                if (exchange_phase % 2 == 0 && global_replica % 2 != 0) {
                    continue;
                }
                if (exchange_phase % 2 != 0 && global_replica % 2 == 0) {
                    continue;
                }

                const double first_energy = calc_route_len(routes[replica]);
                const double second_energy = calc_route_len(routes[replica + 1]);
                const double exponent =
                    (1.0 / (temperatures[replica] + 1.0e-9) -
                     1.0 / (temperatures[replica + 1] + 1.0e-9)) *
                    (first_energy - second_energy);

                if ((double)rand() / (double)RAND_MAX < exp(exponent)) {
                    int *temporary_route = malloc((size_t)n_cities * sizeof(*temporary_route));
                    if (!temporary_route) {
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    }
                    memcpy(temporary_route, routes[replica], (size_t)n_cities * sizeof(*temporary_route));
                    memcpy(routes[replica], routes[replica + 1], (size_t)n_cities * sizeof(*temporary_route));
                    memcpy(routes[replica + 1], temporary_route, (size_t)n_cities * sizeof(*temporary_route));
                    free(temporary_route);
                }
            }

            if (size > 1) {
                if (rank > 0) {
                    const int left = rank - 1;
                    const int left_last_global = (left + 1) * my_replicas - 1;
                    int exchange_expected = 0;
                    if (exchange_phase % 2 == 0 && left_last_global % 2 == 0) {
                        exchange_expected = 1;
                    }
                    if (exchange_phase % 2 != 0 && left_last_global % 2 != 0) {
                        exchange_expected = 1;
                    }

                    if (exchange_expected) {
                        const double my_energy = calc_route_len(routes[0]);
                        double partner_energy = 0.0;
                        MPI_Sendrecv(
                            &my_energy,
                            1,
                            MPI_DOUBLE,
                            left,
                            0,
                            &partner_energy,
                            1,
                            MPI_DOUBLE,
                            left,
                            0,
                            MPI_COMM_WORLD,
                            MPI_STATUS_IGNORE
                        );

                        int do_swap = 0;
                        MPI_Recv(&do_swap, 1, MPI_INT, left, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        if (do_swap) {
                            int *buffer = malloc((size_t)n_cities * sizeof(*buffer));
                            if (!buffer) {
                                MPI_Abort(MPI_COMM_WORLD, 1);
                            }
                            MPI_Sendrecv(
                                routes[0],
                                n_cities,
                                MPI_INT,
                                left,
                                2,
                                buffer,
                                n_cities,
                                MPI_INT,
                                left,
                                2,
                                MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE
                            );
                            memcpy(routes[0], buffer, (size_t)n_cities * sizeof(*buffer));
                            free(buffer);
                        }
                    }
                }

                if (rank < size - 1) {
                    const int right = rank + 1;
                    const int last_local = my_replicas - 1;
                    const int last_global = my_start_index + last_local;
                    int initiate_exchange = 0;
                    if (exchange_phase % 2 == 0 && last_global % 2 == 0) {
                        initiate_exchange = 1;
                    }
                    if (exchange_phase % 2 != 0 && last_global % 2 != 0) {
                        initiate_exchange = 1;
                    }

                    if (initiate_exchange) {
                        const double my_energy = calc_route_len(routes[last_local]);
                        double partner_energy = 0.0;
                        MPI_Sendrecv(
                            &my_energy,
                            1,
                            MPI_DOUBLE,
                            right,
                            0,
                            &partner_energy,
                            1,
                            MPI_DOUBLE,
                            right,
                            0,
                            MPI_COMM_WORLD,
                            MPI_STATUS_IGNORE
                        );

                        const double my_temperature = temperatures[last_local];
                        const double partner_temperature =
                            last_global + 1 == TOTAL_GLOBAL_REPLICAS - 1
                                ? 0.0
                                : INITIAL_TEMP * pow(TEMP_DECAY, last_global + 1);
                        const double exponent =
                            (1.0 / (my_temperature + 1.0e-9) -
                             1.0 / (partner_temperature + 1.0e-9)) *
                            (my_energy - partner_energy);

                        const int do_swap =
                            (double)rand() / (double)RAND_MAX < exp(exponent);
                        MPI_Send(&do_swap, 1, MPI_INT, right, 1, MPI_COMM_WORLD);

                        if (do_swap) {
                            int *buffer = malloc((size_t)n_cities * sizeof(*buffer));
                            if (!buffer) {
                                MPI_Abort(MPI_COMM_WORLD, 1);
                            }
                            MPI_Sendrecv(
                                routes[last_local],
                                n_cities,
                                MPI_INT,
                                right,
                                2,
                                buffer,
                                n_cities,
                                MPI_INT,
                                right,
                                2,
                                MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE
                            );
                            memcpy(routes[last_local], buffer, (size_t)n_cities * sizeof(*buffer));
                            free(buffer);
                        }
                    }
                }
            }
        }
    }

    const double end_time = MPI_Wtime();

    struct {
        double value;
        int rank;
    } local_best = {my_best_distance, rank}, global_best;

    MPI_Allreduce(
        &local_best,
        &global_best,
        1,
        MPI_DOUBLE_INT,
        MPI_MINLOC,
        MPI_COMM_WORLD
    );

    if (rank == global_best.rank) {
        memcpy(winner_route, my_best_route, (size_t)n_cities * sizeof(*winner_route));
    }
    MPI_Bcast(winner_route, n_cities, MPI_INT, global_best.rank, MPI_COMM_WORLD);

    if (rank == 0) {
        printf(
            "Simulation Time: %.4f s | Global Best: %.0f (Found by Rank %d)\n",
            end_time - start_time,
            global_best.value,
            global_best.rank
        );

        FILE *solution_file = fopen("solution.txt", "w");
        FILE *route_file = fopen("my_route.txt", "w");
        if (!solution_file || !route_file) {
            fprintf(stderr, "Could not write solver outputs.\n");
            if (solution_file) {
                fclose(solution_file);
            }
            if (route_file) {
                fclose(route_file);
            }
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fprintf(solution_file, "%.0f\n", global_best.value);
        for (int city = 0; city < n_cities; ++city) {
            fprintf(route_file, "%d\n", winner_route[city]);
        }
        fclose(solution_file);
        fclose(route_file);
    }

    free(routes_flat);
    free(routes);
    free(temperatures);
    free(my_best_route);
    free(winner_route);
    free_problem();

    MPI_Finalize();
    return 0;
}
