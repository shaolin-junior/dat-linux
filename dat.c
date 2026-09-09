#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include <openssl/evp.h>

#define DAT_VERSION "0.3.6"
#define DEV_VERSION "0.3.6.16"

void print_help(void) {
    printf("Ayuda:\n Versión %s\n\nSintaxis:\n", DAT_VERSION);
    printf("\nComandos básicos:\n -h o --help: Muestra el menú de ayuda.\n -v o --version: Muestra la versión del programa.\n");
    printf(" -s o --sha_256: Muestra el hash del archivo en formato sha256.\n\t\t Se le añade --no-print para no mostrar el contenido del archivo.\n");
    printf("\n dat <ruta/archivo>\n dat [opción] <ruta/archivo>\n dat -s --no-print <archivo>\n dat [-v/--version/-h/--help]\n");
}

FILE *open_file(const char *path, const char *mode) {
    FILE *file_open = fopen(path, mode);
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
            line++;
            printf(" %4d | ", line);
        }
    }
    if (ferror(file)) {
        return 1;
    }
    return 0;
}

int display_file(const char *path) {
    FILE *file = open_file(path, "r");
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

int sha_256(const char *path, unsigned char hash[32], unsigned int *hash_len) {
    FILE *file = open_file(path, "rb");
    if (file == NULL) {
        return 1;
    }
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();

    if (!ctx) {
        fclose(file);
        return 1;
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        fclose(file);
        return 1;
    }

    unsigned char buffer[4096];
    size_t bytes;

    while((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (EVP_DigestUpdate(ctx, buffer, bytes) != 1) {
            EVP_MD_CTX_free(ctx);
            fclose(file);
            return 1;
        }
    }
    if(ferror(file)) {
        EVP_MD_CTX_free(ctx);
        fclose(file);
        return 1;
    }

    if (EVP_DigestFinal_ex(ctx, hash, hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        fclose(file);
        return 1;
    }

    EVP_MD_CTX_free(ctx);
    fclose(file);
    return 0;
}

void print_hash(const unsigned char *hash, unsigned int hash_len, const char *path) {
    printf("Archivo: %s \n\n", path);

    printf("sha256: ");

    for (unsigned int i = 0; i < hash_len; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n\n");
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
        print_help();
        return 0;
    }
    if (strcmp(argv[1], "--sha_256") == 0 || strcmp(argv[1], "-s") == 0) {
        if(argc < 3) {
                printf("Uso: dat -s <archivo>\n");
                return 1;
        }

        const char *path;
        bool not_print;

        if (strcmp(argv[2], "--no-print") == 0) {
            if(argc != 4) {
                printf("Uso: dat -s --no-print <archivo>\n");
                return 1;
            }
            path = argv[3];
            not_print = true;
        } else {
            if(argc != 3) {
                printf("Uso: dat -s <archivo>\n");
                return 1;
            }
            path = argv[2];
            not_print = false;
        }
        unsigned char hash[32];
        unsigned int hash_len;
        int result = sha_256(path, hash, &hash_len);
        if (result) {
            perror("dat");
            return 1;
        }

        
        print_hash(hash, hash_len, path);

        if (!not_print) {

            if(display_file(path)) {
                return 1;
            }
        }
        return 0;
    }
    const char *path = argv[1];
    

    if(display_file(path)) {
        return 1;
    }
    
    return 0;
}