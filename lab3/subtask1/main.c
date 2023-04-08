#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

enum Consts {
    NUMBER_OF_REQUIRED_ARGS = 2 /* program name and path to directory */
};

enum PermissionModes {
    READ_WRITE_EXECUTE = 0777
};

enum DirectoryErrors {
    OK = 0,
    DIRECTORY_DOESNT_EXIST = 1,
    CONTENT_COPY_ERROR = 2,
    FAILED_DIRECTORY_CLOSING = -1,
    DIRECTORY_NAME_NOT_FOUND = -1,
    DIRECTORY_WAS_NOT_CREATED = -1
};

enum FileErrors {
    SUCCESSFULLY_COPIED = 0,
    CANNOT_OPEN_FILE = 1,
    MEMORY_ALLOCATION_ERROR = 2
};

enum UtilConsts {
    BUFFER_SIZE = 2048
};

void swap(char* firstChar, char* secondChar) {
    char temporaryCharacter = *firstChar;
    *firstChar = *secondChar;
    *secondChar = temporaryCharacter;
}

void reverse_string(char* originString, char* reversedString) {
    size_t lengthOfString = strlen(originString);
    reversedString = strcpy(reversedString, originString);
    for (int i = 0; i < lengthOfString / 2; ++i) {
        swap(&reversedString[i], &reversedString[lengthOfString - i - 1]);
    }
}

void find_absolute_path_without_directory_name(char* absolutePathWithoutName, char* directoryPath, ssize_t lengthOfAbsolutePathWithoutName) {
    absolutePathWithoutName = strncpy(absolutePathWithoutName, directoryPath, lengthOfAbsolutePathWithoutName);
    absolutePathWithoutName[lengthOfAbsolutePathWithoutName] = '\0';
}

char* find_directory_name_in_path(char* directoryPath) {
    char findingSymbol = '/';
    char* directoryNamePosition = strrchr(directoryPath, findingSymbol);
    return (directoryNamePosition == NULL) ? NULL : directoryNamePosition + 1;
}

ssize_t calculate_length_of_absolute_path_without_directory_name(char* directoryPath) {
    char* directoryNamePosition = find_directory_name_in_path(directoryPath);
    if (directoryNamePosition == NULL) {
        return DIRECTORY_NAME_NOT_FOUND;
    }
    size_t absolutePathSize = strlen(directoryPath);
    size_t directoryNameSize = strlen(directoryNamePosition);
    return (ssize_t) (absolutePathSize - directoryNameSize);
}

enum DirectoryErrors create_directory_with_reversed_name(char* directoryPath, char* directoryPathWithReversedName) {
    ssize_t lengthOfAbsolutePathWithoutName = calculate_length_of_absolute_path_without_directory_name(directoryPath);
    if (lengthOfAbsolutePathWithoutName == DIRECTORY_NAME_NOT_FOUND) {
        fprintf(stderr, "Cannot find directory name in input path!");
        return DIRECTORY_NAME_NOT_FOUND;
    }

    char absolutePathWithoutName[lengthOfAbsolutePathWithoutName];
    find_absolute_path_without_directory_name(absolutePathWithoutName, directoryPath, lengthOfAbsolutePathWithoutName);

    /* Previous steps ensures that the directory name exists */
    char* directoryName = find_directory_name_in_path(directoryPath);
    char reversedDirectoryName[strlen(directoryName)];
    reverse_string(directoryName, reversedDirectoryName);

    char* newDirectoryPath = strcat(absolutePathWithoutName, reversedDirectoryName);
    enum DirectoryErrors creationStatus = mkdir(newDirectoryPath, READ_WRITE_EXECUTE);
    if (creationStatus == DIRECTORY_WAS_NOT_CREATED) {
        fprintf(stderr, "Directory was not created!");
        return DIRECTORY_WAS_NOT_CREATED;
    }
    directoryPathWithReversedName = strcpy(directoryPathWithReversedName, newDirectoryPath);
    return OK;
}

enum DirectoryErrors is_this_directory_existing(char* directoryPath) {
    DIR* openedDirectory = opendir(directoryPath);
    if (openedDirectory == NULL) {
        fprintf(stderr, "Directory does not exist!");
        return DIRECTORY_DOESNT_EXIST;
    }

    enum DirectoryErrors closingStatus = closedir(openedDirectory);
    if (closingStatus == FAILED_DIRECTORY_CLOSING) {
        fprintf(stderr, "Cannot closing the directory!");
    }
    return closingStatus;
}

char* prepare_file_path(char* directoryPath, char* fileName) { // FIXME
    char delimiterSymbol = '/';
    size_t totalFilePathSize = strlen(directoryPath) + 1;
    char* copyOfDirectoryPath = malloc(totalFilePathSize * sizeof(char));
    if (copyOfDirectoryPath == NULL) {
        return NULL;
    }
    copyOfDirectoryPath = strcpy(copyOfDirectoryPath, directoryPath);
    copyOfDirectoryPath[totalFilePathSize - 1] = delimiterSymbol;
    return strcat(copyOfDirectoryPath, fileName);
}

