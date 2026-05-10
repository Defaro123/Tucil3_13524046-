#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#include <direct.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#endif

#define INF (LLONG_MAX / 4)
#define MAX_PATH_LEN 256

typedef enum {
    ALG_UCS,
    ALG_GBFS,
    ALG_ASTAR
} Algorithm;

typedef enum {
    HEUR_H1,
    HEUR_H2,
    HEUR_H3
} HeuristicType;

typedef struct {
    int n, m;
    char **grid;
    long long **cost;
    int startR, startC;
    int goalR, goalC;
    int maxDigit;
    int digitExists[10];
    int digitR[10];
    int digitC[10];
    long long minPositiveCost;
} Board;

typedef struct {
    int r, c;
    int nextDigit;
    long long moveCost;
} MoveResult;

typedef struct {
    int key;
    long long priority;
    long long g;
    long long order;
} PQItem;

typedef struct {
    PQItem *data;
    int size;
    int capacity;
    long long pushOrder;
} PriorityQueue;

typedef struct {
    int *data;
    int size;
    int capacity;
} IntVector;

typedef struct {
    int found;
    int finalKey;
    long long finalCost;
    long long expandedCount;
    double execMs;
    int *parent;
    char *parentAction;
    long long *dist;
    IntVector expandedTrace;
} SearchResult;

typedef struct {
    int *keys;
    char *actions;
    int moveCount;
} SolutionPath;

static const int DR[4] = {-1, 0, 1, 0};
static const int DC[4] = {0, 1, 0, -1};
static const char ACT[4] = {'U', 'R', 'D', 'L'};

static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static int equals_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static int yes_answer(const char *s) {
    return equals_ignore_case(s, "ya") ||
           equals_ignore_case(s, "y") ||
           equals_ignore_case(s, "yes");
}

static int has_path_separator(const char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '/' || s[i] == '\\') return 1;
    }
    return 0;
}

#ifdef _WIN32
static int directory_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static int file_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static int make_directory_if_missing(const char *path) {
    if (directory_exists(path)) return 1;
    return _mkdir(path) == 0 || directory_exists(path);
}
#else
static int directory_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && !S_ISDIR(st.st_mode);
}

static int make_directory_if_missing(const char *path) {
    if (directory_exists(path)) return 1;
    return mkdir(path, 0755) == 0 || directory_exists(path);
}
#endif

static FILE *open_input_file(const char *filename, char *resolvedPath, size_t resolvedSize) {
    FILE *fp = fopen(filename, "r");
    if (fp) {
        snprintf(resolvedPath, resolvedSize, "%s", filename);
        return fp;
    }

    if (!has_path_separator(filename)) {
        char candidate[MAX_PATH_LEN * 2];
        snprintf(candidate, sizeof(candidate), "src/%s", filename);
        fp = fopen(candidate, "r");
        if (fp) {
            snprintf(resolvedPath, resolvedSize, "%s", candidate);
            return fp;
        }

        snprintf(candidate, sizeof(candidate), "./src/%s", filename);
        fp = fopen(candidate, "r");
        if (fp) {
            snprintf(resolvedPath, resolvedSize, "%s", candidate);
            return fp;
        }
    }

    resolvedPath[0] = '\0';
    return NULL;
}

static void resolve_output_path(const char *inputName, char *resolvedPath, size_t resolvedSize) {
    if (has_path_separator(inputName)) {
        snprintf(resolvedPath, resolvedSize, "%s", inputName);
        return;
    }
    if (file_exists("main.c") && !directory_exists("src")) {
        make_directory_if_missing("../test");
        snprintf(resolvedPath, resolvedSize, "../test/%s", inputName);
    } else {
        make_directory_if_missing("test");
        snprintf(resolvedPath, resolvedSize, "test/%s", inputName);
    }
}

