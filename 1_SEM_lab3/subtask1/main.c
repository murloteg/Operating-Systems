#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

enum general_consts {
    NUMBER_OF_REQUIRED_ARGS = 2 /* program name and path to directory */
};

enum permission_modes {
    READ_WRITE_EXECUTE = 0777
};

enum directory_errors {
    OK = 0,
    DIRECTORY_DOESNT_EXIST = 1,
    CONTENT_COPY_ERROR = 2,
    FAILED_DIRECTORY_CLOSING = -1,
    DIRECTORY_NAME_NOT_FOUND = -1,
    DIRECTORY_WAS_NOT_CREATED = -1
};

enum statuses_of_copy {
    SUCCESSFULLY_COPIED = 0,
    CANNOT_OPEN_FILE = 1,
    MEMORY_ALLOCATION_ERROR = 2,
    FAILED_COPIED = 3
};

enum util_consts {
    BUFFER_SIZE = 512,
    SOMETHING_WENT_WRONG = -1
};

void swap(char* first_char, char* second_char) {
    char temporary_char = *first_char;
    *first_char = *second_char;
    *second_char = temporary_char;
}

void reverse_string(char* origin_string, char* reversed_string) {
    size_t string_length = strlen(origin_string);
    reversed_string = strcpy(reversed_string, origin_string);
    for (int i = 0; i < string_length / 2; ++i) {
        swap(&reversed_string[i], &reversed_string[string_length - i - 1]);
    }
}

void find_absolute_path_without_dir_name(char* absolute_path_without_name, char* dir_path, ssize_t absolute_path_without_name_length) {
    absolute_path_without_name = strncpy(absolute_path_without_name, dir_path, absolute_path_without_name_length);
    absolute_path_without_name[absolute_path_without_name_length] = '\0';
}

char* find_dir_name_in_path(char* dir_path) {
    char finding_symbol = '/';
    char* dir_name_position = strrchr(dir_path, finding_symbol);
    return (dir_name_position == NULL) ? NULL : dir_name_position + 1;
}

ssize_t get_absolute_path_without_dir_length(char* dir_path) {
    char* dir_name_position = find_dir_name_in_path(dir_path);
    if (dir_name_position == NULL) {
        return DIRECTORY_NAME_NOT_FOUND;
    }
    size_t absolute_path_length = strlen(dir_path);
    size_t dir_name_length = strlen(dir_name_position);
    return (ssize_t) (absolute_path_length - dir_name_length);
}

enum directory_errors create_dir_with_reversed_name(char* dir_path, char* dir_path_with_reversed_name) {
    ssize_t absolute_path_without_name_length = get_absolute_path_without_dir_length(dir_path);
    if (absolute_path_without_name_length == DIRECTORY_NAME_NOT_FOUND) {
        fprintf(stderr, "Cannot find directory name in input path!\n");
        return DIRECTORY_NAME_NOT_FOUND;
    }

    char absolute_path_without_name[absolute_path_without_name_length];
    find_absolute_path_without_dir_name(absolute_path_without_name, dir_path, absolute_path_without_name_length);

    /* Previous steps ensures that the directory name exists */
    char* dir_name = find_dir_name_in_path(dir_path);
    size_t dir_name_length = strlen(dir_name);
    char reversed_dir_name[dir_name_length];
    reverse_string(dir_name, reversed_dir_name);

    char* new_dir_path = strcat(absolute_path_without_name, reversed_dir_name);
    enum directory_errors creation_status = mkdir(new_dir_path, READ_WRITE_EXECUTE);
    if (creation_status == DIRECTORY_WAS_NOT_CREATED) {
        perror("Error during mkdir");
        return DIRECTORY_WAS_NOT_CREATED;
    }
    dir_path_with_reversed_name = strcpy(dir_path_with_reversed_name, new_dir_path);
    return OK;
}

enum directory_errors is_this_dir_existing(char* dir_path) {
    DIR* opened_dir = opendir(dir_path);
    if (opened_dir == NULL) {
        fprintf(stderr, "Directory does not exist!\n");
        return DIRECTORY_DOESNT_EXIST;
    }

    enum directory_errors closing_status = closedir(opened_dir);
    if (closing_status == FAILED_DIRECTORY_CLOSING) {
        perror("Error during closedir");
    }
    return closing_status;
}

