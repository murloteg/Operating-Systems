#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/eventfd.h>

#include "picohttpparser-master/picohttpparser.h"
#include "data_storage.h"
#include "thread_sync_pipe/thread_sync_pipe.h"

#define START_REQUEST_SIZE BUFSIZ
#define START_RESPONSE_SIZE BUFSIZ

enum statuses_consts {
    SOMETHING_WENT_WRONG = -1
};

enum util_consts {
    REQUIRED_NUMBER_OF_ARGS = 2,
    DECIMAL_BASE = 10,
    LISTEN_BACKLOG_NUMBER = 30,
    TIMEOUT_IN_MILLISEC = 1200000
};

typedef struct client {
    char *request;
    response_t* response_record;
    size_t request_size;
    size_t request_index;
    bool is_stop;
    int write_response_index;
    int fd;
} client_t;

typedef struct server {
    response_t *response_record;
    int write_request_index;
    bool is_stop;
    int fd;
} server_t;

int WRITE_STOP_FD = -1;
int READ_STOP_FD = -1;

int num_threads = 0;
pthread_mutex_t num_threads_mutex;
pthread_attr_t thread_attr;
pthread_cond_t stop_cond;
pthread_rwlockattr_t rw_lock_attr;

bool valid_num_threads_mutex = false;
bool valid_stop_cond = false;
bool valid_thread_attr = false;
bool is_stop = false;
response_list_t* response_list = NULL;
bool valid_response = false;
bool valid_rw_lock_attr = false;

void realloc_poll_fds(struct pollfd** poll_fds, size_t* POLL_TABLE_SIZE) {
    size_t prev_size = *POLL_TABLE_SIZE;
    *POLL_TABLE_SIZE *= 2;
    *poll_fds = realloc(*poll_fds, *POLL_TABLE_SIZE * (sizeof(struct pollfd)));
    for (size_t i = prev_size; i < *POLL_TABLE_SIZE; i++) {
        (*poll_fds)[i].fd = -1;
    }
}

void add_fd_to_poll_fds(struct pollfd** poll_fds, int* poll_last_index, size_t* POLL_TABLE_SIZE, int fd, short events) {
    if (fd < 0) {
        return;
    }
    for (int i = 0; i < *poll_last_index; i++) {
        if ((*poll_fds)[i].fd == -1) {
            (*poll_fds)[i].fd = fd;
            (*poll_fds)[i].events = events;
            return;
        }
    }
    if (*poll_last_index >= *POLL_TABLE_SIZE) {
        realloc_poll_fds(poll_fds, POLL_TABLE_SIZE);
    }
    (*poll_fds)[*poll_last_index].fd = fd;
    (*poll_fds)[*poll_last_index].events = events;
    *poll_last_index += 1;
}

int connect_to_server_host(char* hostname, int port) {
    if (hostname == NULL || port < 0) {
        return -1;
    }
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        return -1;
    }

    struct hostent* h = gethostbyname(hostname);
    if (h == NULL) {
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, h->h_addr, h->h_length);

    int connect_res = connect(server_sock, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));
    if (connect_res != 0) {
        perror("connect");
        return -1;
    }
    return server_sock;
}

int create_thread(bool is_client, void *arg);

void accept_new_client(int listen_fd) {
    int new_client_fd = accept(listen_fd, NULL, NULL);
    if (new_client_fd == -1) {
        perror("new client accept");
        return;
    }
    int fcntl_res = fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
    if (fcntl_res < 0) {
        perror("make new client nonblock");
        close(new_client_fd);
        return;
    }
    client_t *client = (client_t*) calloc(1, sizeof(client_t));
    if (client == NULL) {
        close(new_client_fd);
        pthread_exit((void *) SOMETHING_WENT_WRONG);
    }
    client->fd = new_client_fd;
    client->response_record = NULL;
    client->request = NULL;
    client->request_size = 0;
    client->request_index = 0;
    client->write_response_index = -1;
    client->is_stop = false;
    fprintf(stderr, "new client with fd %d accepted\n", new_client_fd);
    int res = create_thread(true, (void *) client);
    if (res != 0) {
        fprintf(stderr, "error starting thread for client with fd %d\n", new_client_fd);
        close(new_client_fd);
        free(client);
    }
}