static int parse_algorithm(const char *s, Algorithm *alg) {
    if (equals_ignore_case(s, "UCS")) {
        *alg = ALG_UCS;
        return 1;
    }
    if (equals_ignore_case(s, "GBFS")) {
        *alg = ALG_GBFS;
        return 1;
    }
    if (equals_ignore_case(s, "A*") ||
        equals_ignore_case(s, "ASTAR") ||
        equals_ignore_case(s, "A_STAR")) {
        *alg = ALG_ASTAR;
        return 1;
    }
    return 0;
}

static int parse_heuristic(const char *s, HeuristicType *h) {
    if (equals_ignore_case(s, "H1")) {
        *h = HEUR_H1;
        return 1;
    }
    if (equals_ignore_case(s, "H2")) {
        *h = HEUR_H2;
        return 1;
    }
    if (equals_ignore_case(s, "H3")) {
        *h = HEUR_H3;
        return 1;
    }
    return 0;
}

static const char *algorithm_name(Algorithm alg) {
    switch (alg) {
        case ALG_UCS: return "UCS";
        case ALG_GBFS: return "GBFS";
        case ALG_ASTAR: return "A*";
        default: return "UNKNOWN";
    }
}

static const char *heuristic_name(HeuristicType h) {
    switch (h) {
        case HEUR_H1: return "H1";
        case HEUR_H2: return "H2";
        case HEUR_H3: return "H3";
        default: return "UNKNOWN";
    }
}

static int is_digit_tile(char ch) {
    return ch >= '0' && ch <= '9';
}

static int is_valid_tile(char ch) {
    return ch == '*' || ch == 'X' || ch == 'L' || ch == 'Z' || ch == 'O' || is_digit_tile(ch);
}

static void init_board(Board *b) {
    memset(b, 0, sizeof(Board));
    b->startR = b->startC = -1;
    b->goalR = b->goalC = -1;
    b->maxDigit = -1;
    b->minPositiveCost = INF;
    for (int i = 0; i < 10; i++) {
        b->digitExists[i] = 0;
        b->digitR[i] = b->digitC[i] = -1;
    }
}

static void free_board(Board *b) {
    if (!b) return;

    if (b->grid) {
        for (int i = 0; i < b->n; i++) {
            free(b->grid[i]);
        }
        free(b->grid);
    }

    if (b->cost) {
        for (int i = 0; i < b->n; i++) {
            free(b->cost[i]);
        }
        free(b->cost);
    }

    init_board(b);
}

static int allocate_board(Board *b, int n, int m) {
    b->n = n;
    b->m = m;
    b->grid = (char **)malloc((size_t)n * sizeof(char *));
    b->cost = (long long **)malloc((size_t)n * sizeof(long long *));

    if (!b->grid || !b->cost) return 0;

    for (int i = 0; i < n; i++) {
        b->grid[i] = NULL;
        b->cost[i] = NULL;
    }

    for (int i = 0; i < n; i++) {
        b->grid[i] = (char *)malloc((size_t)m + 1);
        b->cost[i] = (long long *)malloc((size_t)m * sizeof(long long));

        if (!b->grid[i] || !b->cost[i]) return 0;
    }

    return 1;
}

