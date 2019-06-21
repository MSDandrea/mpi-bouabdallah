
#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

enum messages {
    REQUEST_CONTROL_TOKEN = 0,
    CONTROL_TOKEN_MESSAGE = 1,
    INQ = 2,
    INQ_ACK1 = 3,
    INQ_ACK2 = 4,
    FINALIZE_MESSAGE = 9,
    ACK_FINALIZE = 10
};


const int n_rec = 6; //recursos: 0,1,2,3,4,5
int pv[7][3] = {
        {0, 2, 4},
        {1, 3, 5},
        {6, 4, 5},
        {2, 3, 5},
        {0, 1, 3},
        {0, 1, 2},
        {0, 3, 5}
};

int *my_tokens;
int *control_token = NULL;
const int elected_node = 0;
int *needed_tokens;
int **queue;
int n_proc;
int self, parent, next, using_token, all_finalized, n_procs;
pthread_mutex_t access_mutex;

int has_all_res() {
    for (int i = 0; i < n_rec; ++i) {
        if (needed_tokens[i] && !my_tokens[i])
            return 0;
    }
    return 1;
}

void init() {
    printf("Iniciando o nó %d\n", self);
    all_finalized = 0;
    using_token = 0;
//    pthread_mutex_init(&access_mutex, NULL);
    my_tokens = malloc(sizeof(int) * n_rec);
    needed_tokens = malloc(sizeof(int) * n_rec);
    queue = malloc(sizeof(int *) * n_procs);
    for (int k = 0; k < n_rec; ++k) {
        my_tokens[k] = 0;
        needed_tokens[k] = 0;
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
//        pthread_mutex_lock(&access_mutex);
        parent = elected_node;
    }
    next = -1;
    printf("terminei de inicializar nó %d\n", self);
}


void send_ack(int response[3], int requester) {

}


void send_request_token(int origin, int parent) {

}

void send_token(int requirer) {
    MPI_Send(&control_token, n_rec, MPI_INT, requirer, CONTROL_TOKEN_MESSAGE, MPI_COMM_WORLD);
}

void receive_token_request(int origin) {
    if (parent != -1) {
        send_request_token(origin, parent);
        parent = origin;
    } else {
        parent = origin;
        next = origin;
        if (!using_token) {
            send_token(origin);
        }
    }
}

void receive_inquire(int tokens[3], int requester) {
    int response[3] = {-1, -1, -1};
    int count = 0;
    for (int i = 0; i < 3; ++i) {
        int value = tokens[i];
        if (value != -1) {
            if (!needed_tokens[i])
                response[count] = value; //coloca na resposta pro ack1 os tokens que não to usando
            else {
                for (int j = 0; j < n_proc; ++j) {
                    if (queue[requester][j] == -1) {
                        queue[requester][j] = value; //coloca em Q os que eu to usando pra ir pro ack2
                    }
                }
            }

        }
    }
    send_ack(response, requester);
}

void *listen_rec() {
    while (all_finalized != n_procs) {
        MPI_Status st;
        int received;
        MPI_Recv(&received, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST_CONTROL_TOKEN, MPI_COMM_WORLD, &st);
        printf("recebi de %d o valor %d\n", st.MPI_SOURCE, received);
        if (received != -1)
            receive_token_request(received);
    }
    return NULL;
}

void *listen_inq() {
    while (all_finalized != n_procs) {
        MPI_Status st;
        int *received = malloc(sizeof(int) * 3);
        MPI_Recv(received, 3, MPI_INT, MPI_ANY_SOURCE, INQ, MPI_COMM_WORLD, &st);
        if (received[0] != -1)
            receive_inquire(received, st.MPI_SOURCE);
    }
}

