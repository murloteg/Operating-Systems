#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

enum util_consts {
    FIRST_THREAD_INDEX = 0,
    SECOND_THREAD_INDEX = 1,
    THIRD_THREAD_INDEX = 2,
    FOURTH_THREAD_INDEX = 3,
    NUMBER_OF_THREADS = 4,
    TIME_TO_SLEEP_SEC = 10
};

typedef enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1
} status_t;

void* first_thread_routine(void* arg) {
    fprintf(stdout, "In the first thread routine\n");
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGILL, SIG_IGN);
    signal(SIGFPE, SIG_IGN);
    signal(SIGBUS, SIG_IGN);
    signal(SIGTRAP, SIG_IGN);
    signal(SIGABRT, SIG_IGN);
    signal(SIGCONT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
//    signal(SIGQUIT, SIG_IGN);
    unsigned int sleep_status = sleep(TIME_TO_SLEEP_SEC);
    if (sleep_status != OK) {
        fprintf(stderr, "Early woke up from the sleep()\n");
    }
    fprintf(stdout, "The first thread successfully finished!\n");
    return NULL;
}

void signal_handler(int signum) {
    fprintf(stdout, "In the DEFAULT handler of signal with signum: %d\n", signum);
}

void* second_thread_routine(void* arg) {
    fprintf(stdout, "In the second thread routine\n");
    signal(SIGINT, signal_handler);
    unsigned int sleep_status = sleep(TIME_TO_SLEEP_SEC);
    if (sleep_status != OK) {
        fprintf(stderr, "Early woke up from the sleep()\n");
    }
    /* This section shows what's happening if different signal handlers presents for certain signal.
    int sending_status = raise(SIGINT);
    if (sending_status != OK) {
        perror("Error during raise()");
    }
     */
    fprintf(stdout, "The second thread successfully finished!\n");
    return NULL;
}

void* third_thread_routine(void* arg) {
    fprintf(stdout, "In the third thread routine\n");
    sigset_t signal_set;
    int status = sigemptyset(&signal_set);
    if (status != OK) {
        fprintf(stderr, "Error during setemptyset()\n");
        return NULL;
    }
    status = sigaddset(&signal_set, SIGQUIT);
    if (status != OK) {
        fprintf(stderr, "Error during sigaddset()\n");
        return NULL;
    }
    int changing_status = sigprocmask(SIG_BLOCK, &signal_set, NULL);
    if (changing_status != OK) {
        perror("Error during sigprocmask()");
        return NULL;
    }

    int sending_status = raise(SIGQUIT);
    if (sending_status != OK) {
        perror("Error during raise()");
    }
    unsigned int sleep_status = sleep(TIME_TO_SLEEP_SEC);
    if (sleep_status != OK) {
        fprintf(stderr, "Early woke up from the sleep()\n");
    }

    int received_signum;
    int wait_status = sigwait(&signal_set, &received_signum);
    if (wait_status != OK) {
        fprintf(stderr, "Error during sigwait(); error number : %d\n", received_signum);
        return NULL;
    }
    fprintf(stdout, "Received signal with signum: %d\n", received_signum);
    fprintf(stdout, "The third thread successfully finished!\n");
    return NULL;
}

void another_signal_handler(int signum) {
    fprintf(stdout, "In the ANOTHER handler of signal with signum: %d\n", signum);
}

void* test_thread_routine(void* arg) {
    fprintf(stdout, "In the test thread routine\n");
    signal(SIGINT, another_signal_handler);
    unsigned int sleep_status = sleep(TIME_TO_SLEEP_SEC);
    if (sleep_status != OK) {
        fprintf(stderr, "Early woke up from the sleep()\n");
    }
    /* This section shows what's happening if different signal handlers presents for certain signal.
    int sending_status = raise(SIGINT);
    if (sending_status != OK) {
        perror("Error during raise()");
    }
     */
    fprintf(stdout, "The second thread successfully finished!\n");
    return NULL;
}

status_t execute_program() {
    pthread_t* threads = calloc(NUMBER_OF_THREADS, sizeof(pthread_t));
    if (threads == NULL) {
        perror("Error during calloc()");
        return SOMETHING_WENT_WRONG;
    }
    int first_creation_status = pthread_create(&threads[FIRST_THREAD_INDEX], NULL, first_thread_routine, NULL);
    if (first_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", first_creation_status);
        free(threads);
        return SOMETHING_WENT_WRONG;
    }
    int second_creation_status = pthread_create(&threads[SECOND_THREAD_INDEX], NULL, second_thread_routine, NULL);
    if (second_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", second_creation_status);
        free(threads);
        return SOMETHING_WENT_WRONG;
    }
    int third_creation_status = pthread_create(&threads[THIRD_THREAD_INDEX], NULL, third_thread_routine, NULL);
    if (third_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", third_creation_status);
        free(threads);
        return SOMETHING_WENT_WRONG;
    }
    int test_creation_status = pthread_create(&threads[FOURTH_THREAD_INDEX], NULL, test_thread_routine, NULL);
    if (test_creation_status != OK) {
        fprintf(stderr, "Error during pthread_create(); error code: %d\n", test_creation_status);
        free(threads);
        return SOMETHING_WENT_WRONG;
    }

    /* This code section shows what's happening if thread doesn't have signal handler for certain signal.
    kill(getpid(), SIGINT);
     */

    void* first_returned_value;
    int first_join_status = pthread_join(threads[FIRST_THREAD_INDEX], &first_returned_value);
    if (first_join_status != OK) {
        fprintf(stderr, "Error during pthread_join(); error code: %d\n", first_join_status);
        free(threads);
        return SOMETHING_WENT_WRONG;
    }
    void* second_returned_value;
    int second_join_status = pthread_join(threads[SECOND_THREAD_INDEX], &second_returned_value);
    if (second_join_status != OK) {
        fprintf(stderr, "Error during pthread_join(); error code: %d\n", second_join_status);
        free(threads);
        return SOMETHING_WENT_WRONG;
    }
    void* third_returned_value;
    int third_join_status = pthread_join(threads[THIRD_THREAD_INDEX], &third_returned_value);
    if (third_join_status != OK) {
        fprintf(stderr, "Error during pthread_join(); error code: %d\n", third_join_status);
        free(threads);
        return SOMETHING_WENT_WRONG;
    }
    void* test_returned_value;
    int test_join_status = pthread_join(threads[FOURTH_THREAD_INDEX], &test_returned_value);
    if (test_join_status != OK) {
        fprintf(stderr, "Error during pthread_join(); error code: %d\n", test_join_status);
        free(threads);
        return SOMETHING_WENT_WRONG;
    }
    free(threads);
    fprintf(stdout, "Successful job\n");
    return OK;
}

int main(int argc, char** argv) {
    status_t execution_status = execute_program();
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
