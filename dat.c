#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <openssl/evp.h>

#define DAT_VERSION "0.3.6"
#define DEV_VERSION "0.3.6.9"

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
        printf("Ayuda:\n Versión %s\n\nComandos básicos:\n -h o --help: Muestra el menú de ayuda.\n -v o --version: Muestra la versión del programa.\n -s o --sha_256: Muestran el hash del archivo en formato sha256. Se le añade --no-print para no mostrar el contenido del archivo.\n\nSintaxis:\n dat [opción] <ruta/archivo>", DAT_VERSION);
        return 0;
    }
    if (strcmp(argv[1], "--sha_256") == 0 || strcmp(argv[1], "-s") == 0) {
        if(argc < 3) {
            printf("Uso: dat -s <archivo>\n");
            return 1;
        }
        if (strcmp(argv[2], "--no-print") == 0) {
            if(argc < 4) {
                printf("Uso: dat -s --no-print <archivo>\n");
                return 1;
            }
            const char *path = argv[3];
            unsigned char hash[32];
            unsigned int hash_len;
            int result = sha_256(path, hash, &hash_len);
            if (result) {
                perror("dat");
                return 1;
            }

            
            printf("Archivo: %s \n\n", path);

            printf("sha256: ");

            for (unsigned int i = 0; i < hash_len; i++) {
                printf("%02x", hash[i]);
            }
            printf("\n\n");

            return 0;
        }
        const char *path = argv[2];
        unsigned char hash[32];
        unsigned int hash_len;
        int result = sha_256(path, hash, &hash_len);
        if (result) {
            perror("dat");
            return 1;
        }

        
        printf("Archivo: %s \n\n", path);

        printf("sha256: ");

        for (unsigned int i = 0; i < hash_len; i++) {
            printf("%02x", hash[i]);
        }
        printf("\n\n");

        FILE *file = open_file(path, "r");

        if (file == NULL) {
            perror("dat");
            return 1;
        }

        int error = print_file(file);
        if (error) {
            printf("\nError al leer el archivo.");
            fclose(file);
            return 1;
        }

        fclose(file);
        return 0;
    }
    const char *path = argv[1];
    

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