#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

enum options {
    CREATE_DIR = 1,
    PRINT_DIR_CONTENT = 2,
    REMOVE_DIR = 3,
    CREATE_FILE = 4,
    PRINT_FILE_CONTENT = 5,
    REMOVE_FILE = 6,
    CREATE_SYM_LINK = 7,
    PRINT_SYM_LINK_CONTENT = 8,
    PRINT_FILE_CONTENT_BY_SYM_LINK = 9,
    REMOVE_SYM_LINK = 10,
    CREATE_HARD_LINK = 11,
    REMOVE_HARD_LINK = 12,
    PRINT_FILE_PERMISSIONS = 13,
    CHANGE_FILE_PERMISSIONS = 14,
    UNDEFINED_OPTION = 15
};

enum statuses {
    OK = 0,
    SOMETHING_WENT_WRONG = -1,
    MEMORY_ALLOCATION_ERROR = 1
};

enum permission_modes {
    READ_WRITE_EXECUTE = 0777
};

enum util_consts {
    REQUIRED_ARGUMENTS_NUMBER = 2,
    BUFFER_LENGTH = 2,
    MAX_CWD_LENGTH = 128
};

enum options parse_option(char* file_name) {
    char* finding_status = NULL;
    finding_status = strstr(file_name, "create_dir");
    if (finding_status != NULL) {
        return CREATE_DIR;
    }

    finding_status = strstr(file_name, "print_dir_content");
    if (finding_status != NULL) {
        return PRINT_DIR_CONTENT;
    }

    finding_status = strstr(file_name, "remove_dir");
    if (finding_status != NULL) {
        return REMOVE_DIR;
    }

    finding_status = strstr(file_name, "create_file");
    if (finding_status != NULL) {
        return CREATE_FILE;
    }

    finding_status = strstr(file_name, "print_file_content_by_sym_link");
    if (finding_status != NULL) {
        return PRINT_FILE_CONTENT_BY_SYM_LINK;
    }

    finding_status = strstr(file_name, "print_file_content");
    if (finding_status != NULL) {
        return PRINT_FILE_CONTENT;
    }

    finding_status = strstr(file_name, "remove_file");
    if (finding_status != NULL) {
        return REMOVE_FILE;
    }

    finding_status = strstr(file_name, "create_sym_link");
    if (finding_status != NULL) {
        return CREATE_SYM_LINK;
    }

    finding_status = strstr(file_name, "print_sym_link_content");
    if (finding_status != NULL) {
        return PRINT_SYM_LINK_CONTENT;
    }

    finding_status = strstr(file_name, "remove_sym_link");
    if (finding_status != NULL) {
        return REMOVE_SYM_LINK;
    }

    finding_status = strstr(file_name, "create_hard_link");
    if (finding_status != NULL) {
        return CREATE_HARD_LINK;
    }

    finding_status = strstr(file_name, "remove_hard_link");
    if (finding_status != NULL) {
        return REMOVE_HARD_LINK;
    }

    finding_status = strstr(file_name, "print_file_permissions");
    if (finding_status != NULL) {
        return PRINT_FILE_PERMISSIONS;
    }

    finding_status = strstr(file_name, "change_file_permissions");
    if (finding_status != NULL) {
        return CHANGE_FILE_PERMISSIONS;
    }
    return UNDEFINED_OPTION;
}

// struct stat file_stat --> permissions and links.

size_t calculate_total_path_length(char* curr_working_dir, char* file_name) {
    size_t total_path_length = 0;
    char delimiter_symbol = '_';
    char* last_position = strrchr(file_name, delimiter_symbol);
    if (last_position == NULL) {
        return total_path_length;
    }
    total_path_length = strlen(curr_working_dir) + strlen(last_position) - 1;
    return total_path_length;
}

char* prepare_full_file_path(char* curr_working_dir, char* file_name) {
    size_t cwd_length = strlen(curr_working_dir);
    size_t file_name_length = strlen(file_name);

    char* merged_file_path = (char*) calloc(cwd_length + file_name_length + 1, sizeof(char));
    if (merged_file_path == NULL) {
        perror("Error during calloc");
        return NULL;
    }

    merged_file_path = strcat(merged_file_path, curr_working_dir);
    char delimiter_symbol = '/';
    merged_file_path[cwd_length] = delimiter_symbol;
    merged_file_path = strcat(merged_file_path, file_name);
    return merged_file_path;
}