void change_event_for_fd(struct pollfd* poll_fds, const int* poll_last_index, int fd, short new_events) {
    if (fd < 0) {
        return;
    }
    for (int i = 0; i < *poll_last_index; i++) {
        if (poll_fds[i].fd == fd) {
            poll_fds[i].events = new_events;
            return;
        }
    }
}

void disconnect_client(client_t* client) {
    if (client == NULL) {
        return;
    }
    client->is_stop = true;
    fprintf(stderr, "disconnecting client with fd %d...\n", client->fd);
}

void add_subscriber(response_t* record, struct pollfd **poll_fds, int *poll_last_index, size_t *POLL_TABLE_SIZE) {
    if (record == NULL || !record->valid) {
        return;
    }
    pthread_mutex_lock(&record->subs_mutex);
    record->num_subscribers += 1;
    pthread_mutex_unlock(&record->subs_mutex);
    add_fd_to_poll_fds(poll_fds, poll_last_index, POLL_TABLE_SIZE, record->event_fd, POLLIN);
}

void notify_subscribers(response_t *record) {
    pthread_mutex_lock(&record->subs_mutex);
    uint64_t u = record->num_subscribers;
    pthread_mutex_unlock(&record->subs_mutex);
    write(record->event_fd, &u, sizeof(uint64_t));
}

void disconnect_server(server_t* server) {
    if (server == NULL) {
        return;
    }
    server->is_stop = true;
    fprintf(stderr, "disconnect server with fd %d...\n", server->fd);
}

void init_empty_response_record(response_t* record) {
    if (record == NULL) {
        return;
    }
    record->request = NULL;
    record->response = NULL;
    record->response_index = 0;
    record->response_size = 0;
    record->full = false;
    record->url = NULL;
    record->URL_LEN = 0;
    record->num_subscribers = 0;
    record->event_fd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
    record->server_alive = -1;
    record->private = true;
    pthread_rwlock_init(&record->rw_lock, &rw_lock_attr);
    record->valid_rw_lock = true;
    pthread_mutex_init(&record->subs_mutex, NULL);
    record->valid_subs_mutex = true;
    record->valid = false;
}