static int load_board(const char *filename, Board *b, char *err, size_t errSize) {
    init_board(b);

    char resolvedPath[MAX_PATH_LEN * 2];
    FILE *fp = open_input_file(filename, resolvedPath, sizeof(resolvedPath));

    if (!fp) {
        snprintf(err, errSize, "File tidak dapat dibuka: %s", filename);
        return 0;
    }

    int n, m;
    if (fscanf(fp, "%d %d", &n, &m) != 2) {
        snprintf(err, errSize, "Baris pertama harus berisi N dan M.");
        fclose(fp);
        return 0;
    }

    if (n <= 0 || m <= 0) {
        snprintf(err, errSize, "Ukuran papan harus positif.");
        fclose(fp);
        return 0;
    }

    if (!allocate_board(b, n, m)) {
        snprintf(err, errSize, "Alokasi memori gagal.");
        fclose(fp);
        free_board(b);
        return 0;
    }

    char line[4096];
    int startCount = 0;
    int goalCount = 0;

    for (int i = 0; i < n; i++) {
        if (fscanf(fp, "%4095s", line) != 1) {
            snprintf(err, errSize, "Baris papan ke-%d tidak ditemukan.", i + 1);
            fclose(fp);
            free_board(b);
            return 0;
        }

        if ((int)strlen(line) != m) {
            snprintf(err, errSize, "Panjang baris papan ke-%d harus %d karakter.", i + 1, m);
            fclose(fp);
            free_board(b);
            return 0;
        }

        for (int j = 0; j < m; j++) {
            char ch = line[j];

            if (!is_valid_tile(ch)) {
                snprintf(err, errSize, "Karakter tidak valid '%c' pada (%d,%d).", ch, i, j);
                fclose(fp);
                free_board(b);
                return 0;
            }

            b->grid[i][j] = ch;

            if (ch == 'Z') {
                startCount++;
                b->startR = i;
                b->startC = j;
            } else if (ch == 'O') {
                goalCount++;
                b->goalR = i;
                b->goalC = j;
            } else if (is_digit_tile(ch)) {
                int d = ch - '0';

                if (b->digitExists[d]) {
                    snprintf(err, errSize, "Digit %d muncul lebih dari satu kali.", d);
                    fclose(fp);
                    free_board(b);
                    return 0;
                }

                b->digitExists[d] = 1;
                b->digitR[d] = i;
                b->digitC[d] = j;

                if (d > b->maxDigit) b->maxDigit = d;
            }
        }

        b->grid[i][m] = '\0';
    }

    if (startCount != 1) {
        snprintf(err, errSize, "Papan harus memiliki tepat satu Z sebagai posisi awal.");
        fclose(fp);
        free_board(b);
        return 0;
    }

    if (goalCount != 1) {
        snprintf(err, errSize, "Papan harus memiliki tepat satu O sebagai tujuan.");
        fclose(fp);
        free_board(b);
        return 0;
    }

    if (b->maxDigit >= 0) {
        for (int d = 0; d <= b->maxDigit; d++) {
            if (!b->digitExists[d]) {
                snprintf(err, errSize, "Digit wajib harus kontigu dari 0. Digit %d tidak ditemukan.", d);
                fclose(fp);
                free_board(b);
                return 0;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            long long c;

            if (fscanf(fp, "%lld", &c) != 1) {
                snprintf(err, errSize, "Cost tile pada (%d,%d) tidak ditemukan.", i, j);
                fclose(fp);
                free_board(b);
                return 0;
            }

            if (c < 0) {
                snprintf(err, errSize, "Cost tile tidak boleh negatif pada (%d,%d).", i, j);
                fclose(fp);
                free_board(b);
                return 0;
            }

            b->cost[i][j] = c;

            char tile = b->grid[i][j];
            if (tile != 'X' && tile != 'L' && c > 0 && c < b->minPositiveCost) {
                b->minPositiveCost = c;
            }
        }
    }

    if (b->minPositiveCost == INF) {
        b->minPositiveCost = 1;
    }

    fclose(fp);
    return 1;
}

static int target_count(const Board *b) {
    return b->maxDigit >= 0 ? b->maxDigit + 1 : 0;
}

static int state_count(const Board *b) {
    int tc = target_count(b);
    return b->n * b->m * (tc + 1);
}

static int encode_key(const Board *b, int r, int c, int nextDigit) {
    return (nextDigit * b->n + r) * b->m + c;
}

static void decode_key(const Board *b, int key, int *r, int *c, int *nextDigit) {
    *c = key % b->m;
    int tmp = key / b->m;
    *r = tmp % b->n;
    *nextDigit = tmp / b->n;
}

static int abs_int(int x) {
    return x < 0 ? -x : x;
}

static long long manhattan(int r1, int c1, int r2, int c2) {
    return (long long)abs_int(r1 - r2) + abs_int(c1 - c2);
}

static long long heuristic_value(const Board *b, int r, int c, int nextDigit, HeuristicType hType) {
    int tc = target_count(b);
    long long h = 0;

    if (hType == HEUR_H1 || hType == HEUR_H2) {
        int tr, tcCol;

        if (nextDigit < tc) {
            tr = b->digitR[nextDigit];
            tcCol = b->digitC[nextDigit];
        } else {
            tr = b->goalR;
            tcCol = b->goalC;
        }

        h = manhattan(r, c, tr, tcCol);

        if (hType == HEUR_H2) {
            h *= b->minPositiveCost;
        }

        return h;
    }

    int cr = r;
    int cc = c;

    for (int d = nextDigit; d < tc; d++) {
        h += manhattan(cr, cc, b->digitR[d], b->digitC[d]);
        cr = b->digitR[d];
        cc = b->digitC[d];
    }

    h += manhattan(cr, cc, b->goalR, b->goalC);
    h *= b->minPositiveCost;

    return h;
}

static int slide_move(const Board *b, int r, int c, int nextDigit, int dir, MoveResult *res) {
    int curR = r;
    int curC = c;
    long long totalCost = 0;
    int moved = 0;
    int newNext = nextDigit;
    int tc = target_count(b);

    while (1) {
        int nr = curR + DR[dir];
        int nc = curC + DC[dir];

        if (nr < 0 || nr >= b->n || nc < 0 || nc >= b->m) {
            return 0;
        }

        char tile = b->grid[nr][nc];

        if (tile == 'X') {
            if (!moved) return 0;

            res->r = curR;
            res->c = curC;
            res->nextDigit = newNext;
            res->moveCost = totalCost;
            return 1;
        }

        if (tile == 'L') {
            return 0;
        }

        moved = 1;
        totalCost += b->cost[nr][nc];

        if (is_digit_tile(tile)) {
            int d = tile - '0';

            if (d < newNext) {
            } else if (d == newNext && newNext < tc) {
                newNext++;
            } else {
                return 0;
            }
        }

        curR = nr;
        curC = nc;

        if (tile == 'O' && newNext == tc) {
            res->r = curR;
            res->c = curC;
            res->nextDigit = newNext;
            res->moveCost = totalCost;
            return 1;
        }
    }
}

static void pq_init(PriorityQueue *pq) {
    pq->size = 0;
    pq->capacity = 64;
    pq->pushOrder = 0;
    pq->data = (PQItem *)malloc((size_t)pq->capacity * sizeof(PQItem));
}

static void pq_free(PriorityQueue *pq) {
    free(pq->data);
    pq->data = NULL;
    pq->size = 0;
    pq->capacity = 0;
    pq->pushOrder = 0;
}

static int pq_less(PQItem a, PQItem b) {
    if (a.priority != b.priority) return a.priority < b.priority;
    if (a.g != b.g) return a.g < b.g;
    return a.order < b.order;
}

static void pq_swap(PQItem *a, PQItem *b) {
    PQItem t = *a;
    *a = *b;
    *b = t;
}

static int pq_push(PriorityQueue *pq, int key, long long priority, long long g) {
    if (pq->size >= pq->capacity) {
        int newCap = pq->capacity * 2;
        PQItem *newData = (PQItem *)realloc(pq->data, (size_t)newCap * sizeof(PQItem));

        if (!newData) return 0;

        pq->data = newData;
        pq->capacity = newCap;
    }

    int idx = pq->size++;

    pq->data[idx].key = key;
    pq->data[idx].priority = priority;
    pq->data[idx].g = g;
    pq->data[idx].order = pq->pushOrder++;

    while (idx > 0) {
        int parent = (idx - 1) / 2;

        if (!pq_less(pq->data[idx], pq->data[parent])) break;

        pq_swap(&pq->data[idx], &pq->data[parent]);
        idx = parent;
    }

    return 1;
}

static int pq_empty(const PriorityQueue *pq) {
    return pq->size == 0;
}

static PQItem pq_pop(PriorityQueue *pq) {
    PQItem ret = pq->data[0];
    pq->data[0] = pq->data[--pq->size];

    int idx = 0;

    while (1) {
        int left = idx * 2 + 1;
        int right = idx * 2 + 2;
        int smallest = idx;

        if (left < pq->size && pq_less(pq->data[left], pq->data[smallest])) {
            smallest = left;
        }

        if (right < pq->size && pq_less(pq->data[right], pq->data[smallest])) {
            smallest = right;
        }

        if (smallest == idx) break;

        pq_swap(&pq->data[idx], &pq->data[smallest]);
        idx = smallest;
    }

    return ret;
}

static void intvec_init(IntVector *v) {
    v->size = 0;
    v->capacity = 128;
    v->data = (int *)malloc((size_t)v->capacity * sizeof(int));
}

static void intvec_free(IntVector *v) {
    free(v->data);
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
}

static int intvec_push(IntVector *v, int x) {
    if (v->size >= v->capacity) {
        int newCap = v->capacity * 2;
        int *newData = (int *)realloc(v->data, (size_t)newCap * sizeof(int));

        if (!newData) return 0;

        v->data = newData;
        v->capacity = newCap;
    }

    v->data[v->size++] = x;
    return 1;
}

static long long priority_for(const Board *b,
                              Algorithm alg,
                              HeuristicType hType,
                              int r,
                              int c,
                              int nextDigit,
                              long long g) {
    long long h = heuristic_value(b, r, c, nextDigit, hType);

    if (alg == ALG_UCS) return g;
    if (alg == ALG_GBFS) return h;

    return g + h;
}

static void free_search_result(SearchResult *res) {
    free(res->parent);
    free(res->parentAction);
    free(res->dist);
    intvec_free(&res->expandedTrace);
    memset(res, 0, sizeof(SearchResult));
}

static SearchResult search_solution(const Board *b, Algorithm alg, HeuristicType hType) {
    SearchResult res;
    memset(&res, 0, sizeof(SearchResult));

    int totalStates = state_count(b);
    int tc = target_count(b);

    res.parent = (int *)malloc((size_t)totalStates * sizeof(int));
    res.parentAction = (char *)malloc((size_t)totalStates * sizeof(char));
    res.dist = (long long *)malloc((size_t)totalStates * sizeof(long long));
    intvec_init(&res.expandedTrace);

    if (!res.parent || !res.parentAction || !res.dist || !res.expandedTrace.data) {
        fprintf(stderr, "Alokasi memori search gagal.\n");
        return res;
    }

    for (int i = 0; i < totalStates; i++) {
        res.parent[i] = -1;
        res.parentAction[i] = '\0';
        res.dist[i] = INF;
    }

    PriorityQueue pq;
    pq_init(&pq);

    int startKey = encode_key(b, b->startR, b->startC, 0);

    res.dist[startKey] = 0;

    long long startPriority = priority_for(b, alg, hType, b->startR, b->startC, 0, 0);
    pq_push(&pq, startKey, startPriority, 0);

    clock_t startClock = clock();

    while (!pq_empty(&pq)) {
        PQItem item = pq_pop(&pq);

        if (item.g != res.dist[item.key]) {
            continue;
        }

        int r, c, nextDigit;
        decode_key(b, item.key, &r, &c, &nextDigit);

        res.expandedCount++;
        intvec_push(&res.expandedTrace, item.key);

        if (r == b->goalR && c == b->goalC && nextDigit == tc) {
            res.found = 1;
            res.finalKey = item.key;
            res.finalCost = item.g;
            break;
        }

        for (int dir = 0; dir < 4; dir++) {
            MoveResult mv;

            if (!slide_move(b, r, c, nextDigit, dir, &mv)) {
                continue;
            }

            int nextKey = encode_key(b, mv.r, mv.c, mv.nextDigit);
            long long newG = item.g + mv.moveCost;

            if (newG < res.dist[nextKey]) {
                res.dist[nextKey] = newG;
                res.parent[nextKey] = item.key;
                res.parentAction[nextKey] = ACT[dir];

                long long pri = priority_for(b, alg, hType, mv.r, mv.c, mv.nextDigit, newG);
                pq_push(&pq, nextKey, pri, newG);
            }
        }
    }

    clock_t endClock = clock();
    res.execMs = ((double)(endClock - startClock) * 1000.0) / CLOCKS_PER_SEC;

    pq_free(&pq);
    return res;
}

static SolutionPath reconstruct_solution(const Board *b, const SearchResult *res) {
    SolutionPath path;

    path.keys = NULL;
    path.actions = NULL;
    path.moveCount = 0;

    if (!res->found) return path;

    int count = 0;
    int k = res->finalKey;

    while (res->parent[k] != -1) {
        count++;
        k = res->parent[k];
    }

    path.moveCount = count;
    path.keys = (int *)malloc((size_t)(count + 1) * sizeof(int));
    path.actions = (char *)malloc((size_t)count + 1);

    if (!path.keys || !path.actions) {
        free(path.keys);
        free(path.actions);

        path.keys = NULL;
        path.actions = NULL;
        path.moveCount = 0;
        return path;
    }

    path.actions[count] = '\0';

    k = res->finalKey;

    for (int idx = count; idx >= 1; idx--) {
        path.keys[idx] = k;
        path.actions[idx - 1] = res->parentAction[k];
        k = res->parent[k];
    }

    path.keys[0] = k;

    (void)b;
    return path;
}

static void free_solution_path(SolutionPath *path) {
    free(path->keys);
    free(path->actions);

    path->keys = NULL;
    path->actions = NULL;
    path->moveCount = 0;
}

static char display_char_at(const Board *b, int stateKey, int r, int c) {
    int pr, pc, nextDigit;
    decode_key(b, stateKey, &pr, &pc, &nextDigit);

    if (r == pr && c == pc) return 'Z';

    char ch = b->grid[r][c];

    if (ch == 'Z') return '*';

    if (is_digit_tile(ch)) {
        int d = ch - '0';

        if (d < nextDigit) return '*';
    }

    return ch;
}

static void print_board_state(FILE *out, const Board *b, int stateKey) {
    for (int i = 0; i < b->n; i++) {
        for (int j = 0; j < b->m; j++) {
            fputc(display_char_at(b, stateKey, i, j), out);
        }
        fputc('\n', out);
    }
}

static void print_solution_steps(FILE *out, const Board *b, const SolutionPath *path) {
    fprintf(out, "\nInitial\n");
    print_board_state(out, b, path->keys[0]);

    for (int i = 1; i <= path->moveCount; i++) {
        fprintf(out, "\nStep %d : %c\n", i, path->actions[i - 1]);
        print_board_state(out, b, path->keys[i]);
    }
}

static void print_expanded_trace(FILE *out, const Board *b, const SearchResult *res) {
    fprintf(out, "\nTrace konfigurasi yang ditinjau:\n");

    for (int i = 0; i < res->expandedTrace.size; i++) {
        int r, c, nextDigit;
        int key = res->expandedTrace.data[i];

        decode_key(b, key, &r, &c, &nextDigit);

        fprintf(out,
                "Iterasi %d: posisi=(%d,%d), nextDigit=%d, cost=%lld\n",
                i + 1,
                r,
                c,
                nextDigit,
                res->dist[key]);
    }
}

#ifndef _WIN32
static struct termios oldTermios;
static int rawEnabled = 0;

static void disable_raw_mode(void) {
    if (rawEnabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldTermios);
        rawEnabled = 0;
    }
}

