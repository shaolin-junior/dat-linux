#include <stdio.h>
#include <string.h>
#include <windows.h>

#define DAT_VERSION "0.2.5"
#define DEV_VERSION "0.2.5.1"

FILE *read_file(const char *path) {
    FILE *file_open = fopen(path, "r");
    if (file_open == NULL) {
        return NULL;
    }
    return file_open;
}

int print_file(FILE *file) {
    int c;
    int line = 1;
    printf(" %4d | ", line);
    while((c = fgetc(file)) != EOF) {
        printf("%c", c);

        if (c == '\n') {
            line ++;
            printf(" %4d | ", line);
        }
    }
    if (ferror(file)) {
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if(argc < 2) {
        printf("Mire dat --help para ver como usar el programa.");
        return 1;
    }
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v" )== 0) {
        printf("dat %s\n\n", DAT_VERSION);
        return 0;
    } 
    if (strcmp(argv[1], "-dev") == 0) {
        printf("\nDev information:\n Versión: %s\n Dev version: %s\n\n", DAT_VERSION, DEV_VERSION);
        return 0;
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("Ayuda:\n Versión %s\n\nComandos básicos:\n -h o --help: Muestra el menú de ayuda.\n -v o --version: Muestra la versión del programa.\n\nSintaxis:\n dat [opción] <ruta/archivo>", DAT_VERSION);
        return 0;
    }
    const char *path = argv[1];

    FILE *file = read_file(path);
    if (file == NULL) {
        perror("dat");
        return 1;
    }
    printf("Archivo: %s \n\n", path);

    int error = print_file(file);
    if (error) {
        printf("\nError al leer el archivo.");
        fclose(file);
        return 1;
    }

    fclose(file);
    
    return 0;
}