void *listen_fin() {
    while (all_finalized < n_procs) {
        MPI_Status st;
        int received;
        MPI_Recv(&received, 1, MPI_INT, MPI_ANY_SOURCE, FINALIZE_MESSAGE, MPI_COMM_WORLD, &st);
//        printf("Recebi finalize de %d\n", st.MPI_SOURCE);
        all_finalized++;
        MPI_Send(&all_finalized, 1, MPI_INT, st.MPI_SOURCE, ACK_FINALIZE, MPI_COMM_WORLD);
    }
    return NULL;
}

void send_finalize() {
    for (int to = 0; to < n_procs; to++) {
        printf("%d: Enviando finalize pra %d\n", self, to);
        MPI_Send(&self, 1, MPI_INT, to, FINALIZE_MESSAGE, MPI_COMM_WORLD);
        int meh;
        MPI_Recv(&meh, 1, MPI_INT, to, ACK_FINALIZE, MPI_COMM_WORLD, NULL);
        printf("%d: recebi ack finalize pra %d\n", self, to);
        int void_value = -1;
        printf("Enviando fake req pra %d\n", to);
        MPI_Send(&void_value, 1, MPI_INT, to, REQUEST_CONTROL_TOKEN, MPI_COMM_WORLD);
        int void_array[3] = {void_value, void_value, void_value};
        printf("Enviando fake inq pra %d\n", to);
        MPI_Send(&void_array, 3, MPI_INT, to, INQ, MPI_COMM_WORLD);
    }
}

void use_resources(int requested[3]) {
    printf("Nó %d quer usar os recursos:", self);
    for (int i = 0; i < 3; ++i) {
        printf(" %d", requested[i]);
        needed_tokens[requested[i]] = 1;
    }
    printf("\n");
    if (!has_all_res(requested)) { //se ele não possui todos os recursos
        MPI_Send(&self, 1, MPI_INT, parent, REQUEST_CONTROL_TOKEN, MPI_COMM_WORLD);
        control_token = malloc(sizeof(int) * n_rec);
        MPI_Status st;
        MPI_Recv(&control_token, n_rec, MPI_INT, MPI_ANY_SOURCE, CONTROL_TOKEN_MESSAGE, MPI_COMM_WORLD, &st);
        using_token = 1;
        for (int i = 0; i < n_rec; ++i) {
            if (needed_tokens[i] && control_token[i] == -1) {//pegue todos os tokens que precisa e não tem
                control_token[i] = self;
                my_tokens[i] = 1;
            } else if (control_token[i] == self && !needed_tokens[i]) {//devolva todos os que tem e não precisa
                my_tokens[i] = 0;
                control_token[i] = -1;
            }
        }
        if (!has_all_res()){

        }

    }
    if (control_token && next != -1) {
        //envia control token
    }
}

void execute() {//while gera um numero entre 0 e total_procs se gerou "meu numero" faço o pedido, dorme por um segundo
    int count = 0;
    while (count != 3) {
        int random = (rand() % (n_procs));
        if (self == random) {
            int rand_idx = (rand() % (7));
            use_resources(pv[rand_idx]);
            count++;
        }
        usleep(1 * 1000000);
    }
}


int main(int argc, char **argv) {
    srand(rand());
    pthread_t rec_thread, inq_thread, fin_thread;
    MPI_Init(&argc, &argv);

    //inicializa
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &self);
    init();
    MPI_Barrier(MPI_COMM_WORLD);

    //thread pra controle de finalize
    pthread_create(&fin_thread, NULL, listen_fin, NULL);
    //levanta uma thread para ficar escutando requests de control token
    pthread_create(&rec_thread, NULL, listen_rec, NULL);
    //levanta uma thread para ficar escutando request de INQUIRE
    pthread_create(&inq_thread, NULL, listen_inq, NULL);

    //executa
    execute();

    //finaliza
    send_finalize();

    pthread_join(fin_thread, NULL);
    pthread_join(rec_thread, NULL);
    pthread_join(inq_thread, NULL);
    MPI_Finalize();
    return 0;
}