static int enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &oldTermios) == -1) return 0;

    atexit(disable_raw_mode);

    struct termios raw = oldTermios;
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return 0;

    rawEnabled = 1;
    return 1;
}

static int read_byte_timeout(char *c, int ms) {
    fd_set set;
    struct timeval tv;

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;

    int rv = select(STDIN_FILENO + 1, &set, NULL, NULL, &tv);

    if (rv > 0) {
        return read(STDIN_FILENO, c, 1) == 1;
    }

    return 0;
}
#endif

static int playback_key(void) {
#ifdef _WIN32
    int ch = _getch();

    if (ch == 0 || ch == 224) {
        int code = _getch();

        if (code == 77) return 1;
        if (code == 75) return -1;

        return 99;
    }

    if (ch == 27) return 2;
    if (ch == 'q' || ch == 'Q') return 0;
    if (ch == 'n' || ch == 'N' || ch == 'd' || ch == 'D') return 1;
    if (ch == 'p' || ch == 'P' || ch == 'a' || ch == 'A') return -1;
    if (ch == 'j' || ch == 'J') return 2;

    return 99;
#else
    char ch;

    if (read(STDIN_FILENO, &ch, 1) != 1) return 99;

    if (ch == 27) {
        char a, b;

        if (read_byte_timeout(&a, 40)) {
            if (a == '[' && read_byte_timeout(&b, 40)) {
                if (b == 'C') return 1;
                if (b == 'D') return -1;
            }
        }

        return 2;
    }

    if (ch == 'q' || ch == 'Q') return 0;
    if (ch == 'n' || ch == 'N' || ch == 'd' || ch == 'D') return 1;
    if (ch == 'p' || ch == 'P' || ch == 'a' || ch == 'A') return -1;
    if (ch == 'j' || ch == 'J') return 2;

    return 99;
#endif
}

