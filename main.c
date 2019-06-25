
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
        {3, 4, 5},
        {2, 3, 5},
        {0, 1, 3},
        {0, 1, 2},
        {0, 3, 5}
};

int *my_resources;
int *control_token = NULL;
const int elected_node = 0;
int *needed_resources;
int *using_resources;
int **queue;
int self, parent, next, using_control_token, all_finalized, n_procs;

int has_all_res() {
    for (int i = 0; i < n_rec; ++i) {
        if (needed_resources[i] && !my_resources[i]) {
            return 0;
        }
    }
    return 1;
}

void init() {
    printf("Iniciando o nó %d PID: %d\n", self, getpid());
    all_finalized = 0;
    using_control_token = 0;
    my_resources = malloc(sizeof(int) * n_rec);
    needed_resources = malloc(sizeof(int) * n_rec);
    using_resources = malloc(sizeof(int) * n_rec);
    queue = malloc(sizeof(int *) * n_procs);
    for (int k = 0; k < n_rec; ++k) {
        my_resources[k] = 0;
        needed_resources[k] = 0;
        using_resources[k] = 0;
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


void send_ack(int response[3], int requester, int inq_type) {
    printf("%d: enviando para %d os tokens", self, requester);
    for (int j = 0; j < 3; ++j) {
        if (response[j] == -1) break;
        printf(" %d", response[j]);
    }
    printf("\n");
    MPI_Send(response, 3, MPI_INT, requester, inq_type, MPI_COMM_WORLD);
}


void send_request_token(int origin, int parent) {
    MPI_Send(&origin, 1, MPI_INT, parent, REQUEST_CONTROL_TOKEN, MPI_COMM_WORLD);
}

void receive_ack(int tokens[3], int sender, int send_inq[n_procs][3]) {
    for (int i = 0; i < 3; ++i) {
        int value = tokens[i];
        if (value ==-1) break;
        my_resources[value] = 1;
        using_resources[value] = 1;
        for (int j = 0; j < 3; ++j) {
            if (send_inq[sender][j] == value) {
                send_inq[sender][j] = -1;
            }
        }
    }

}

void send_token() {
    printf("%d: Enviando CONTROL_TOKEN para %d\n", self, next);
    MPI_Send(control_token, n_rec, MPI_INT, next, CONTROL_TOKEN_MESSAGE, MPI_COMM_WORLD);
    next = -1;
    control_token = NULL;
}

void receive_token_request(int origin) {
    printf("%d: Recebi pedido de control Token", self);
    if (parent != -1) {
        printf(" encaminhando para meu parent %d\n", parent);
        send_request_token(origin, parent);
        parent = origin;
    } else {
        printf(" não tenho parent ");
        parent = origin;
        next = origin;
        if (control_token && !using_control_token) {
            printf("tenho o token e não o estou usando, enviar para %d\n", origin);
            send_token();
        } else
            printf("estou usando o token o entregarei quando terminar | next=%d parent=%d\n", next, parent);
    }
}

void receive_inquire(int tokens[3], int requester) {
    int response[3] = {-1, -1, -1};
    int count = 0;
    for (int i = 0; i < 3; ++i) {
        int value = tokens[i];
        if (value != -1) {
            if (!using_resources[value]) {
                printf("%d: colocando %d no ack1\n", self, value);
                response[count] = value; //coloca na resposta pro ack1 os tokens que não to usando
                my_resources[value] = 0;
                count++;
            } else {
                printf("%d: colocando %d no Q para enviar depois\n", self, value);
                for (int j = 0; j < n_procs; ++j) {
                    if (queue[requester][j] == -1) {
                        queue[requester][j] = value; //coloca em Q os que eu to usando pra ir pro ack2
                        break;
                    }
                }
            }

        }
    }
    send_ack(response, requester, INQ_ACK1);
}

void *listen_rec(void *args) {
    while (all_finalized < n_procs) {
//        printf("%d: Estou esperando pedido de token; all_finalized=%d\n", self, all_finalized);
        MPI_Status st;
        int received;
        MPI_Recv(&received, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST_CONTROL_TOKEN, MPI_COMM_WORLD, &st);
//        printf("%d: recebi de %d \n", self, st.MPI_SOURCE);
        if (received != -1)
            receive_token_request(received);
    }
    return NULL;
}

void *listen_inq(void *args) {
    while (all_finalized < n_procs) {
//        printf("%d: Estou esperando inquire; all_finalized=%d\n", self, all_finalized);
        MPI_Status st;
        int *received = malloc(sizeof(int) * 3);
        MPI_Recv(received, 3, MPI_INT, MPI_ANY_SOURCE, INQ, MPI_COMM_WORLD, &st);
        if (received[0] != -1)
            receive_inquire(received, st.MPI_SOURCE);
    }
    return NULL;
}

void *listen_fin(void *args) {
    while (all_finalized < n_procs) {
//        printf("%d: Estou esperando finalize; all_finalized=%d\n", self, all_finalized);
        MPI_Status st;
        int received = 0;
        MPI_Recv(&received, 1, MPI_INT, MPI_ANY_SOURCE, FINALIZE_MESSAGE, MPI_COMM_WORLD, &st);
        all_finalized = all_finalized + 1;
        printf("%d: enviando ack de finalize para %d\n", self, st.MPI_SOURCE);
        MPI_Send(&all_finalized, 1, MPI_INT, st.MPI_SOURCE, ACK_FINALIZE, MPI_COMM_WORLD);
    }
    return NULL;
}

void send_finalize() {
    for (int to = 0; to < n_procs; to++) {
        printf("%d: Enviando finalize pra %d\n", self, to);
        MPI_Send(&self, 1, MPI_INT, to, FINALIZE_MESSAGE, MPI_COMM_WORLD);
        int meh = 0;
        MPI_Recv(&meh, 1, MPI_INT, to, ACK_FINALIZE, MPI_COMM_WORLD, NULL);
        printf("%d: recebi ack finalize pra %d\n", self, to);
        int void_value = -1;
//        printf("Enviando fake req pra %d\n", to);
        MPI_Send(&void_value, 1, MPI_INT, to, REQUEST_CONTROL_TOKEN, MPI_COMM_WORLD);
        int void_array[3] = {void_value, void_value, void_value};
//        printf("Enviando fake inq pra %d\n", to);
        MPI_Send(void_array, 3, MPI_INT, to, INQ, MPI_COMM_WORLD);
    }
}

void use_resources(int requested[3]) {
    printf("%d: quer usar os recursos:", self);
    for (int i = 0; i < 3; ++i) {
        printf(" %d", requested[i]);
        needed_resources[requested[i]] = 1;
    }
    printf("\n");
    if (!has_all_res(requested)) { //se ele não possui todos os recursos
        if (!control_token) {
            printf("%d: Pedindo token para %d\n", self, parent);
            MPI_Send(&self, 1, MPI_INT, parent, REQUEST_CONTROL_TOKEN, MPI_COMM_WORLD);
            control_token = malloc(sizeof(int) * n_rec);
            MPI_Status st;
            parent = -1;
            MPI_Recv(control_token, n_rec, MPI_INT, MPI_ANY_SOURCE, CONTROL_TOKEN_MESSAGE, MPI_COMM_WORLD, &st);
            printf("%d: Recebi o control token de %d\n", self, st.MPI_SOURCE);
        }
        using_control_token = 1;
        int send_inq[n_procs][3];
        for (int k = 0; k < n_procs; ++k) {
            for (int i = 0; i < 3; ++i) {
                send_inq[k][i] = -1;
            }
        }
        for (int i = 0; i < n_rec; ++i) {
            if (needed_resources[i] && my_resources[i]) {
                printf("%d: token %d é necessário e já o tenho localmente\n", self, i);
                using_resources[i] = 1;
            }
            if (needed_resources[i] && !my_resources[i]) {//pegue todos os tokens que precisa e não tem
                if (control_token[i] == -1) {
                    printf("%d: token %d é necessário e está no control token\n", self, i);
                    my_resources[i] = 1;
                    using_resources[i] = 1;
                    control_token[i] = self;
                } else {
                    int owner = control_token[i];
                    printf("%d: token %d é necessário e está em posse de %d\n", self, i, owner);
                    for (int j = 0; j < 3; ++j) {
                        if (send_inq[owner][j] == -1) {
                            send_inq[owner][j] = i;
                            break;
                        }
                    }
                }
            } else if (my_resources[i] &&
                       (!using_resources[i] || !needed_resources[i])) {//devolva todos os que tem e não precisa
                if (control_token[i] == self) {
                    printf("%d: devolvendo token %d para o control_token\n", self, i);
                    my_resources[i] = 0;
                    control_token[i] = -1;
                }
            }
        }
        if (!has_all_res()) {
            printf("%d: Control token nao tinha todos\n", self);
            for (int i = 0; i < n_procs; ++i) {
                if (send_inq[i][0] != -1) {
                    printf("%d: pedindo para %d os tokens", self, i);
                    for (int j = 0; j < 3; ++j) {
                        if (send_inq[i][j] == -1) break;
                        printf(" %d", send_inq[i][j]);
                    }
                    printf("\n");
                    MPI_Send(send_inq[i], 3, MPI_INT, i, INQ, MPI_COMM_WORLD);
                    int *received = malloc(sizeof(int) * 3);
                    MPI_Status st;
                    printf("%d: esperando de %d o INQ_ACK1\n", self, i);
                    MPI_Recv(received, 3, MPI_INT, i, INQ_ACK1, MPI_COMM_WORLD, &st);
                    printf("%d: recebi de %d os tokens", self, i);
                    for (int j = 0; j < 3; ++j) {
                        if (received[j] == -1) break;
                        control_token[received[j]] = self;
                        printf(" %d", received[j]);
                    }
                    printf("\n");
                    receive_ack(received, st.MPI_SOURCE, send_inq);
                }
            }
        }
        using_control_token = 0;
        if (control_token && next != -1) {
            send_token();
        }
        if (!has_all_res()) {
            printf("%d: INQ_ACK1 não trouxe todas\n", self);
            for (int i = 0; i < n_procs; ++i) {
                if (send_inq[i][0] != -1) {
                    int received[3];
                    MPI_Status st;
                    printf("%d: esperando INQ_ACK2 de %d\n", self, i);
                    MPI_Recv(received, 3, MPI_INT, i, INQ_ACK2, MPI_COMM_WORLD, &st);
                    printf("%d: recebi ACK2 de %d\n", self, i);
                }
            }
        }

    }
    using_control_token = 0;
    printf("%d: Entrei seção crítica\n", self);
    //seção crítica
    usleep((__useconds_t) ((rand() % (4)) * 1000000));
    printf("%d: Saindo da seção crítica\n", self);
    for (int m = 0; m < n_rec; ++m) {
        needed_resources[m] = 0;
        using_resources[m] = 0;
    }
    for (int l = 0; l < n_procs; ++l) {
        printf("%d: Q_%d: [%d,%d,%d]\n", self, l, queue[l][0], queue[l][1], queue[l][2]);
        if (queue[l][0] != -1) {
            send_ack(queue[l], l, INQ_ACK2);
        }
    }
    if (control_token && next != -1) {
        send_token();
    }
}

void execute() {//while gera um numero entre 0 e total_procs se gerou "meu numero" faço o pedido, dorme por um segundo
    int count = 0;
    while (count != 2) {
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
    int err = 0;
    srand(rand());
    pthread_t rec_thread, inq_thread, fin_thread;
    int given;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &given);
    if (given != MPI_THREAD_MULTIPLE) {
        printf("Error, cannot be multithread");
    }
    //inicializa

    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &self);
    init();


    //thread pra controle de finalize
    err = pthread_create(&fin_thread, NULL, listen_fin, NULL);
    if (err != 0) {
        printf("ERROR: ERR=%d", err);
    }
    //levanta uma thread para ficar escutando requests de control token
    err = pthread_create(&rec_thread, NULL, listen_rec, NULL);
    if (err != 0) {
        printf("ERROR: ERR=%d", err);
    }
    //levanta uma thread para ficar escutando request de INQUIRE
    err = pthread_create(&inq_thread, NULL, listen_inq, NULL);
    if (err != 0) {
        printf("ERROR: ERR=%d", err);
    }

    //executa
    execute();

    //finalize
    send_finalize();

    pthread_join(fin_thread, NULL);
    pthread_join(rec_thread, NULL);
    pthread_join(inq_thread, NULL);
//
    printf("%d: TERMINEI\n", self);
    MPI_Finalize();
    return 0;
}