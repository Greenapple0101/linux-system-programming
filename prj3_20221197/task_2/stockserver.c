/*
 * stockserver_threaded.c - Thread-based Concurrent Stock Server (Task2)
 * Usage: ./stockserver_threaded <port>
 * Implements a thread pool with shared buffer to handle multiple clients concurrently.
 * Commands: show, buy <ID> <N>, sell <ID> <N>, exit
 */

#include "csapp.h"
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <string.h>

/* ===== Data Structures ===== */
typedef struct item {
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;        /* for fine-grained locking */
} item_t;

typedef struct node {
    item_t data;
    struct node *left, *right;
} node_t;

/* Shared buffer for available connections */
typedef struct {
    int *buf;
    int n;
    int front, rear;
    sem_t mutex, slots, items;
} sbuf_t;

/* ===== Global Variables ===== */
static node_t *root = NULL;    /* stock BST root */
static sbuf_t sbuf;            /* shared buffer */

/* ===== Function Prototypes ===== */
void load_stock(const char *filename);
void save_stock(const char *filename);
void save_inorder(node_t *n, FILE *fp);

/* Show formatting (single line) */
void inorder_show_line(node_t *n, char *outbuf, int *offset);

node_t *insert_node(node_t *n, int id, int stock, int price);
node_t *find_node(node_t *n, int id);
void sigint_handler(int sig);

/* command handling */
void process_command(int connfd, char *cmd);

/* buffer operations */
void sbuf_init(sbuf_t *sp, int n);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

/* worker thread */
void *worker(void *arg);

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;
    int i;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    /* load stock data */
    load_stock("stock.txt");

    /* handle SIGINT for persistence */
    Signal(SIGINT, sigint_handler);

    /* init listening socket */
    listenfd = Open_listenfd(argv[1]);

    /* init shared buffer and thread pool */
    sbuf_init(&sbuf, 16);            /* buffer size 16 */
    for (i = 0; i < 40; i++)          /* NTHREADS = 40 */
        Pthread_create(&tid, NULL, worker, NULL);

    /* accept loop: master thread */
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen,
                    client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n",
               client_hostname, client_port);
        sbuf_insert(&sbuf, connfd);
    }
    return 0;
}

/* ===== SIGINT Handler ===== */
void sigint_handler(int sig) {
    /* save current tree to file */
    save_stock("stock.txt");
    printf("\nStock data saved. Exiting.\n");
    exit(0);
}

/* ===== Shared Buffer Implementation ===== */
void sbuf_init(sbuf_t *sp, int n) {
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

void sbuf_insert(sbuf_t *sp, int item) {
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(sp->rear) % sp->n] = item;
    sp->rear++;
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp) {
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(sp->front) % sp->n];
    sp->front++;
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}

/* ===== Worker Thread ===== */
void *worker(void *arg) {
    int connfd;
    char buf[MAXLINE];
    ssize_t n;
    rio_t rio;

    while (1) {
        connfd = sbuf_remove(&sbuf);
        Rio_readinitb(&rio, connfd);
        while ((n = Rio_readlineb(&rio, buf, MAXLINE)) > 0) {
            process_command(connfd, buf);
        }
        Close(connfd);
    }
}

/* ===== Command Processing ===== */
void process_command(int connfd, char *cmd) {
    char response[MAXLINE];
    int id, cnt;
    node_t *node;
    int offset;

    /* trim newline or CRLF */
    cmd[strcspn(cmd, "\r\n")] = '\0';

    if (strcmp(cmd, "show") == 0) {
        offset = 0;
        response[0] = '\0';
        inorder_show_line(root, response, &offset);
        /* terminate with newline */
        offset += sprintf(response + offset, "\n");
        Rio_writen(connfd, response, offset);

    } else if (strncmp(cmd, "buy ", 4) == 0 && sscanf(cmd+4, "%d %d", &id, &cnt) == 2) {
        node = find_node(root, id);
        if (node == NULL) {
            sprintf(response, "ID %d not found\n", id);
        } else {
            P(&node->data.mutex);
            if (node->data.left_stock < cnt) {
                sprintf(response, "Not enough left stocks\n");
            } else {
                node->data.left_stock -= cnt;
                sprintf(response, "[buy] success\n");
            }
            V(&node->data.mutex);
        }
        Rio_writen(connfd, response, strlen(response));

    } else if (strncmp(cmd, "sell ", 5) == 0 && sscanf(cmd+5, "%d %d", &id, &cnt) == 2) {
        node = find_node(root, id);
        if (node == NULL) {
            sprintf(response, "ID %d not found\n", id);
        } else {
            P(&node->data.mutex);
            node->data.left_stock += cnt;
            V(&node->data.mutex);
            sprintf(response, "[sell] success\n");
        }
        Rio_writen(connfd, response, strlen(response));

    } else if (strcmp(cmd, "exit") == 0) {
        return;  /* client disconnects */

    } else {
        /* echo unknown */
        Rio_writen(connfd, cmd, strlen(cmd));
        Rio_writen(connfd, "\n", 1);
    }
}

/* ===== Single-line Show Helper ===== */
void inorder_show_line(node_t *n, char *outbuf, int *offset) {
    if (n == NULL) return;
    inorder_show_line(n->left, outbuf, offset);
    *offset += sprintf(outbuf + *offset, "%d %d %d; ",
                        n->data.ID,
                        n->data.left_stock,
                        n->data.price);
    inorder_show_line(n->right, outbuf, offset);
}

/* ===== BST Operations ===== */
node_t *insert_node(node_t *n, int id, int stock, int price) {
    if (n == NULL) {
        node_t *new = Malloc(sizeof(node_t));
        new->data.ID = id;
        new->data.left_stock = stock;
        new->data.price = price;
        new->data.readcnt = 0;
        Sem_init(&new->data.mutex, 0, 1);
        new->left = new->right = NULL;
        return new;
    }
    if (id < n->data.ID)
        n->left = insert_node(n->left, id, stock, price);
    else if (id > n->data.ID)
        n->right = insert_node(n->right, id, stock, price);
    return n;
}

node_t *find_node(node_t *n, int id) {
    if (n == NULL) return NULL;
    if (id == n->data.ID) return n;
    if (id < n->data.ID) return find_node(n->left, id);
    else return find_node(n->right, id);
}

void load_stock(const char *filename) {
    FILE *fp = Fopen(filename, "r");
    int id, stock, price;
    while (fscanf(fp, "%d %d %d", &id, &stock, &price) == 3) {
        root = insert_node(root, id, stock, price);
    }
    Fclose(fp);
}

void save_stock(const char *filename) {
    FILE *fp = Fopen(filename, "w");
    save_inorder(root, fp);
    Fclose(fp);
}

void save_inorder(node_t *n, FILE *fp) {
    if (n == NULL) return;
    save_inorder(n->left, fp);
    fprintf(fp, "%d %d %d\n",
            n->data.ID,
            n->data.left_stock,
            n->data.price);
    save_inorder(n->right, fp);
}