void create_response_record(char* url, size_t url_len, client_t* client, char* host, size_t request_size,
                           struct pollfd** poll_fds, int* poll_last_index, size_t* POLL_TABLE_SIZE) {
    pthread_mutex_lock(&response_list->mutex);
    response_node_t *list_nodes = response_list->head;
    response_node_t *prev_node = NULL;
    while (list_nodes != NULL) {
        if (!list_nodes->record->valid) {
            response_node_t *next_node = list_nodes->next;
            free_response_record(list_nodes->record);
            free(list_nodes->record);
            free(list_nodes);
            list_nodes = next_node;
            continue;
        }
        pthread_mutex_unlock(&response_list->mutex);

        pthread_mutex_lock(&response_list->mutex);
        prev_node = list_nodes;
        list_nodes = list_nodes->next;
    }
    response_node_t *new_node = (response_node_t *) calloc(1, sizeof(response_node_t));
    if (new_node == NULL) {
        pthread_mutex_unlock(&response_list->mutex);
        fprintf(stderr, "failed to add client with fd %d to data_storage (can't create new node)\n", client->fd);
        disconnect_client(client);
        return;
    }
    new_node->record = (response_t *) calloc(1, sizeof(response_t));
    if (new_node->record == NULL) {
        pthread_mutex_unlock(&response_list->mutex);
        fprintf(stderr, "failed to add client with fd %d to data_storage (can't create new record)\n", client->fd);
        free(new_node);
        disconnect_client(client);
        return;
    }
    new_node->next = NULL;
    init_empty_response_record(new_node->record);
    new_node->record->url = url;
    new_node->record->URL_LEN = url_len;
    new_node->record->valid = true;
    if (prev_node == NULL) {
        response_list->head = new_node;
    } else {
        prev_node->next = new_node;
    }
    pthread_mutex_unlock(&response_list->mutex);

    new_node->record->request = (char *) calloc(request_size, sizeof(char));
    if (new_node->record->request == NULL) {
        free_response_record(new_node->record);
        disconnect_client(client);
        return;
    }
    memcpy(new_node->record->request, client->request, request_size);
    new_node->record->request_size = request_size;
    int server_fd = connect_to_server_host(host, 80);
    if (server_fd == -1) {
        fprintf(stderr, "failed to connect to remote host: %s\n", host);
        free_response_record(new_node->record);
        disconnect_client(client);
        free(host);
        return;
    }
    free(host);
    int fcntl_res = fcntl(server_fd, F_SETFL, O_NONBLOCK);
    if (fcntl_res < 0) {
        perror("make new server fd nonblock");
        close(server_fd);
        free_response_record(new_node->record);
        disconnect_client(client);
        return;
    }
    server_t *server = (server_t *) calloc(1, sizeof(server_t));
    if (server == NULL) {
        fprintf(stderr, "failed to alloc memory for new server\n");
        close(server_fd);
        free_response_record(new_node->record);
        disconnect_client(client);
        return;
    }
    server->fd = server_fd;
    server->response_record = new_node->record;
    server->write_request_index = 0;
    server->is_stop = false;
    int res = create_thread(false, (void *) server);
    if (res != 0) {
        fprintf(stderr, "error starting server thread with server fd %d by client with fd %d\n", server_fd, client->fd);
        close(server_fd);
        free(server);
        free_response_record(new_node->record);
        disconnect_client(client);
        return;
    }

    client->response_record = new_node->record;
    add_subscriber(new_node->record, poll_fds, poll_last_index, POLL_TABLE_SIZE);
    client->write_response_index = 0;
}

void shift_request(char** request, size_t* request_index, int parsing_status) {
    if (*request == NULL || *request_index == 0 || parsing_status <= 0) {
        return;
    }
    for (int i = parsing_status; i < *request_index; i++) {
        (*request)[i] = (*request)[i - parsing_status];
    }
    memset(&(*request)[*request_index - parsing_status], 0, parsing_status);
    *request_index -= parsing_status;
}

void read_data_from_client(client_t* client, struct pollfd** poll_fds, int* poll_last_index, size_t* POLL_TABLE_SIZE) {
    if (client->fd == -1 || client->is_stop) {
        return;
    }
    char buf[BUFSIZ];
    ssize_t was_read = read(client->fd, buf, BUFSIZ);
    if (was_read < 0) {
        fprintf(stderr, "read from client with fd %d ", client->fd);
        perror("read");
        disconnect_client(client);
        return;
    } else if (was_read == 0) {
        fprintf(stderr, "client with fd %d closed connection\n", client->fd);
        disconnect_client(client);
        return;
    }
    if (client->request_size <= 0) {
        client->request_size = START_REQUEST_SIZE;
        client->request = (char *) calloc(client->request_size, sizeof(char));
        if (client->request == NULL) {
            fprintf(stderr, "calloc returned NULL\n");
            disconnect_client(client);
            return;
        }
    }
    if (client->request_index + was_read >= client->request_size) {
        client->request_size *= 2;
        client->request = realloc(client->request, client->request_size * sizeof(char));
    }
    memcpy(&client->request[client->request_index], buf, was_read);
    client->request_index += was_read;
    char *method;
    char *path;
    size_t method_len, path_len;
    int minor_version;
    size_t num_headers = 100;
    struct phr_header headers[num_headers];
    int parsing_status = phr_parse_request(client->request, client->request_index,
                                 (const char **) &method, &method_len, (const char **) &path, &path_len,
                                 &minor_version, headers, &num_headers, 0);
    if (parsing_status > 0) {
        if (strncmp(method, "GET", method_len) != 0) {
            disconnect_client(client);
            return;
        }
        size_t url_len = path_len;
        char *url = calloc(url_len, sizeof(char));
        if (url == NULL) {
            disconnect_client(client);
            return;
        }
        memcpy(url, path, path_len);


        char *host = NULL;
        for (size_t i = 0; i < num_headers; i++) {
            if (strncmp(headers[i].name, "Host", 4) == 0) {
                host = calloc(headers[i].value_len + 1, sizeof(char));
                if (host == NULL) {
                    free(url);
                    disconnect_client(client);
                    return;
                }
                memcpy(host, headers[i].value, headers[i].value_len);
                break;
            }
        }
        if (host == NULL) {
            free(url);
            disconnect_client(client);
            return;
        }
        create_response_record(url, url_len, client, host, parsing_status, poll_fds, poll_last_index, POLL_TABLE_SIZE);
        shift_request(&client->request, &client->request_index, parsing_status);
    } else if (parsing_status == -1) {
        disconnect_client(client);
    }
}


