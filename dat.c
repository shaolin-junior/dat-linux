#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <openssl/evp.h>

#define ERROR_NOT_EXISTS 1
#define ERROR_SAME_FILE 2

#define DAT_VERSION "0.5.8"
#define DEV_VERSION "0.5.8.30"

#ifdef _WIN32
    #include <windows.h>

    int validate_cat_args(const char *path_a, const char *path_b, const char *final_path) {
        HANDLE h_a = CreateFileA(path_a, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        HANDLE h_b = CreateFileA(path_b, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

        if (h_a == INVALID_HANDLE_VALUE || h_b == INVALID_HANDLE_VALUE) {
            if (h_a != INVALID_HANDLE_VALUE) CloseHandle(h_a);
            if (h_b != INVALID_HANDLE_VALUE) CloseHandle(h_b);
            return ERROR_NOT_EXISTS;
        }

        HANDLE h_f = CreateFileA(final_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (h_f == INVALID_HANDLE_VALUE) {
            CloseHandle(h_a);
            CloseHandle(h_b);
            return 0;
        }

        BY_HANDLE_FILE_INFORMATION info_a, info_b, info_f;
        int result = 0;

        if (GetFileInformationByHandle(h_a, &info_a) && 
            GetFileInformationByHandle(h_b, &info_b) && 
            GetFileInformationByHandle(h_f, &info_f)) {
            
            bool same_a = (info_f.dwVolumeSerialNumber == info_a.dwVolumeSerialNumber) && (info_f.nFileIndexHigh == info_a.nFileIndexHigh) && (info_f.nFileIndexLow == info_a.nFileIndexLow);

            bool same_b = (info_f.dwVolumeSerialNumber == info_b.dwVolumeSerialNumber) && (info_f.nFileIndexHigh == info_b.nFileIndexHigh) && (info_f.nFileIndexLow == info_b.nFileIndexLow);

            if (same_a || same_b) {
                result = ERROR_SAME_FILE;
            }
        }

        CloseHandle(h_a);
        CloseHandle(h_b);
        CloseHandle(h_f);

        return result;
    }
#else
    #include <sys/stat.h>
    int validate_cat_args(const char *path_a, const char *path_b, const char *final_path) {
        struct stat st1, st2, stf;
        if (stat(path_a, &st1) != 0 || stat(path_b, &st2) != 0) {
            return ERROR_NOT_EXISTS;
        }

        if (stat(final_path, &stf) != 0) {
            return 0;
        }

        return (st1.st_dev == stf.st_dev) && (st1.st_ino == stf.st_ino) || (st2.st_dev == stf.st_dev) && (st2.st_ino == stf.st_ino) ? ERROR_SAME_FILE: 0;
    }
#endif

typedef struct {
    bool help;
    bool version;
    bool sha256;
    bool no_print;
    bool compare;
    bool concatenate;
    const char *paths[3];
    int path_count;
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
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compare") == 0) {
            if (args->compare) {
                printf("Demasiadas opciones.");
                return 1;
            }
            args->compare = true;
        } else if (strcmp(argv[i], "--cat") == 0) {
            if (args->concatenate) {
                printf("Demasiadas opciones.");
                return 1;
            }
            args->concatenate = true;
        }else if (argv[i][0] == '-') {
            printf("Opción desconocida: %s\n", argv[i]);
            return 1;
        } else {
            if (args->path_count == 3) {
                printf("Error: demasiados archivos.\n");
                return 1;
            }
            args->paths[args->path_count] = argv[i];
            args->path_count++;
        }
    }
    return 0;
}

int validate_args(Arguments args) {
    int commands = 0;
    
    if (args.help) commands++;
    if (args.version) commands++;
    if (args.sha256) commands++;
    if (args.compare) commands++;

    if (commands > 1) {
        printf("No se pueden combinar varias operaciones.\n");
        return 1;
    } else if (args.no_print && !args.sha256) {
        printf("--no-print solo se puede usar con --sha_256 o -s.\n");
        return 1;
    } else if (args.sha256 && args.path_count == 0) {
        printf("Uso: dat -s <archivo>\n");
        return 1;
    } else if (!args.sha256 && !args.help && !args.version && args.path_count == 0) {
        printf("Falta especificar el archivo.\n");
        return 1;
    } else if ((args.help && args.path_count > 0) || (args.help && args.concatenate)) {
        printf("Uso: dat -h\n");
        return 1;
    } else if ((args.version && args.path_count > 0) || (args.version && args.concatenate)) {
        printf("Uso: dat -v");
        return 1;
    } else if (!args.compare && !args.sha256 && !args.concatenate && args.path_count > 1) {
        printf("Error: demasiados archivos.");
        return 1;
    } else if (args.compare && !args.concatenate &&  args.path_count != 2) {
        printf("Uso: dat -c <archivo1> <archivo2>");
        return 1;
    } else if (args.concatenate && args.path_count != 3) {
        printf("Uso dat [operador] --cat <archivo1> <archivo2> <archivo3>");
        return 1;
    }
    return 0;
}

void print_help(void) {
    printf("Ayuda:\n Versión %s\n\nSintaxis:\n", DAT_VERSION);
    printf("\nComandos básicos:\n -h o --help: Muestra el menú de ayuda.\n -v o --version: Muestra la versión del programa.\n");
    printf(" -s o --sha_256: Muestra el hash del archivo en formato sha256.\n\t\t Se le añade --no-print para no mostrar el contenido del archivo. Se le añade --cat para concatenar 2 archivos en 1.\n");
    printf(" -c o --compare: Compara dos archivos.\n\t\t Se le añade --cat para concatenar dos archivos.");
    printf("\n dat <ruta/archivo>\n dat [opción] <ruta/archivo>\n dat -s [flag] <archivo1> <archivo2> <archivo3>\n dat -c [flag] <archivo1> <archivo2> <archivo3>\n dat [-v/--version/-h/--help]\n dat --cat <archivo1> archivo2> archivo3>");
}

FILE *open_file(const char *path, const char *mode) {
    return fopen(path, mode);
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
    printf("\n");
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
    printf("\nArchivo: %s \n\n", path);

    printf("sha256: ");

    for (unsigned int i = 0; i < hash_len; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n\n");
}

int compare_files(const char *path_a, const char *path_b) {
    FILE *file_a = open_file(path_a, "rb");
    FILE *file_b = open_file(path_b, "rb");

    if (file_a == NULL || file_b == NULL) {
        if (file_a != NULL) fclose(file_a);
        if (file_b != NULL) fclose(file_b);
        return 1;
    }
    int a;
    int b;
    
    while(true) {
        a = fgetc(file_a);
        b = fgetc(file_b);

        if (a != b) {
            fclose(file_a);
            fclose(file_b);
            return 2;
        }

        if (a == EOF && b == EOF) {
            break;
        }
    }
    fclose(file_a);
    fclose(file_b);
    return 0;    
}

int print_concatenate(FILE *file, FILE *destiny) {
    char buffer[4096];
    size_t read_bytes;

    while((read_bytes = fread(buffer, 1, 4096, file)) > 0) {
        size_t written_bytes = fwrite(buffer, 1, read_bytes, destiny);

        if (written_bytes < read_bytes) {
            return 1;
        }
    }
    return 0;
}

int concatenate(const char *path_a, const char *path_b, const char *path_final) {
    FILE *file_a = open_file(path_a, "rb");
    FILE *file_b = open_file(path_b, "rb");
    FILE *destiny = open_file(path_final, "wb");
    int status = 0;

    if (file_a == NULL || file_b == NULL || destiny == NULL) {
        status = 2;
    }

    if (status == 0) {
        if (print_concatenate(file_a, destiny)) {
            status = 1;
        }
    }

    if (status == 0) {
        if (print_concatenate(file_b, destiny)) {
            status = 1;
        }
    }

    if (file_a != NULL) fclose(file_a);
    if (file_b != NULL) fclose(file_b);
    if (destiny != NULL) fclose(destiny);
    return status;
}

void manage_cat_error(int err) {
    switch (err) {
        case ERROR_NOT_EXISTS:
            printf("Error: uno de los archivos a concatenar no existe. Por favor revise si lo ha escrito correctamente.");
            break;
        case ERROR_SAME_FILE:
            printf("Error: no puede concatenar un archivo dentro de sí mismo ya que el contenido del archivo donde se concatena se borra.");
            break;
    }
}

int main(int argc, char *argv[]) {

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (argc < 2) {
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
    } else if (args.help) {
        print_help();
        return 0;
    } else if (args.sha256) {

        if ((args.path_count > 1 && !args.concatenate) || (args.concatenate && args.path_count > 3)) {
            printf("Demasiados archivos");
            return 1;
        }

        int files_to_hash = args.concatenate ? 2 : 1;

        for(int i = 0; i < files_to_hash; i++) {
            unsigned char hash[32];
            unsigned int hash_len;
            int result = sha_256(args.paths[i], hash, &hash_len);
            if (result) {
                perror("dat");
                return 1;
            }

            
            print_hash(hash, hash_len, args.paths[i]);

            if (!args.no_print) {
                
                if(display_file(args.paths[i])) {
                    return 1;
                }
            }
        }
        if (args.concatenate) {
            int err = validate_cat_args(args.paths[0], args.paths[1], args.paths[2]);
            if (err > 0) {
                manage_cat_error(err);
                return 1;
            }
            int  result = (concatenate(args.paths[0], args.paths[1], args.paths[2]));
            if (result == 1) {
                printf("Error al escribir el archivo.");
                return 1;
            }
            if (result == 2) {
                perror("dat");
                return 1;
            }
        }
        return 0;
    } else if (args.compare) {
        int result = compare_files(args.paths[0], args.paths[1]);
        switch (result)
        {
        case 0:
            printf("Los archivos %s y %s son iguales.\n", args.paths[0], args.paths[1]);
            break;
        case 1:
            perror("dat");
            return 1;
        case 2:
            printf("Los archivos %s y %s son diferentes.\n", args.paths[0], args.paths[1]);
            break;
        }
        if (args.concatenate) {
            int err = validate_cat_args(args.paths[0], args.paths[1], args.paths[2]);
            if (err > 0) {
                manage_cat_error(err);
                return 1;
            }
            int  result = (concatenate(args.paths[0], args.paths[1], args.paths[2]));
            if (result == 1) {
                printf("Error al escribir el archivo.");
                return 1;
            }
            if (result == 2) {
                perror("dat");
                return 1;
            }
        }
        return 0;
    } else if (args.concatenate) {
        int err = validate_cat_args(args.paths[0], args.paths[1], args.paths[2]);
        if (err > 0) {
            manage_cat_error(err);
            return 1;
        }
        int result = (concatenate(args.paths[0], args.paths[1], args.paths[2]));
        if (result == 1) {
            printf("Error al escribir el archivo.");
            return 1;
        }
        if (result == 2) {
            perror("dat");
            return 1;
        }
        return 0;
    } else {
        printf("Archivo: %s \n\n", args.paths[0]);

        if(display_file(args.paths[0])) {
            return 1;
        }
        return 0;
    }
}