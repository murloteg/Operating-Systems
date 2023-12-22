#ifndef HTTP_PROXY_SYNC_PIPE_H
#define HTTP_PROXY_SYNC_PIPE_H

int sync_pipe_init();
void sync_pipe_close();
void sync_pipe_notify(int num_really_created_threads);
int get_rfd_spipe();

#endif //HTTP_PROXY_SYNC_PIPE_H
