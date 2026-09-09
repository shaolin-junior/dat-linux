#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include <openssl/evp.h>

#define DAT_VERSION "0.3.8"
#define DEV_VERSION "0.3.8.16"

typedef struct {
    bool help;
    bool version;
    bool sha256;
    bool no_print;
    const char *path;
} Arguments;

int parse_args(int argc, char *argv[], Arguments *args) {
    *args = (Arguments){0};

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            if (args->help) {
                printf("Demasiadas opciones.");
                return 1;
            }
            args->help = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0){
            if (args->version) {
                printf("Demasiadas opciones.");
                return 1;
            }
            args->version = true;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sha_256") == 0) {
            if (args->sha256) {
                printf("Demasiadas opciones.");
                return 1;
            }
            args->sha256 = true;
        } else if (strcmp(argv[i], "--no-print") ==0) {
            if (args->no_print) {
                printf("Demasiadas opciones.");
                return 1;
            }
            args->no_print = true;
        } else if (argv[i][0] == '-') {
                printf("Opción desconocida: %s\n", argv[i]);
                return 1;
        } else {
            if (args->path != NULL) {
                printf("Error: demasiados archivos.\n");
                return 1;
            }
            args->path = argv[i];
        }
    }
    return 0;
}

int validate_args(Arguments args) {
    int commands = 0;
    
    if (args.help) commands++;
    if (args.version) commands++;
    if (args.sha256) commands++;

    if (commands > 1) {
        printf("No se pueden combinar varias operaciones.\n");
        return 1;
    } else if (args.no_print && !args.sha256) {
        printf("--no-print solo se puede usar con --sha_256 o -s.\n");
        return 1;
    } else if (args.sha256 && args.path == NULL) {
        printf("Uso: dat -s <archivo>\n");
        return 1;
    } else if (!args.sha256 && !args.help && !args.version && args.path == NULL) {
        printf("Falta especificar el archivo.\n");
        return 1;
    } else if (args.help && args.path != NULL) {
        printf("Uso: dat -h\n");
        return 1;
    } else if (args.version && args.path != NULL) {
        printf("Uso: dat -v");
        return 1;
    }
    return 0;
}

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

    if (strcmp(argv[1], "-dev") == 0) {
        printf("\nDev information:\n Versión: %s\n Dev version: %s\n\n", DAT_VERSION, DEV_VERSION);
        return 0;
    }

    Arguments args;

    if (parse_args(argc, argv, &args)) {
        return 1;
    }
    if (validate_args(args)) {
        return 1;
    }

    if (args.version) {
        printf("dat %s\n\n", DAT_VERSION);
        return 0;
    } 
    
    if (args.help) {
        print_help();
        return 0;
    }
    if (args.sha256) {

        unsigned char hash[32];
        unsigned int hash_len;
        int result = sha_256(args.path, hash, &hash_len);
        if (result) {
            perror("dat");
            return 1;
        }

        
        print_hash(hash, hash_len, args.path);

        if (!args.no_print) {
            if(display_file(args.path)) {
                return 1;
            }
        }
        return 0;
    }

    if(display_file(args.path)) {
        return 1;
    }
    
    return 0;
}