void backward_copying_files(FILE* inputFile, FILE* outputFile) {
    fseek(inputFile, 0, SEEK_END);
    long fileSize = ftell(inputFile);
    char readingBuffer[BUFFER_SIZE];

    while (fileSize > 0) {
        long readSize = BUFFER_SIZE;
        if (readSize > fileSize) {
            readSize = fileSize;
        }
        fseek(inputFile, -readSize, SEEK_CUR);
        fread(readingBuffer, 1, readSize, inputFile);
        for (long i = readSize - 1; i >= 0; --i) {
            fprintf(outputFile, "%c", readingBuffer[i]);
        }
        fileSize -= readSize;
    }
}

enum FileErrors copy_file_content(char* directoryPath, char* fileName, char* directoryPathWithReversedName) {
    char* filePath = prepare_file_path(directoryPath, fileName);
    if (filePath == NULL) {
        fprintf(stderr, "Not enough of memory!");
        return MEMORY_ALLOCATION_ERROR;
    }

    char reversedFileName[strlen(fileName)];
    reverse_string(fileName, reversedFileName);
    char* reversedFilePath = prepare_file_path(directoryPathWithReversedName, reversedFileName);
    if (reversedFilePath == NULL) {
        free(filePath);
        fprintf(stderr, "Not enough of memory!");
        return MEMORY_ALLOCATION_ERROR;
    }

    printf("%s\n", filePath);
    FILE* inputFile = fopen(filePath, "r");
    if (inputFile == NULL) {
        free(filePath);
        free(reversedFilePath);
        fprintf(stderr, "Invalid file path!");
        return CANNOT_OPEN_FILE;
    }
    free(filePath);

    FILE* outputFile = fopen(reversedFilePath, "w");
    if (outputFile == NULL) {
        free(reversedFilePath);
        fclose(inputFile);
        fprintf(stderr, "Invalid file path!");
        return CANNOT_OPEN_FILE;
    }
    free(reversedFilePath);

    backward_copying_files(inputFile, outputFile);
    fclose(inputFile);
    fclose(outputFile);

    return SUCCESSFULLY_COPIED;
}

enum DirectoryErrors close_opened_directories(DIR* openedOriginDirectory, DIR* openedReversedDirectory) {
    enum DirectoryErrors closingStatus = closedir(openedOriginDirectory);
    if (closingStatus == FAILED_DIRECTORY_CLOSING) {
        fprintf(stderr, "Cannot closing the directory!");
        return FAILED_DIRECTORY_CLOSING;
    }
    closingStatus = closedir(openedReversedDirectory);
    if (closingStatus == FAILED_DIRECTORY_CLOSING) {
        fprintf(stderr, "Cannot closing the directory!");
        return FAILED_DIRECTORY_CLOSING;
    }
    return OK;
}

enum DirectoryErrors copy_all_content_of_directory(char* directoryPath, char* directoryPathWithReversedName) {
    DIR* openedOriginDirectory = opendir(directoryPath);
    DIR* openedReversedDirectory = opendir(directoryPathWithReversedName);
    if (openedOriginDirectory == NULL || openedReversedDirectory == NULL) {
        fprintf(stderr, "Directory does not exist!");
        return DIRECTORY_DOESNT_EXIST;
    }

    struct dirent* directoryEntry;
    directoryEntry = readdir(openedOriginDirectory);
    enum FileErrors copyStatus;
    while (directoryEntry != NULL) {
        if (directoryEntry->d_type == DT_REG) {
            char* fileName = directoryEntry->d_name;
            copyStatus = copy_file_content(directoryPath, fileName, directoryPathWithReversedName);
            if (copyStatus != SUCCESSFULLY_COPIED) {
                break;
            }
        } else if (directoryEntry->d_type == DT_DIR) {
            printf("dir\n"); // TODO: work on it.
        }
        directoryEntry = readdir(openedOriginDirectory);
    }

    enum DirectoryErrors closingStatus = close_opened_directories(openedOriginDirectory, openedReversedDirectory);
    if (copyStatus != OK) {
        return CONTENT_COPY_ERROR;
    }
    return closingStatus;
}

int main(int argc, char** argv) {
    if (argc != NUMBER_OF_REQUIRED_ARGS) {
        fprintf(stderr, "Invalid number of arguments!");
        return EXIT_FAILURE;
    }

    char* directoryPath = argv[1];
    enum DirectoryErrors existingStatus = is_this_directory_existing(directoryPath);
    if (existingStatus != OK) {
        return EXIT_FAILURE;
    }

    char directoryPathWithReversedName[strlen(directoryPath)];
    enum DirectoryErrors creationStatus = create_directory_with_reversed_name(directoryPath, directoryPathWithReversedName);
    if (creationStatus != OK) {
        return EXIT_FAILURE;
    }

    enum DirectoryErrors copyStatus = copy_all_content_of_directory(directoryPath, directoryPathWithReversedName);
    if (copyStatus != OK) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