char* prepare_file_path(char* dir_path, char* file_name) {
    size_t dir_path_length = strlen(dir_path);
    size_t file_name_length = strlen(file_name);

    size_t merged_path_length = dir_path_length + file_name_length + 2;
    char* merged_file_path = (char*) malloc(merged_path_length * sizeof(char));
    if (merged_file_path == NULL) {
        perror("Error during malloc");
        return NULL;
    }
    merged_file_path = memset(merged_file_path, '\0', merged_path_length);

    char delimiter_symbol = '/';
    merged_file_path = strncpy(merged_file_path, dir_path, dir_path_length);
    merged_file_path[dir_path_length] = delimiter_symbol;

    merged_file_path = strncat(merged_file_path, file_name, file_name_length);
    return merged_file_path;
}

void backward_copying_files(FILE* input_file, FILE* output_file) {
    int seeking_status = fseek(input_file, 0, SEEK_END);
    if (seeking_status == SOMETHING_WENT_WRONG) {
        perror("Error during fseek");
        return;
    }
    long file_position = ftell(input_file);
    if (file_position == SOMETHING_WENT_WRONG) {
        perror("Error during ftell");
        return;
    }
    
    char reading_buffer[BUFFER_SIZE];
    long current_offset_from_end = 0;
    while (file_position > 0) {
        long count_of_reading_symbols = BUFFER_SIZE;
        if (count_of_reading_symbols > file_position) {
            count_of_reading_symbols = file_position;
        }

        current_offset_from_end += count_of_reading_symbols;
        seeking_status = fseek(input_file, -current_offset_from_end, SEEK_END);
        if (seeking_status == SOMETHING_WENT_WRONG) {
            perror("Error during fseek");
            return;
        }

        size_t return_read_code = fread(reading_buffer, sizeof(char), count_of_reading_symbols, input_file);
        if (return_read_code < count_of_reading_symbols || (ferror(input_file) != OK)) {
            fprintf(stderr, "Error during fread\n");
            return;
        }

        for (long i = count_of_reading_symbols - 1; i >= 0; --i) {
            fprintf(output_file, "%c", reading_buffer[i]);
        }
        file_position -= count_of_reading_symbols;
    }
}

enum statuses_of_copy copy_file_content(char* dir_path, char* file_name, char* dir_path_with_reversed_name) {
    char* file_path = prepare_file_path(dir_path, file_name);
    if (file_path == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }

    size_t file_name_length = strlen(file_name);
    char reversed_file_name[file_name_length];
    reverse_string(file_name, reversed_file_name);
    char* reversed_file_path = prepare_file_path(dir_path_with_reversed_name, reversed_file_name);
    if (reversed_file_path == NULL) {
        free(file_path);
        return MEMORY_ALLOCATION_ERROR;
    }

    FILE* input_file = fopen(file_path, "rb");
    if (input_file == NULL) {
        perror("Error during fopen");
        free(file_path);
        free(reversed_file_path);
        return CANNOT_OPEN_FILE;
    }
    free(file_path);

    FILE* output_file = fopen(reversed_file_path, "wb");
    if (output_file == NULL) {
        perror("Error during fopen");
        free(reversed_file_path);
        int closing_status = fclose(input_file);
        if (closing_status != OK) {
            perror("Error during fclose");
            return FAILED_COPIED;
        }
        return CANNOT_OPEN_FILE;
    }
    free(reversed_file_path);

    backward_copying_files(input_file, output_file);
    int closing_status = fclose(input_file);
    if (closing_status != OK) {
        perror("Error during fclose");
        return FAILED_COPIED;
    }
    closing_status = fclose(output_file);
    if (closing_status != OK) {
        perror("Error during fclose");
        return FAILED_COPIED;
    }
    return SUCCESSFULLY_COPIED;
}

bool is_available_subdir_name(char* subdir_name) {
    size_t subdir_name_length = strlen(subdir_name);
    return !((subdir_name_length == strlen(".") && (strstr(subdir_name, ".") != NULL)) ||
            (subdir_name_length == strlen("..") && (strstr(subdir_name, "..") != NULL)));
}

char* prepare_subdir_path(char* dir_path, char* subdir_name) {
    if (!is_available_subdir_name(subdir_name)) {
        return NULL;
    }

    size_t dir_path_length = strlen(dir_path);
    size_t subdir_name_length = strlen(subdir_name);

    size_t merged_subdir_path_length = dir_path_length + subdir_name_length + 2;
    char* merged_subdir_path = (char*) malloc(merged_subdir_path_length * sizeof(char));
    if (merged_subdir_path == NULL) {
        perror("Error during malloc");
        return NULL;
    }
    merged_subdir_path = memset(merged_subdir_path, '\0', merged_subdir_path_length);

    char delimiter_symbol = '/';
    merged_subdir_path = strncpy(merged_subdir_path, dir_path, dir_path_length);
    merged_subdir_path[dir_path_length] = delimiter_symbol;

    merged_subdir_path = strncat(merged_subdir_path, subdir_name, subdir_name_length);
    return merged_subdir_path;
}

