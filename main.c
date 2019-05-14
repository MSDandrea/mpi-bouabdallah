#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

const int elected_node = 0;
enum messages {
    REQUEST_MESSAGE = 0,
    TOKEN_MESSAGE = 1,
    FINALIZE_MESSAGE = 2
};

int self, owner, next, requesting, all_finalized, total_nodes;
pthread_mutex_t token;

void release_cs();

void send_token(int to);

void send_request(int origin, int to);

void send_finalize();

void receive_request_cs(int sj);

void receive_token();

void request_cs();

void *receive(void *nil);


int main(int argc, char **argv) {
    srand(rand());
    pthread_t rec_thread;
    MPI_Init(&argc, &argv);

    //initialize
    MPI_Comm_size(MPI_COMM_WORLD, &total_nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &self);
    printf("Olá sou o nó %d\n", self);
    all_finalized = 0;
    requesting = 0;
    pthread_mutex_init(&token, NULL);
    next = -1; //usando -1 como NULL para nós
    if (self == elected_node) {
        owner = -1;
    } else {
        pthread_mutex_lock(&token);
        owner = elected_node;
    }

    //levanta uma thread para ficar escutando requests
    pthread_create(&rec_thread, NULL, receive, NULL);

    //while gera um numero entre 0 e 4 se gerou "meu numero" faço o pedido, dorme por um segundo
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
        int received;
        MPI_Recv(&received, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
        printf("Look! Nó %d recebeu mensagem %d originária do nó %d através do nó %d\n", self, st.MPI_TAG,
               received, st.MPI_SOURCE);
        if (st.MPI_TAG == REQUEST_MESSAGE) {
            receive_request_cs(received);
        } else if (st.MPI_TAG == TOKEN_MESSAGE) {
            receive_token();
        } else if (st.MPI_TAG == FINALIZE_MESSAGE) {
            receive_finalize();
        } else {
            printf("ERROR UNKNOWN MESSAGE RECEIVED: %c", st.MPI_TAG);
        }
    }
}

void send_finalize() {
    for (int to = 0; to < total_nodes; to++) {
        MPI_Send(&self, 1, MPI_INT, to, FINALIZE_MESSAGE, MPI_COMM_WORLD);
    }
}

void send_token(int to) {
    MPI_Send(&self, 1, MPI_INT, to, TOKEN_MESSAGE, MPI_COMM_WORLD);
}

void send_request(int origin, int to) {
    MPI_Send(&origin, 1, MPI_INT, to, REQUEST_MESSAGE, MPI_COMM_WORLD);
}