void write_data_to_server(server_t* server, struct pollfd* poll_fds, int* poll_last_index) {
    if (server == NULL || server->fd < 0 || server->is_stop) {
        return;
    }
    ssize_t written = write(server->fd,
                            &server->response_record->request[server->write_request_index],
                            server->response_record->request_size - server->write_request_index);
    if (written < 0) {
        fprintf(stderr, "write to server with fd %d ", server->fd);
        perror("write");
        disconnect_server(server);
        notify_subscribers(server->response_record);
        return;
    }
    server->write_request_index += (int) written;
    if (server->write_request_index == server->response_record->request_size) {
        change_event_for_fd(poll_fds, poll_last_index, server->fd, POLLIN);
    }
}

void read_data_from_server(server_t *server) {
    if (server == NULL || server->fd < 0 || server->is_stop) {
        return;
    }
    char buf[BUFSIZ];
    ssize_t was_read = read(server->fd, buf, BUFSIZ);
    if (was_read < 0) {
        fprintf(stderr, "read from server with fd %d ", server->fd);
        perror("read");
        disconnect_server(server);
        notify_subscribers(server->response_record);
        return;
    } else if (was_read == 0) {
        server->response_record->full = true;
        pthread_rwlock_wrlock(&server->response_record->rw_lock);

        server->response_record->response = realloc(
                server->response_record->response,
                server->response_record->response_index * sizeof(char));
        server->response_record->response_size = server->response_record->response_index;

        pthread_rwlock_unlock(&server->response_record->rw_lock);
        notify_subscribers(server->response_record);
        disconnect_server(server);
        return;
    }
    pthread_rwlock_wrlock(&server->response_record->rw_lock);
    if (server->response_record->response_size == 0) {
        server->response_record->response_size = START_RESPONSE_SIZE;
        server->response_record->response = (char *) calloc(START_RESPONSE_SIZE, sizeof(char));
        if (server->response_record->response == NULL) {
            pthread_rwlock_unlock(&server->response_record->rw_lock);
            disconnect_server(server);
            notify_subscribers(server->response_record);
            return;
        }
    }
    if (was_read + server->response_record->response_index >=
        server->response_record->response_size) {
        server->response_record->response_size *= 2;
        server->response_record->response = realloc(
                server->response_record->response,
                server->response_record->response_size * sizeof(char));
    }
    memcpy(&server->response_record->response[server->response_record->response_index],
           buf, was_read);
    size_t prev_len = server->response_record->response_index;
    server->response_record->response_index += was_read;
    pthread_rwlock_unlock(&server->response_record->rw_lock);
    int minor_version, status;
    char *msg;
    size_t msg_len;
    size_t num_headers = 100;
    struct phr_header headers[num_headers];
    pthread_rwlock_rdlock(&server->response_record->rw_lock);
    int parsing_status = phr_parse_response(server->response_record->response, server->response_record->response_index,
                                  &minor_version, &status, (const char **) &msg, &msg_len, headers,
                                  &num_headers, prev_len);
    pthread_rwlock_unlock(&server->response_record->rw_lock);
    notify_subscribers(server->response_record);
    if (parsing_status > 0) {
        if (status >= 200 && status < 300) {
            server->response_record->private = false;
        }
    }
}

