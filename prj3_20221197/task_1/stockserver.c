/*
 * stockserver_select.c - Event-driven concurrent stock server (Task1 implementation)
 * Usage: ./stockserver_select <port>
 */

#include "csapp.h"

/* ===== BST Node Structure ===== */
typedef struct item {
    int id;
    int left_stock;
    int price;
    struct item *left, *right;
} item;

static item *root = NULL;

/* ===== Client Pool for select() ===== */
typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
} pool_t;

static pool_t pool;

/* ===== Function Prototypes ===== */
void sigint_handler(int sig);
void load_stock(const char *filename);
void save_stock(const char *filename);
item *insert_item(item *node, int id, int stock, int price);
void collect_stock_info(item *node, char *buf, int *offset);
void process_command(int connfd, char *cmdline);
int buy_stock(item *node, int id, int cnt);
int sell_stock(item *node, int id, int cnt);
void init_pool(int listenfd, pool_t *p);
void add_client(int connfd, pool_t *p);
void check_clients(pool_t *p);

/* ===== Main ===== */
int main(int argc, char **argv) {
    int listenfd;
    struct sockaddr_storage clientaddr;
    socklen_t clientlen;
    char port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    strcpy(port, argv[1]);

    load_stock("stock.txt");
    Signal(SIGINT, sigint_handler);
    listenfd = Open_listenfd(port);
    init_pool(listenfd, &pool);

    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);
        if (FD_ISSET(listenfd, &pool.ready_set)) {
            clientlen = sizeof(struct sockaddr_storage);
            int connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            add_client(connfd, &pool);
        }
        check_clients(&pool);
    }
}

/* ===== Signal Handler ===== */
void sigint_handler(int sig) {
    save_stock("stock.txt");
    exit(0);
}

/* ===== Load Stock from File ===== */
void load_stock(const char *filename) {
    FILE *fp = Fopen(filename, "r");
    int id, stock, price;
    while (fscanf(fp, "%d %d %d", &id, &stock, &price) == 3) {
        root = insert_item(root, id, stock, price);
    }
    Fclose(fp);
}

/* ===== Save Stock to File ===== */
void save_stock_recursive(item *n, FILE *fp) {
    if (!n) return;
    fprintf(fp, "%d %d %d\n", n->id, n->left_stock, n->price);
    save_stock_recursive(n->left, fp);
    save_stock_recursive(n->right, fp);
}

void save_stock(const char *filename) {
    FILE *fp = Fopen(filename, "w");
    save_stock_recursive(root, fp);
    Fclose(fp);
}

/* ===== BST Insert ===== */
item *insert_item(item *node, int id, int stock, int price) {
    if (!node) {
        item *n = Malloc(sizeof(item));
        n->id = id;
        n->left_stock = stock;
        n->price = price;
        n->left = n->right = NULL;
        return n;
    }
    if (id < node->id)
        node->left = insert_item(node->left, id, stock, price);
    else if (id > node->id)
        node->right = insert_item(node->right, id, stock, price);
    return node;
}

/* ===== Inorder to Buffer ===== */
void collect_stock_info(item *node, char *buf, int *offset) {
    if (!node) return;
    collect_stock_info(node->left, buf, offset);
    int n = snprintf(buf + *offset, MAXLINE - *offset, "%d %d %d\n", node->id, node->left_stock, node->price);
    *offset += n;
    collect_stock_info(node->right, buf, offset);
}

/* ===== Buy / Sell ===== */
int buy_stock(item *node, int id, int cnt) {
    if (!node) return 0;
    if (id < node->id) return buy_stock(node->left, id, cnt);
    if (id > node->id) return buy_stock(node->right, id, cnt);
    if (node->left_stock < cnt) return 0;
    node->left_stock -= cnt;
    return 1;
}

int sell_stock(item *node, int id, int cnt) {
    if (!node) return 0;
    if (id < node->id) return sell_stock(node->left, id, cnt);
    if (id > node->id) return sell_stock(node->right, id, cnt);
    node->left_stock += cnt;
    return 1;
}

/* ===== Command Processor ===== */
void process_command(int connfd, char *cmdline) {
    char cmd[MAXLINE];
    int id, cnt;

    if (sscanf(cmdline, "%s", cmd) < 1) return;

    if (!strcmp(cmd, "show")) {
        char buf[MAXLINE * 10] = {0};
        int offset = 0;
        collect_stock_info(root, buf, &offset);
        Rio_writen(connfd, buf, offset);
    } else if (!strcmp(cmd, "buy") && sscanf(cmdline, "%*s %d %d", &id, &cnt) == 2) {
        if (buy_stock(root, id, cnt))
            Rio_writen(connfd, "[buy] success\n", strlen("[buy] success\n"));
        else
            Rio_writen(connfd, "Not enough left stocks\n", strlen("Not enough left stocks\n"));
    } else if (!strcmp(cmd, "sell") && sscanf(cmdline, "%*s %d %d", &id, &cnt) == 2) {
        if (sell_stock(root, id, cnt))
            Rio_writen(connfd, "[sell] success\n", strlen("[sell] success\n"));
        else
            Rio_writen(connfd, "[sell] fail: Invalid ID\n", strlen("[sell] fail: Invalid ID\n"));
    } else if (!strcmp(cmd, "exit")) {
        Close(connfd);
    } else {
        Rio_writen(connfd, "Invalid command\n", strlen("Invalid command\n"));
    }
}

/* ===== Pool Init / Add / Check ===== */
void init_pool(int listenfd, pool_t *p) {
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) p->clientfd[i] = -1;
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool_t *p) {
    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);
            FD_SET(connfd, &p->read_set);
            if (connfd > p->maxfd) p->maxfd = connfd;
            if (i > p->maxi) p->maxi = i;
            return;
        }
    }
    app_error("Too many clients");
}

void check_clients(pool_t *p) {
    int i, connfd;
    char buf[MAXLINE];
    for (i = 0; i <= p->maxi && p->nready > 0; i++) {
        connfd = p->clientfd[i];
        if (connfd < 0) continue;
        if (FD_ISSET(connfd, &p->ready_set)) {
            p->nready--;
            if (Rio_readlineb(&p->clientrio[i], buf, MAXLINE) <= 0) {
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            } else {
                process_command(connfd, buf);
            }
        }
    }
}