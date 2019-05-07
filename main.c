#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

const int elected_node = 0;
const unsigned char REQUEST_MESSAGE = 'R';
const unsigned char TOKEN_MESSAGE = 'T';
const unsigned char FINALIZE_MESSAGE = 'F';

int self, owner, next, requesting, all_finalized, total_nodes;
pthread_mutex_t token;
MPI_Datatype mpi_req;

void release_cs();

void send_token(int to);

void send_request(int origin, int to);

void send_finalize();

void receive_request_cs(int sj);

void receive_token();

void request_cs();

void *receive(void *nil);

typedef struct {
    unsigned char type;
    int origin;
} request;

int main(int argc, char **argv) {
    srand(rand());
    pthread_t rec_thread;
    MPI_Init(&argc, &argv);

    /* Define um novo type no mpi para lidar com o request */
    const int nitems = 2;
    int blocklengths[2] = {1, 1};
    MPI_Datatype types[2] = {MPI_UNSIGNED_CHAR, MPI_INT};
    MPI_Aint offsets[2];

    offsets[0] = offsetof(request, type);
    offsets[1] = offsetof(request, origin);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_req);
    MPI_Type_commit(&mpi_req);

    //initialize
    MPI_Comm_size(MPI_COMM_WORLD, &total_nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &self);
    printf("Olá sou o nó %d\n", self);
    all_finalized = 0;
    requesting = 0;
    pthread_mutex_init(&token,NULL);
    next = -1; //usando -1 como NULL para nós
    if (self == elected_node) {
        owner = -1;
    } else {
        pthread_mutex_lock(&token);
        owner = elected_node;
    }

    //levanta uma thread para ficar escutando requests
    pthread_create(&rec_thread, NULL, receive, NULL);

    //while true gera um numero entre 0 e 4 se gerou "meu numero" faço o pedido, dorme por um segundo
    int count = 0;
    while (count != 3) {
        int random = (rand() % (5));
        if (self == random) {
            request_cs();
            count++;
        }
        usleep(1 * 1000000);
    }
    send_finalize();
    pthread_join(rec_thread, NULL);
    MPI_Finalize();
    return 0;
}

void request_cs() {
    requesting = 1;
    printf("Nó %d quer entrar na seção crítico.\n", self);
    if (owner != -1) {
        send_request(self, owner);
        owner = -1;

        //espera token
        pthread_mutex_lock(&token);

    }
    int random_sleep = (rand() % (3)) * 1000000;
    printf("Nó %d entrou na seção irá dormir por %d segundos\n", self, random_sleep / 1000000);
    usleep((__useconds_t) random_sleep);
    release_cs();
}

void receive_token() {
    pthread_mutex_unlock(&token);
    printf("Nó %d recebeu o token.\n", self);
}

void receive_finalize() {
    all_finalized++;
}

void receive_request_cs(int sj) {
    if (owner == -1) {
        if (requesting) {
            printf("%d é next de %d\n", sj, self);
            next = sj;
        } else {
            send_token(sj);
        }
    } else {
        send_request(sj, owner);
    }
    owner = sj;
    printf(" Owner de %d é %d\n", self, owner);
}

void release_cs() {
    printf("Nó %d liberando seção crítica\n", self);
    pthread_mutex_unlock(&token);
    requesting = 0;
    if (next != -1) {
        send_token(next);
        pthread_mutex_lock(&token);
        next = -1;
    }
}

//função pra thread de receive
void *receive(void *nil) {
    while (all_finalized != total_nodes) {
        MPI_Status st;
        request received;
        MPI_Recv(&received, 1, mpi_req, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
        printf("Look! Nó %d recebeu mensagem %c originária do nó %d através do nó %d\n", self, received.type,
               received.origin, st.MPI_SOURCE);
        if (received.type == REQUEST_MESSAGE) {
            receive_request_cs(received.origin);
        } else if (received.type == TOKEN_MESSAGE) {
            receive_token();
        } else if (received.type == FINALIZE_MESSAGE){
            receive_finalize();
        }
        else {
            printf("ERROR UNKNOWN MESSAGE RECEIVED: %c", received.type);
        }
    }
}

void send_finalize() {
    request req;
    req.type = FINALIZE_MESSAGE;
    req.origin = self;
    for (int to = 0; to < total_nodes; to++) {
        MPI_Send(&req, 1, mpi_req, to, 0, MPI_COMM_WORLD);
    }
}

void send_token(int to) {
    request req;
    req.type = TOKEN_MESSAGE;
    req.origin = self;
    MPI_Send(&req, 1, mpi_req, to, 0, MPI_COMM_WORLD);
}

void send_request(int origin, int to) {
    request req;
    req.type = REQUEST_MESSAGE;
    req.origin = origin;
    MPI_Send(&req, 1, mpi_req, to, 0, MPI_COMM_WORLD);
}