void write_data_to_client(client_t *client, struct pollfd *poll_fds, int *poll_last_index) {
    if (client == NULL || client->fd < 0 || client->is_stop) {
        fprintf(stderr, "invalid client\n");
        return;
    }
    if (client->response_record == NULL) {
        fprintf(stderr, "client with fd %d data_storage record is NULL\n", client->fd);
        disconnect_client(client);
        return;
    }
    if (!client->response_record->server_alive && !client->response_record->full) {
        free_response_record(client->response_record);
        disconnect_client(client);
        return;
    }
    pthread_rwlock_rdlock(&client->response_record->rw_lock);
    ssize_t written = write(client->fd, &client->response_record->response[client->write_response_index],
                            client->response_record->response_index - client->write_response_index);
    pthread_rwlock_unlock(&client->response_record->rw_lock);
    if (written < 0) {
        fprintf(stderr, "write to client with fd %d ", client->fd);
        perror("write");
        disconnect_client(client);
        return;
    }
    client->write_response_index += (int) written;
    if (client->write_response_index == client->response_record->response_index) {
        change_event_for_fd(poll_fds, poll_last_index, client->fd, POLLIN);
    }
}

static void sig_catch(int sig) {
    if (sig == SIGINT) {
        if (WRITE_STOP_FD != -1) {
            char a = 'a';
            write(WRITE_STOP_FD, &a, 1);
            close(WRITE_STOP_FD);
            WRITE_STOP_FD = -1;
        }
    }
}

struct pollfd* init_poll_fds(size_t POLL_TABLE_SIZE, int *poll_last_index) {
    struct pollfd *poll_fds = (struct pollfd *) calloc(POLL_TABLE_SIZE, sizeof(struct pollfd));
    if (poll_fds == NULL) {
        fprintf(stderr, "failed to alloc memory for poll_fds\n");
        return NULL;
    }
    for (int i = 0; i < POLL_TABLE_SIZE; i++) {
        poll_fds[i].fd = -1;
    }
    *poll_last_index = 0;
    return poll_fds;
}

void remove_from_poll_fds(struct pollfd *poll_fds, int *poll_last_index, int fd) {
    if (fd < 0) {
        return;
    }
    int i;
    for (i = 0; i < *poll_last_index; i++) {
        if (poll_fds[i].fd == fd) {
            close(poll_fds[i].fd);
            poll_fds[i].fd = -1;
            poll_fds[i].events = 0;
            poll_fds[i].revents = 0;
            break;
        }
    }
    if (i == *poll_last_index - 1) {
        *poll_last_index -= 1;
    }
    for (i = (int) *poll_last_index - 1; i > 0; i--) {
        if (poll_fds[i].fd == -1) {
            *poll_last_index -= 1;
        } else {
            break;
        }
    }
}