enum statuses_of_copy create_reversed_subdir(char* dir_path_with_reversed_name, char* subdir_name, char* subdir_path_with_reversed_name) {
    char* subdir_path = prepare_subdir_path(dir_path_with_reversed_name, subdir_name);
    enum directory_errors creation_status = create_dir_with_reversed_name(subdir_path, subdir_path_with_reversed_name);
    if (creation_status != OK) {
        return FAILED_COPIED;
    }
    return SUCCESSFULLY_COPIED;
}

enum directory_errors close_opened_directories(DIR* openedOriginDirectory, DIR* openedReversedDirectory) {
    enum directory_errors closingStatus = closedir(openedOriginDirectory);
    if (closingStatus == FAILED_DIRECTORY_CLOSING) {
        perror("Error during closedir");
        return FAILED_DIRECTORY_CLOSING;
    }
    closingStatus = closedir(openedReversedDirectory);
    if (closingStatus == FAILED_DIRECTORY_CLOSING) {
        perror("Error during closedir");
        return FAILED_DIRECTORY_CLOSING;
    }
    return OK;
}

enum directory_errors copy_all_content_of_dir(char* dir_path, char* dir_path_with_reversed_name);
enum statuses_of_copy copy_subdir_content(char* dir_path, char* dir_path_with_reversed_name, char* subdir_name) {
    char* subdir_path = prepare_subdir_path(dir_path, subdir_name);
    if (subdir_path != NULL) {
        size_t subdir_path_with_reversed_name_length = strlen(dir_path_with_reversed_name) + strlen(subdir_name);
        char subdir_path_with_reversed_name[subdir_path_with_reversed_name_length];
        enum statuses_of_copy copy_status = create_reversed_subdir(dir_path_with_reversed_name, subdir_name, subdir_path_with_reversed_name);
        if (copy_status != SUCCESSFULLY_COPIED) {
            free(subdir_path);
            return FAILED_COPIED;
        }
        enum directory_errors subdir_copied_status = copy_all_content_of_dir(subdir_path, subdir_path_with_reversed_name);
        if (subdir_copied_status != OK) {
            free(subdir_path);
            return FAILED_COPIED;
        }
    }
    free(subdir_path);
    return SUCCESSFULLY_COPIED;
}

enum directory_errors copy_all_content_of_dir(char* dir_path, char* dir_path_with_reversed_name) {
    DIR* opened_origin_dir = opendir(dir_path);
    DIR* opened_reversed_dir = opendir(dir_path_with_reversed_name);
    if (opened_origin_dir == NULL || opened_reversed_dir == NULL) {
        fprintf(stderr, "Directory does not exist!\n");
        return DIRECTORY_DOESNT_EXIST;
    }

    enum statuses_of_copy copy_status;
    struct dirent* dir_entry = readdir(opened_origin_dir);
    while (dir_entry != NULL) {
        if (dir_entry->d_type == DT_REG) {
            char* file_name = dir_entry->d_name;
            copy_status = copy_file_content(dir_path, file_name, dir_path_with_reversed_name);
            if (copy_status != SUCCESSFULLY_COPIED) {
                break;
            }
        } else if (dir_entry->d_type == DT_DIR) {
            char* subdir_name = dir_entry->d_name;
            copy_status = copy_subdir_content(dir_path, dir_path_with_reversed_name, subdir_name);
            if (copy_status != SUCCESSFULLY_COPIED) {
                break;
            }
        }
        dir_entry = readdir(opened_origin_dir);
    }

    enum directory_errors closing_status = close_opened_directories(opened_origin_dir, opened_reversed_dir);
    if (copy_status != SUCCESSFULLY_COPIED) {
        return CONTENT_COPY_ERROR;
    }
    return closing_status;
}

int main(int argc, char** argv) {
    if (argc != NUMBER_OF_REQUIRED_ARGS) {
        fprintf(stderr, "Invalid number of arguments!\n");
        return EXIT_FAILURE;
    }

    char* dir_path = argv[1];
    enum directory_errors existing_status = is_this_dir_existing(dir_path);
    if (existing_status != OK) {
        return EXIT_FAILURE;
    }

    size_t dir_path_length = strlen(dir_path);
    char dir_path_with_reversed_name[dir_path_length];
    enum directory_errors creation_status = create_dir_with_reversed_name(dir_path, dir_path_with_reversed_name);
    if (creation_status != OK) {
        return EXIT_FAILURE;
    }

    enum directory_errors copy_status = copy_all_content_of_dir(dir_path, dir_path_with_reversed_name);
    if (copy_status != OK) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
