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
    FAILED_DIRECTORY_CLOSING = -1,
    DIRECTORY_NAME_NOT_FOUND = -1,
    DIRECTORY_WAS_NOT_CREATED = -1
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

    existingStatus = is_this_directory_existing(directoryPathWithReversedName);
    if (existingStatus != OK) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