void* client_thread_routine(void *arg) {
    client_t *client = (client_t *) arg;

    size_t POLL_TABLE_SIZE = 3;
    int poll_last_index = -1;

    struct pollfd *poll_fds = init_poll_fds(POLL_TABLE_SIZE, &poll_last_index);
    if (poll_fds == NULL) {
        close(client->fd);
        pthread_exit((void *) SOMETHING_WENT_WRONG);
    }
    add_fd_to_poll_fds(&poll_fds, &poll_last_index, &POLL_TABLE_SIZE, get_rfd_spipe(), POLLIN);
    add_fd_to_poll_fds(&poll_fds, &poll_last_index, &POLL_TABLE_SIZE, client->fd, POLLIN);

    pthread_mutex_lock(&num_threads_mutex);
    num_threads += 1;
    pthread_mutex_unlock(&num_threads_mutex);

    while (!is_stop && !client->is_stop) {
        int poll_res = poll(poll_fds, poll_last_index, TIMEOUT_IN_MILLISEC);
        if (is_stop || client->is_stop) {
            break;
        }
        if (poll_res < 0) {
            perror("poll");
            break;
        } else if (poll_res == 0) {
            fprintf(stdout, "proxy timeout\n");
            break;
        }
        int num_handled_fd = 0;
        size_t i = 0;
        size_t prev_last_index = poll_last_index;
        while (num_handled_fd < poll_res && i < prev_last_index && !is_stop && !client->is_stop) {
            if (poll_fds[i].fd == get_rfd_spipe() && (poll_fds[i].revents & POLLIN)) {
                char s;
                read(poll_fds[i].fd, &s, 1);
                is_stop = true;
                break;
            }
            bool handled = false;
            if (poll_fds[i].fd == client->fd && (poll_fds[i].revents & POLLIN)) {
                read_data_from_client(client, &poll_fds, &poll_last_index, &POLL_TABLE_SIZE);
                handled = true;
            }
            if (poll_fds[i].fd == client->fd && (poll_fds[i].revents & POLLOUT) && !client->is_stop) {
                write_data_to_client(client, poll_fds, &poll_last_index);
                handled = true;
            }
            if (poll_fds[i].fd != client->fd && (poll_fds[i].revents & POLLIN)) {
                uint64_t u;
                ssize_t read_res = read(poll_fds[i].fd, &u, sizeof(u));
                if (u != 0 && read_res >= 0) {
                    pthread_rwlock_rdlock(&client->response_record->rw_lock);
                    if (client->response_record->response_index > client->write_response_index) {
                        change_event_for_fd(poll_fds, &poll_last_index, client->fd, POLLIN | POLLOUT);
                    }
                    pthread_rwlock_unlock(&client->response_record->rw_lock);
                }
                handled = true;
            }
            i += 1;
            if (handled) {
                num_handled_fd += 1;
            }
        }
    }

    remove_from_poll_fds(poll_fds, &poll_last_index, client->fd);
    if (client->request != NULL) {
        free(client->request);
    }
    free_response_record(client->response_record);
    free(client);
    free(poll_fds);
    pthread_mutex_lock(&num_threads_mutex);
    num_threads -= 1;
    fprintf(stderr, "num_threads = %d\n", num_threads);
    pthread_cond_signal(&stop_cond);
    pthread_mutex_unlock(&num_threads_mutex);
    pthread_exit((void *) EXIT_SUCCESS);
}

void* server_thread_routine(void *arg) {
    server_t *server = (server_t *) arg;
    size_t POLL_TABLE_SIZE = 2;
    int poll_last_index = -1;

    struct pollfd *poll_fds = init_poll_fds(POLL_TABLE_SIZE, &poll_last_index);
    if (poll_fds == NULL) {
        close(server->fd);
        free(server);
        pthread_exit((void *) SOMETHING_WENT_WRONG);
    }
    add_fd_to_poll_fds(&poll_fds, &poll_last_index, &POLL_TABLE_SIZE, get_rfd_spipe(), POLLIN);
    add_fd_to_poll_fds(&poll_fds, &poll_last_index, &POLL_TABLE_SIZE, server->fd, POLLIN | POLLOUT);

    pthread_mutex_lock(&num_threads_mutex);
    num_threads += 1;
    pthread_mutex_unlock(&num_threads_mutex);

    while (!is_stop && !server->is_stop) {
        int poll_res = poll(poll_fds, poll_last_index, TIMEOUT_IN_MILLISEC);
        if (is_stop || server->is_stop) {
            break;
        }
        if (poll_res < 0) {
            perror("poll");
            break;
        } else if (poll_res == 0) {
            fprintf(stdout, "proxy timeout\n");
            break;
        }
        int num_handled_fd = 0;
        size_t i = 0;
        size_t prev_last_index = poll_last_index;
        while (num_handled_fd < poll_res && i < prev_last_index && !is_stop && !server->is_stop) {
            if (poll_fds[i].fd == get_rfd_spipe() && (poll_fds[i].revents & POLLIN)) {
                char s;
                read(poll_fds[i].fd, &s, 1);
                is_stop = true;
                break;
            }
            bool handled = false;
            if (poll_fds[i].fd == server->fd && (poll_fds[i].revents & POLLIN)) {
                read_data_from_server(server);
                handled = true;
            }
            if (poll_fds[i].fd == server->fd && (poll_fds[i].revents & POLLOUT) && !server->is_stop) {
                write_data_to_server(server, poll_fds, &poll_last_index);
                handled = true;
            }
            i += 1;
            if (handled) {
                num_handled_fd += 1;
            }
        }
    }
    if (server->response_record != NULL) {
        server->response_record->server_alive = false;
        fprintf(stderr, "server with fd %d was working with url: %s\n", server->fd, server->response_record->url);
    }
    remove_from_poll_fds(poll_fds, &poll_last_index, server->fd);
    free(server);
    free(poll_fds);
    pthread_mutex_lock(&num_threads_mutex);
    num_threads -= 1;
    fprintf(stderr, "num_threads = %d\n", num_threads);
    pthread_cond_signal(&stop_cond);
    pthread_mutex_unlock(&num_threads_mutex);
    pthread_exit((void *) EXIT_SUCCESS);
}

