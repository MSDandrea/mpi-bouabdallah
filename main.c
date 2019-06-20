#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

const int n_rec = 6; //recursos: 0 1 2 3 4 5
int *control_token = NULL;
int my_tokens[n_rec];
int **queue;
const int elected_node = 0;
enum messages {
    REQUEST_MESSAGE = 0,
    CONTROL_TOKEN_MESSAGE = 1,
    INQ = 2,
    INQ_ACK1 = 3,
    INQ_ACK2 = 4,
    FINALIZE_MESSAGE = 9
};

int self, parent, next, requesting, all_finalized, n_procs;
pthread_mutex_t access_mutex;

void init() {
    printf("Iniciando o nó %d\n", self);
    all_finalized = 0;
    requesting = 0;
    pthread_mutex_init(&access_mutex, NULL);
    queue = malloc(sizeof(int *) * n_procs);
    for (int k = 0; k < n_rec; ++k) {
        my_tokens[k] = 0;
    }
    for (int i = 0; i < n_procs; ++i) {
        queue[i] = malloc(sizeof(int) * n_rec);
        for (int j = 0; j < n_rec; ++j) {
            queue[i][j] = -1;
        }
    }
    if (self == elected_node) {
        control_token = malloc(sizeof(int) * n_rec);
        for (int i = 0; i < n_rec; ++i) {
            control_token[i] = -1;
        }
        parent = -1;
    } else {
        pthread_mutex_lock(&access_mutex);
        parent = elected_node;
    }
    next = -1;
}

void release_cs();

void send_token(int to);

void send_request(int origin, int to);

void send_finalize();

void receive_request_cs(int sj);

void receive_token();

void request_cs();

void *receive(void *nil);

void request_cs() {
    requesting = 1;
    printf("Nó %d quer entrar na seção crítico.\n", self);
    if (parent != -1) {
        send_request(self, parent);
        parent = -1;

        //espera token
        pthread_mutex_lock(&access_mutex);

    }
    int random_sleep = (rand() % (3)) * 1000000;
    printf("Nó %d entrou na seção irá dormir por %d segundos\n", self, random_sleep / 1000000);
    usleep((__useconds_t) random_sleep);
    release_cs();
}


int has_all_res(int requested[n_rec]) {
    for (int i = 0; i < n_rec && requested[i] != -1; ++i) {
        requested
    }
}

void request_resources(int requested[n_rec]) {
    printf("Nó %d quer usar os recursos:", self);
    for (int i = 0; i < n_rec && requested[i] != -1; ++i) {
        printf(" %d", requested[i]);
    }
    printf("\n");
    if (!has_all_res(requested)){

    }
    //seção crítica

}

void execute() {//while gera um numero entre 0 e total_procs se gerou "meu numero" faço o pedido, dorme por um segundo
    int count = 0;
    while (count != 3) {
        int random = (rand() % (n_procs));
        if (self == random) {
            int base_req = (rand() % (n_rec)); //primeiro recurso
            int top_req = (rand() % (n_rec)); //ultimo recurso
            top_req = top_req <= base_req ? base_req + 1 : top_req; // se top_req é menor que a base então ele é base +1
            int requested_res[n_rec]; // array de recursos que eu quero
            int j = 0;
            for (int i = base_req; i < top_req; ++i) {
                requested_res[j] = i; // quero o iésimo recurso a partir da base
                j++;
            }
            requested_res[j] = -1; //para de procurar recurso
            request_resources(requested_res);
            count++;
        }
        usleep(1 * 1000000);
    }
}

int main(int argc, char **argv) {
    srand(rand());
    pthread_t rec_thread;
    MPI_Init(&argc, &argv);

    //inicializa
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &self);
    init();

    //levanta uma thread para ficar escutando requests
    pthread_create(&rec_thread, NULL, receive, NULL);

    //executa
    execute();

    //finaliza
    send_finalize();
    pthread_join(rec_thread, NULL);
    MPI_Finalize();
    return 0;
}


void receive_token() {
    pthread_mutex_unlock(&access_mutex);
    printf("Nó %d recebeu o token.\n", self);
}

void receive_finalize() {
    all_finalized++;
}

void receive_request_cs(int sj) {
    if (parent == -1) {
        if (requesting) {
            printf("%d é next de %d\n", sj, self);
            next = sj;
        } else {
            send_token(sj);
        }
    } else {
        send_request(sj, parent);
    }
    parent = sj;
    printf(" Owner de %d é %d\n", self, parent);
}

void release_cs() {
    printf("Nó %d liberando seção crítica\n", self);
    pthread_mutex_unlock(&access_mutex);
    requesting = 0;
    if (next != -1) {
        send_token(next);
        pthread_mutex_lock(&access_mutex);
        next = -1;
    }
}

//função pra thread de receive
void *receive(void *nil) {
    while (all_finalized != n_procs) {
        MPI_Status st;
        int received;
        MPI_Recv(&received, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
        printf("Look! Nó %d recebeu mensagem %d originária do nó %d através do nó %d\n", self, st.MPI_TAG,
               received, st.MPI_SOURCE);
        if (st.MPI_TAG == REQUEST_MESSAGE) {
            receive_request_cs(received);
        } else if (st.MPI_TAG == CONTROL_TOKEN_MESSAGE) {
            receive_token();
        } else if (st.MPI_TAG == FINALIZE_MESSAGE) {
            receive_finalize();
        } else {
            printf("ERROR UNKNOWN MESSAGE RECEIVED: %c", st.MPI_TAG);
        }
    }
}

void send_finalize() {
    for (int to = 0; to < n_procs; to++) {
        MPI_Send(&self, 1, MPI_INT, to, FINALIZE_MESSAGE, MPI_COMM_WORLD);
    }
}

void send_token(int to) {
    MPI_Send(&self, 1, MPI_INT, to, CONTROL_TOKEN_MESSAGE, MPI_COMM_WORLD);
}

void send_request(int origin, int to) {
    MPI_Send(&origin, 1, MPI_INT, to, REQUEST_MESSAGE, MPI_COMM_WORLD);
}