static void playback_solution(const Board *b, const SolutionPath *path) {
    if (!path->keys || path->moveCount < 0) return;

    int step = 0;

#ifndef _WIN32
    enable_raw_mode();
#endif

    while (1) {
        clear_screen();

        if (step == 0) {
            printf("Playback - Initial\n");
        } else {
            printf("Playback - Step %d/%d : %c\n",
                   step,
                   path->moveCount,
                   path->actions[step - 1]);
        }

        print_board_state(stdout, b, path->keys[step]);

        printf("\nPanah kanan/n/d = maju | Panah kiri/p/a = mundur | ESC/j = lompat step | q = keluar\n");
        fflush(stdout);

        int cmd = playback_key();

        if (cmd == 0) {
            break;
        }

        if (cmd == 1 && step < path->moveCount) {
            step++;
        } else if (cmd == -1 && step > 0) {
            step--;
        } else if (cmd == 2) {
#ifndef _WIN32
            disable_raw_mode();
#endif

            printf("\nMasukkan step tujuan (0-%d): ", path->moveCount);

            int target;

            if (scanf("%d", &target) == 1) {
                if (target < 0) target = 0;
                if (target > path->moveCount) target = path->moveCount;

                step = target;
            }

            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }

#ifndef _WIN32
            enable_raw_mode();
#endif
        }
    }