char* get_file_name_from_input(char* option) {
    char delimiter_symbol = '_';
    char* last_position = strrchr(option, delimiter_symbol);
    if (last_position == NULL) {
        fprintf(stderr, "Incorrect argument!\n");
        return NULL;
    }
    return last_position + 1;
}

enum statuses create_dir(char* full_file_path) {
    int creation_status = mkdir(full_file_path, READ_WRITE_EXECUTE);
    if (creation_status == SOMETHING_WENT_WRONG) {
        perror("Error during mkdir");
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

enum statuses print_dir_content(char* full_file_path) {
    DIR* opened_dir = opendir(full_file_path);
    if (opened_dir == NULL) {
        fprintf(stderr, "Cannot open the directory!\n");
        return SOMETHING_WENT_WRONG;
    }

    struct dirent* dir_entry = readdir(opened_dir);
    while (dir_entry != NULL) {
        fprintf(stdout, "%s\n", dir_entry->d_name);
        dir_entry = readdir(opened_dir);
    }

    int closing_status = closedir(opened_dir);
    if (closing_status == SOMETHING_WENT_WRONG) {
        perror("Error during closedir");
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

enum statuses create_file(char* full_file_path) {
    FILE* created_file = fopen(full_file_path, "w");
    if (created_file == NULL) {
        perror("Error during fopen");
        return SOMETHING_WENT_WRONG;
    }
    int closing_status = fclose(created_file);
    if (closing_status != OK) {
        perror("Error during fclose");
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

/* for files, directories and symbolic links */
enum statuses remove_file(char* full_file_path) {
    int removing_status = remove(full_file_path);
    if (removing_status == SOMETHING_WENT_WRONG) {
        perror("Error during remove");
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

enum statuses read_content_from_file(FILE* opened_file) {
    int seeking_status = fseek(opened_file, 0, SEEK_END);
    if (seeking_status == SOMETHING_WENT_WRONG) {
        perror("Error during fseek");
        return SOMETHING_WENT_WRONG;
    }

    long reading_count = ftell(opened_file);
    if (reading_count == SOMETHING_WENT_WRONG) {
        perror("Error during ftell");
        return SOMETHING_WENT_WRONG;
    }

    seeking_status = fseek(opened_file, 0, SEEK_SET);
    if (seeking_status == SOMETHING_WENT_WRONG) {
        perror("Error during fseek");
        return SOMETHING_WENT_WRONG;
    }

    char buffer[BUFFER_LENGTH];
    while (reading_count > 0) {
        long curr_reading_count = BUFFER_LENGTH;
        if (reading_count < BUFFER_LENGTH) {
            curr_reading_count = reading_count;
        }
        size_t reading_status = fread(buffer, sizeof(char), curr_reading_count, opened_file);
        if (reading_status < curr_reading_count || (ferror(opened_file) != OK)) {
            fprintf(stderr, "Error during fread!\n");
            return SOMETHING_WENT_WRONG;
        }

        size_t writing_status = fwrite(buffer, sizeof(char), curr_reading_count, stdout);
        if (writing_status < curr_reading_count) {
            fprintf(stderr, "Error during fwrite!\n");
            return SOMETHING_WENT_WRONG;
        }
        reading_count -= BUFFER_LENGTH;
    }
    return OK;
}

enum statuses print_file_content(char* full_file_path) {
    FILE* opened_file = fopen(full_file_path, "rb");
    if (opened_file == NULL) {
        perror("Error during fopen");
        return SOMETHING_WENT_WRONG;
    }

    enum statuses reading_status = read_content_from_file(opened_file);
    int closing_status = fclose(opened_file);
    if (closing_status != OK) {
        perror("Error during fclose");
        return SOMETHING_WENT_WRONG;
    }
    return reading_status;
}

enum statuses create_sym_link(char* full_file_path) {
    int linking_status = symlink(full_file_path, "symlink");
    if (linking_status == SOMETHING_WENT_WRONG) {
        perror("Error during symlink");
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

enum statuses print_sym_link_content(char* full_file_path) {
    char buffer[MAX_CWD_LENGTH];
    ssize_t reading_status = readlink(full_file_path, buffer, MAX_CWD_LENGTH);
    if (reading_status == SOMETHING_WENT_WRONG) {
        perror("Error during readlink");
        return SOMETHING_WENT_WRONG;
    }

    size_t content_size = strlen(buffer);
    size_t writing_status = fwrite(buffer, sizeof(char), content_size, stdout);
    if (writing_status < content_size) {
        fprintf(stderr, "Error during fwrite!\n");
        return SOMETHING_WENT_WRONG;
    }
    return OK;
}

enum statuses print_file_content_by_sym_link(char* full_file_path) {
    char buffer[MAX_CWD_LENGTH];
    ssize_t reading_status = readlink(full_file_path, buffer, MAX_CWD_LENGTH);
    if (reading_status == SOMETHING_WENT_WRONG) {
        perror("Error during readlink");
        return SOMETHING_WENT_WRONG;
    }

    FILE* opened_file = fopen(buffer, "rb");
    if (opened_file == NULL) {
        perror("Error during fopen");
        return SOMETHING_WENT_WRONG;
    }

    reading_status = read_content_from_file(opened_file);
    int closing_status = fclose(opened_file);
    if (closing_status != OK) {
        perror("Error during fclose");
        return SOMETHING_WENT_WRONG;
    }
    return reading_status;
}

void clean_up(char* curr_working_dir, char* full_file_path) {
    free(curr_working_dir);
    free(full_file_path);
}

enum statuses execute_option(enum options option_type, char* option) {
    char* curr_working_dir = (char*) calloc(MAX_CWD_LENGTH, sizeof(char));
    if (curr_working_dir == NULL) {
        perror("Error during calloc");
        return MEMORY_ALLOCATION_ERROR;
    }

    curr_working_dir = getcwd(curr_working_dir, MAX_CWD_LENGTH);
    if (curr_working_dir == NULL) {
        perror("Error during getcwd");
        return SOMETHING_WENT_WRONG;
    }

    char* full_file_path = get_file_name_from_input(option);
    if (full_file_path == NULL) {
        return SOMETHING_WENT_WRONG;
    }
    full_file_path = prepare_full_file_path(curr_working_dir, full_file_path);
    if (full_file_path == NULL) {
        return SOMETHING_WENT_WRONG;
    }

    enum statuses execution_status = OK;
    switch (option_type) {
        case CREATE_DIR: {
            execution_status = create_dir(full_file_path);
            break;
        }
        case PRINT_DIR_CONTENT: {
            execution_status = print_dir_content(full_file_path);
            break;
        }
        case REMOVE_DIR: {
            execution_status = remove_file(full_file_path);
            break;
        }
        case CREATE_FILE: {
            execution_status = create_file(full_file_path);
            break;
        }
        case PRINT_FILE_CONTENT: {
            execution_status = print_file_content(full_file_path);
            break;
        }
        case REMOVE_FILE: {
            execution_status = remove_file(full_file_path);
            break;
        }
        case CREATE_SYM_LINK: {
            execution_status = create_sym_link(full_file_path);
            break;
        }
        case PRINT_SYM_LINK_CONTENT: {
            execution_status = print_sym_link_content(full_file_path);
            break;
        }
        case PRINT_FILE_CONTENT_BY_SYM_LINK: {
            execution_status = print_file_content_by_sym_link(full_file_path);
            break;
        }
        case REMOVE_SYM_LINK: {
            execution_status = remove_file(full_file_path);
            break;
        }
        case CREATE_HARD_LINK: {
            //
            break;
        }
        case REMOVE_HARD_LINK: {
            //
            break;
        }
        case PRINT_FILE_PERMISSIONS: {
            //
            break;
        }
        case CHANGE_FILE_PERMISSIONS: {
            //
            break;
        }
        default: {
            clean_up(curr_working_dir, full_file_path);
            return SOMETHING_WENT_WRONG;
        }
    }
    clean_up(curr_working_dir, full_file_path);
    return execution_status;
}

int main(int argc, char** argv) {
    if (argc != REQUIRED_ARGUMENTS_NUMBER) {
        fprintf(stderr, "Incorrect number of arguments!\n");
        return EXIT_FAILURE;
    }

    char* option = argv[1];
    enum options option_type = parse_option(option);
    if (option_type == UNDEFINED_OPTION) {
        fprintf(stderr, "Unexpected option!\n");
        return EXIT_FAILURE;
    }

    enum statuses execution_status = execute_option(option_type, option);
    if (execution_status != OK) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