int create_thread(bool is_client, void *arg) {
    sigset_t old_set;
    sigset_t thread_set;
    sigemptyset(&thread_set);
    sigaddset(&thread_set, SIGINT);
    int create_res = 0;
    pthread_t tid;
    pthread_sigmask(SIG_BLOCK, &thread_set, &old_set);
    if (is_client) {
        create_res = pthread_create(&tid, &thread_attr, client_thread_routine, arg);
    } else {
        create_res = pthread_create(&tid, &thread_attr, server_thread_routine, arg);
    }
    pthread_sigmask(SIG_SETMASK, &old_set, NULL);
    return create_res;
}

void clean_up();

int init_listener(int port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        clean_up();
        abort();
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    int bind_res = bind(listen_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
    if (bind_res != 0) {
        perror("bind");
        close(listen_fd);
        clean_up();
        abort();
    }

    int listen_res = listen(listen_fd, LISTEN_BACKLOG_NUMBER);
    if (listen_res == -1) {
        perror("listen");
        close(listen_fd);
        clean_up();
        abort();
    }
    return listen_fd;
}

void init_response_list() {
    response_list = init_list();
    if (response_list == NULL) {
        clean_up();
        return;
    }
    valid_response = true;
}

void init_stop_cond() {
    if (!valid_stop_cond) {
        pthread_cond_init(&stop_cond, NULL);
        valid_stop_cond = true;
    }
}


void init_thread_attr() {
    if (!valid_thread_attr) {
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        valid_thread_attr = true;
    }
}

void init_rw_lock_attr() {
    if (!valid_rw_lock_attr) {
        pthread_rwlockattr_init(&rw_lock_attr);
        pthread_rwlockattr_setkind_np(&rw_lock_attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
        valid_rw_lock_attr = true;
    }
}

void destroy_poll_fds(struct pollfd *poll_fds, int *poll_last_index) {
    for (int i = 0; i < *poll_last_index; i++) {
        if (poll_fds[i].fd > 0) {
            int close_res = close(poll_fds[i].fd);
            if (close_res < 0) {
                fprintf(stderr, "error while closing fd %d ", poll_fds[i].fd);
                perror("close");
            }
            poll_fds[i].fd = -1;
        }
    }
    free(poll_fds);
    *poll_last_index = -1;
}

void destroy_response_list() {
    destroy_list(response_list);
    valid_response = false;
}

void destroy_rw_lock_attr() {
    if (valid_rw_lock_attr) {
        pthread_rwlockattr_destroy(&rw_lock_attr);
        valid_rw_lock_attr = false;
    }
}

void destroy_stop_cond() {
    if (valid_stop_cond) {
        pthread_cond_destroy(&stop_cond);
        valid_stop_cond = false;
    }
}

void clean_up() {
    is_stop = true;
    sync_pipe_notify(num_threads * 2);
    pthread_mutex_lock(&num_threads_mutex);
    while (num_threads != 0) {
        pthread_cond_wait(&stop_cond, &num_threads_mutex);
    }
    pthread_mutex_unlock(&num_threads_mutex);
    if (WRITE_STOP_FD != -1) {
        close(WRITE_STOP_FD);
        WRITE_STOP_FD = -1;
    }
    if (valid_response) {
        destroy_response_list();
    }
    if (valid_rw_lock_attr) {
        destroy_rw_lock_attr();
    }
    if (valid_num_threads_mutex) {
        pthread_mutex_destroy(&num_threads_mutex);
        valid_num_threads_mutex = false;
    }
    if (valid_thread_attr) {
        pthread_attr_destroy(&thread_attr);
        valid_thread_attr = false;
    }
    destroy_stop_cond();
    sync_pipe_close();
}

int main(int argc, char** argv) {
    if (argc < REQUIRED_NUMBER_OF_ARGS) {
        fprintf(stderr, "Error wrong number of args\n");
        return SOMETHING_WENT_WRONG;
    }
    char* invalid_sym;
    int port = (int) strtol(argv[1], &invalid_sym, DECIMAL_BASE);
    if (port == 0 || *invalid_sym != '\0') {
        fprintf(stderr, "Cannot parse port\n");
        return SOMETHING_WENT_WRONG;
    }

    init_stop_cond();

    int sync_pipe_res = sync_pipe_init();
    if (sync_pipe_res != 0) {
        fprintf(stderr, "failed to init sync pipe\n");
        clean_up();
        return SOMETHING_WENT_WRONG;
    }
    size_t POLL_TABLE_SIZE = 2;
    int poll_last_index = -1;

    struct pollfd *poll_fds = init_poll_fds(POLL_TABLE_SIZE, &poll_last_index);
    if (poll_fds == NULL) {
        fprintf(stderr, "Error during allocation\n");
        clean_up();
        return SOMETHING_WENT_WRONG;
    }

    int pipe_fds[2];
    int pipe_res = pipe(pipe_fds);
    if (pipe_res != 0) {
        perror("Error during pipe()");
        return SOMETHING_WENT_WRONG;
    }
    READ_STOP_FD = pipe_fds[0];
    WRITE_STOP_FD = pipe_fds[1];
    add_fd_to_poll_fds(&poll_fds, &poll_last_index, &POLL_TABLE_SIZE, READ_STOP_FD, POLLIN);

    struct sigaction sig_act = {0};
    sig_act.sa_handler = sig_catch;
    sigemptyset(&sig_act.sa_mask);
    int sigact_res = sigaction(SIGINT, &sig_act, NULL);
    if (sigact_res != 0) {
        perror("sigaction");
        clean_up();
        return SOMETHING_WENT_WRONG;
    }

    init_rw_lock_attr();
    init_thread_attr();
    init_response_list();

    int listen_fd = init_listener(port);
    add_fd_to_poll_fds(&poll_fds, &poll_last_index, &POLL_TABLE_SIZE, listen_fd, POLLIN);
    
    while (!is_stop) {
        fprintf(stderr, "main: poll()\n");
        int poll_res = poll(poll_fds, poll_last_index, TIMEOUT_IN_MILLISEC);
        if (poll_res < 0) {
            perror("poll");
            break;
        } else if (poll_res == 0) {
            fprintf(stdout, "proxy timeout\n");
            break;
        }
        int num_handled_fd = 0;
        size_t i = 0;
        size_t prev_last_index = poll_last_index;
        fprintf(stderr, "main: poll_res = %d\n", poll_res);

        while (num_handled_fd < poll_res && i < prev_last_index && !is_stop) {
            if (poll_fds[i].fd == READ_STOP_FD && (poll_fds[i].revents & POLLIN)) {
                remove_from_poll_fds(poll_fds, &poll_last_index, READ_STOP_FD);
                READ_STOP_FD = -1;
                destroy_poll_fds(poll_fds, &poll_last_index);
                clean_up();
                pthread_exit(EXIT_SUCCESS);
            }
            if (poll_fds[i].fd == listen_fd && (poll_fds[i].revents & POLLIN)) {
                accept_new_client(listen_fd);
                num_handled_fd += 1;
            }
            i += 1;
        }
    }
    remove_from_poll_fds(poll_fds, &poll_last_index, READ_STOP_FD);
    READ_STOP_FD = -1;
    destroy_poll_fds(poll_fds, &poll_last_index);
    clean_up();
    pthread_exit(EXIT_SUCCESS);
}