#ifndef _WIN32
    disable_raw_mode();
#endif
}

static int save_solution_file(const char *filename,
                              const Board *b,
                              Algorithm alg,
                              HeuristicType hType,
                              const SearchResult *res,
                              const SolutionPath *path) {
    FILE *out = fopen(filename, "w");

    if (!out) return 0;

    fprintf(out, "Algoritma: %s\n", algorithm_name(alg));
    fprintf(out, "Heuristic: %s\n", heuristic_name(hType));

    if (res->found) {
        fprintf(out, "Solusi Yang Ditemukan: %s\n", path->actions);
        fprintf(out, "Cost dari Solusi: %lld\n", res->finalCost);
        fprintf(out, "Waktu eksekusi: %.3f ms\n", res->execMs);
        fprintf(out, "Banyak iterasi yang dilakukan: %lld iterasi\n", res->expandedCount);

        print_solution_steps(out, b, path);
    } else {
        fprintf(out, "Solusi tidak ditemukan.\n");
        fprintf(out, "Waktu eksekusi: %.3f ms\n", res->execMs);
        fprintf(out, "Banyak iterasi yang dilakukan: %lld iterasi\n", res->expandedCount);
    }

    print_expanded_trace(out, b, res);

    fclose(out);
    return 1;
}

static void print_heuristic_info(void) {
    printf("\nPilihan heuristic:\n");
    printf("H1 = Manhattan distance ke target berikutnya.\n");
    printf("H2 = H1 dikali cost tile minimum.\n");
    printf("H3 = Estimasi rantai: posisi sekarang -> digit tersisa -> tujuan, dikali cost minimum.\n");
}

int main(void) {
    char inputPath[MAX_PATH_LEN];
    char algStr[32];
    char hStr[32];

    Algorithm alg;
    HeuristicType hType = HEUR_H1;

    Board board;
    char err[512];

    printf(">> Masukan file input:\n   ");

    if (scanf("%255s", inputPath) != 1) {
        printf("Input path tidak valid.\n");
        return 1;
    }

    if (!load_board(inputPath, &board, err, sizeof(err))) {
        printf("Input tidak valid: %s\n", err);
        return 1;
    }

    printf("\nPapan awal:\n");
    print_board_state(stdout, &board, encode_key(&board, board.startR, board.startC, 0));

    printf("\n>> Algoritma apa yang anda pilih? (UCS/GBFS/A*)\n   ");

    if (scanf("%31s", algStr) != 1 || !parse_algorithm(algStr, &alg)) {
        printf("Algoritma tidak valid. Gunakan UCS, GBFS, atau A*.\n");
        free_board(&board);
        return 1;
    }

    if (alg == ALG_GBFS || alg == ALG_ASTAR) {
        print_heuristic_info();

        printf(">> Heuristic apa yang anda pilih? (H1/H2/H3)\n   ");

        if (scanf("%31s", hStr) != 1 || !parse_heuristic(hStr, &hType)) {
            printf("Heuristic tidak valid. Gunakan H1, H2, atau H3.\n");
            free_board(&board);
            return 1;
        }
    } else {
        strcpy(hStr, "H1");
        hType = HEUR_H1;
    }

    SearchResult result = search_solution(&board, alg, hType);
    SolutionPath path = reconstruct_solution(&board, &result);

    if (result.found && path.keys) {
        printf("\nSolusi Yang Ditemukan : %s\n", path.actions);
        printf("Cost dari Solusi : %lld\n", result.finalCost);

        print_solution_steps(stdout, &board, &path);
    } else {
        printf("\nSolusi tidak ditemukan.\n");
    }

    printf("\n>> Waktu eksekusi: %.3f ms\n", result.execMs);
    printf(">> Banyak iterasi yang dilakukan: %lld iterasi\n", result.expandedCount);

    if (result.found && path.keys) {
        char ans[32];

        printf(">> Apakah Anda ingin melakukan playback? (Ya/Tidak):\n   ");

        if (scanf("%31s", ans) == 1 && yes_answer(ans)) {
            playback_solution(&board, &path);
        }
    }

    char saveAns[32];

    printf("\n>> Apakah Anda ingin menyimpan solusi? (Ya/Tidak):\n   ");

    if (scanf("%31s", saveAns) == 1 && yes_answer(saveAns)) {
        char outName[MAX_PATH_LEN];
        char resolvedOutPath[MAX_PATH_LEN * 2];

        printf(">> Masukkan nama file output, atau '-' untuk solusi.txt di folder test:\n   ");

        if (scanf("%255s", outName) == 1) {
            if (strcmp(outName, "-") == 0) {
                strcpy(outName, "solusi.txt");
            }

            resolve_output_path(outName, resolvedOutPath, sizeof(resolvedOutPath));

            if (save_solution_file(resolvedOutPath, &board, alg, hType, &result, &path)) {
                printf(">> Solusi disimpan pada %s\n", resolvedOutPath);
            } else {
                printf(">> Gagal menyimpan solusi ke %s\n", resolvedOutPath);
            }
        }
    }

    free_solution_path(&path);
    free_search_result(&result);
    free_board(&board);

    return 0;
}