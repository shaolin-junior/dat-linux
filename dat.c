#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "println2.h"

#define ERROR_NOT_EXISTS 1
#define ERROR_SAME_FILE 2
#define ERROR_CAT_ACCESS 3

#define BUFFER_SIZE 32770

#define DAT_VERSION "0.7.0"
#define DEV_VERSION "0.7.0.6"

extern int dasm(int fd);

typedef enum{
    ALL_RIGHT = 0,
    ERROR_READ_FILE,
    ERROR_OPEN_FILE,
    ERROR_WRITE_FILE,
    ERROR_PIPE,

} OutputCode;

int validate_cat_args(const char *path_a, const char *path_b, const char *final_path) {
    struct stat st1, st2, stf;
    if (stat(path_a, &st1) != 0) {
        if (errno == ENOENT) {
            return ERROR_NOT_EXISTS;
        }
        return ERROR_CAT_ACCESS;
    }

    if (stat(path_b, &st2) != 0) {
        if (errno == ENOENT) {
            return ERROR_NOT_EXISTS;
        }
        return ERROR_CAT_ACCESS;
    }

    if (stat(final_path, &stf) != 0) {
        if (errno == ENOENT) {
            return 0;
        }
        return ERROR_CAT_ACCESS;
    }

    return ((st1.st_dev == stf.st_dev) && (st1.st_ino == stf.st_ino)) || ((st2.st_dev == stf.st_dev) && (st2.st_ino == stf.st_ino)) ? ERROR_SAME_FILE: 0;
}

void manage_cat_error(int err) {
    switch (err) {
        case ERROR_NOT_EXISTS:
            println("Error: uno de los archivos a concatenar no existe. Por favor revise si lo ha escrito correctamente.");
            break;
        case ERROR_SAME_FILE:
            println("Error: no puede concatenar un archivo dentro de sí mismo ya que el contenido del archivo donde se concatena se borra.");
            break;
        case ERROR_CAT_ACCESS:
            perror("dat");
            break;
    }
}

typedef struct {
    bool help;
    bool version;
    bool sha256;
    bool no_print;
    bool compare;
    bool concatenate;
    bool no_line_mark;
    const char *paths[3];
    int path_count;
} Arguments;

void manage_messages(OutputCode code) {
    switch (code)
    {
    case ERROR_READ_FILE:
    case ERROR_OPEN_FILE:
        perror("dat");
        break;
    case ERROR_WRITE_FILE:
        println("Error al escribir el archivo.");
        break;
    case ERROR_PIPE:
        println("Error con el pipe/splice.");
        break;
    case ALL_RIGHT:
        println("Sin errores/Error desconocido.");
        break;
    }
}

int parse_args(int argc, char *argv[], Arguments *args) {
    *args = (Arguments){0};

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            if (args->help) {
                println("Demasiadas opciones.");
                return 1;
            }
            args->help = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0){
            if (args->version) {
                println("Demasiadas opciones.");
                return 1;
            }
            args->version = true;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sha_256") == 0) {
            if (args->sha256) {
                println("Demasiadas opciones.");
                return 1;
            }
            args->sha256 = true;
        } else if (strcmp(argv[i], "--no-print") ==0) {
            if (args->no_print) {
                println("Demasiadas opciones.");
                return 1;
            }
            args->no_print = true;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compare") == 0) {
            if (args->compare) {
                println("Demasiadas opciones.");
                return 1;
            }
            args->compare = true;
        } else if (strcmp(argv[i], "--cat") == 0) {
            if (args->concatenate) {
                println("Demasiadas opciones.");
                return 1;
            }
            args->concatenate = true;
        } else if (strcmp(argv[i], "--no-line-n") == 0 || strcmp(argv[i], "-N") == 0) {
            if (args->no_line_mark) {
                println("Demasiadas opciones.");
                return 1;
            }
            args->no_line_mark = true;
        } else if (argv[i][0] == '-') {
            println("Opción desconocida: %s", argv[i]);
            return 1;
        } else {
            if (args->path_count == 3) {
                println("Error: demasiados archivos.");
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
        println("Error: No se pueden combinar varias operaciones.");
        return 1;
    } else if (args.no_print && !args.sha256) {
        println("Error: --no-print solo se puede usar con --sha_256 o -s.");
        return 1;
    } else if (args.sha256 && args.path_count == 0) {
        println("Uso: dat -s <archivo>");
        return 1;
    } else if (args.compare && !args.concatenate &&  args.path_count != 2) {
        println("Uso: dat -c <archivo1> <archivo2>");
        return 1;
    } else if (!args.sha256 && !args.help && !args.version && args.path_count == 0) {
        println("Error: Falta especificar el archivo.");
        return 1;
    } else if ((args.help && args.path_count > 0) || (args.help && args.concatenate) || (args.help && args.no_line_mark)) {
        println("Uso: dat -h");
        return 1;
    } else if ((args.version && args.path_count > 0) || (args.version && args.concatenate) || (args.help && args.no_line_mark)) {
        println("Uso: dat -v");
        return 1;
    } else if (!args.compare && !args.sha256 && !args.concatenate && args.path_count > 1) {
        println("Error: demasiados archivos.");
        return 1;
    } else if ( args.sha256 && ((args.path_count > 1 && !args.concatenate) || (args.concatenate && args.path_count > 3))) {
        println("Error: Demasiados archivos");
        return 1;
    } else if (args.concatenate && args.path_count != 3) {
        println("Uso: dat [operador] --cat <archivo1> <archivo2> <archivo3>");
        return 1;
    } else if (args.no_print && args.no_line_mark) {
        println("Error: no se pueden mezclar estas 2 opciones.");
        return 1;
    } else if ((args.compare && args.no_line_mark) || (args.concatenate && args.no_line_mark && !args.sha256)) {
        println("Error: no se pueden mezclar estas 2 opciones.");
    }
    return 0;
}

void print_separator(const char *top) {
    for (int i = 0; i < 145; i++){
        if (i == 6) {
            printf("%s", top);
            continue;
        }
        printf("─");
    }
    printf("\n");
}

void print_help(void) {
    println("Ayuda:\n Versión %s\n\nSintaxis:", DAT_VERSION);
    println("\nComandos básicos:\n -h o --help: Muestra el menú de ayuda.\n -v o --version: Muestra la versión del programa.");
    println(" -s o --sha_256: Muestra el hash del archivo en formato sha256.\n\t\t Se le añade --no-print para no mostrar el contenido del archivo. Se le añade --cat para concatenar 2 archivos en 1. Se le añade -N para no mostrar numero de linea.");
    printf(" -c o --compare: Compara dos archivos.\n\t\t Se le añade --cat para concatenar dos archivos.");
    printf("\n dat <ruta/archivo>\n dat [opción] <ruta/archivo>\n dat -s [flag] <archivo1> <archivo2> <archivo3>\n dat -c [flag] <archivo1> <archivo2> <archivo3>\n dat [-v/--version/-h/--help]\n dat --cat <archivo1> <archivo2> <archivo3>");
}

FILE *open_file(const char *path, const char *mode) {
    return fopen(path, mode);
}

int print_file(FILE *file) {
    int line = 1;

    char buffer[BUFFER_SIZE];
    char *buffer2 = calloc(BUFFER_SIZE*16, sizeof(char));
    if (buffer2 == NULL) {
        return 1;
    }
    size_t y = 0;
    size_t read = 0;
    
    print_separator("┬");

    printf(" %4d │ ", line);
    while((read = fread(buffer, sizeof(char), BUFFER_SIZE, file)) > 0) {
        
        for (size_t i = 0; i < read; i++) {
            if (buffer[i] == '\n') {
                line ++;
                int result = snprintf(&buffer2[y], 16, "\n %4d │ ", line);
                y += result;
                continue;
            } else {
                buffer2[y] = buffer[i];
                y++;
            }
        }
        buffer2[y] = '\0';
        fwrite(buffer2, sizeof(char), read, stdout);
        y = 0;
    }

    free(buffer2);

    printf("\n");

    print_separator("┴");

    printf("\n");
    if (ferror(file)) {
        return 1;
    }
    return 0;
}

int display_file(const char *path, OutputCode *code, Arguments args) {
    FILE *file = open_file(path, "rb");
    if (file == NULL) {
        *code = ERROR_OPEN_FILE;
        return 1;
    }
    if (args.no_line_mark) {
        print_separator("─");
        int error = dasm(fileno(file));
        printf("\n");
        print_separator("─");
        if (error) {
            switch (error)
            {
            case 1:
                *code = ERROR_READ_FILE;
                break;
            case 2:
                *code = ERROR_PIPE;
                break;
            }
            fclose(file);
            return 1;
        }
        fclose(file);
        return 0;
    }

    int error = print_file(file);
    if (error) {
        *code = ERROR_READ_FILE;
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
    printf("\n");
    print_separator("┬");
    println("      │ Archivo: %s", path);
    printf("      │ sha256: ");

    for (unsigned int i = 0; i < hash_len; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
    print_separator("┴");
}

int compare_files(const char *path_a, const char *path_b, OutputCode *code) {
    FILE *file_a = open_file(path_a, "rb");
    FILE *file_b = open_file(path_b, "rb");

    if (file_a == NULL || file_b == NULL) {
        if (file_a != NULL) fclose(file_a);
        if (file_b != NULL) fclose(file_b);
        *code = ERROR_OPEN_FILE;
        return 1;
    }

    int a;
    int b;
    int result = 0;
    
    while(true) {
        a = fgetc(file_a);
        b = fgetc(file_b);

        if (a != b) {
            if(ferror(file_a) || ferror(file_b)){
                *code = ERROR_READ_FILE;
                result = 1;
                break;
            }
            result = 2;
            break;
        }

        if (a == EOF && b == EOF) {
            if(ferror(file_a) || ferror(file_b)){
                *code = ERROR_READ_FILE;
                result = 1;
                break;
            }
            break;
        }
    }
    fclose(file_a);
    fclose(file_b);
    return result;
}

int print_concatenate(FILE *file, FILE *destiny) {
    char buffer[4096];
    size_t read_bytes;

    while((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        size_t written_bytes = fwrite(buffer, 1, read_bytes, destiny);

        if (written_bytes < read_bytes) {
            return 1;
        }
    }
    if(ferror(file)) {
        return 2;
    }
    return 0;
}

int concatenate(const char *path_a, const char *path_b, const char *path_final, OutputCode *code) {
    FILE *file_a = open_file(path_a, "rb");
    FILE *file_b = open_file(path_b, "rb");

    if (file_a == NULL || file_b == NULL) {
        *code = ERROR_OPEN_FILE;

        if (file_a != NULL) fclose(file_a);
        if (file_b != NULL) fclose(file_b);

        return 1;
    }

    FILE *destiny = open_file(path_final, "wb");

    if (destiny == NULL) {
        *code = ERROR_OPEN_FILE;

        fclose(file_a);
        fclose(file_b);

        return 1;
    }

    int status = 0;

    
    int result = print_concatenate(file_a, destiny);
    if (result == 1) {
        *code = ERROR_WRITE_FILE;
        status = 1;

    } else if (result == 2) {
        *code = ERROR_READ_FILE;
        status = 1;
    }

    if (status == 0) {
        result = print_concatenate(file_b, destiny);
        if (result == 1) {
            *code = ERROR_WRITE_FILE;
            status = 1;

        } else if (result == 2) {
            *code = ERROR_READ_FILE;
            status = 1;
        }
    }

    fclose(file_a);
    fclose(file_b);
    if (fclose(destiny) != 0) {
        *code = ERROR_WRITE_FILE;
        status = 1;
    } 
    return status;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        println("Mire dat --help para ver como usar el programa.");
        return 1;
    }

    if (strcmp(argv[1], "-dev") == 0) {
        println("\nDev information:\n Versión: %s\n Dev version: %s\n", DAT_VERSION, DEV_VERSION);
        return 0;
    }

    Arguments args;
    OutputCode code = ALL_RIGHT;

    if (parse_args(argc, argv, &args)) {
        return 1;
    }
    if (validate_args(args)) {
        return 1;
    }
    if (args.version) {
        println("dat %s\n", DAT_VERSION);
        return 0;
    } else if (args.help) {
        print_help();
        return 0;
    } else if (args.sha256) {

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
                
                if(display_file(args.paths[i], &code, args)) {
                    manage_messages(code);
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
            int result = concatenate(args.paths[0], args.paths[1], args.paths[2], &code);
            if (result) {
                manage_messages(code);
                return 1;
            }
        }
        return 0;
    } else if (args.compare) {
        int result = compare_files(args.paths[0], args.paths[1], &code);
        if (!result) {
            println("Los archivos %s y %s son iguales.", args.paths[0], args.paths[1]);
        } else if (result == 2) {
            println("Los archivos %s y %s son diferentes.", args.paths[0], args.paths[1]);
        } else {
            manage_messages(code);
            return 1;
        }

        if (args.concatenate) {
            int err = validate_cat_args(args.paths[0], args.paths[1], args.paths[2]);
            if (err > 0) {
                manage_cat_error(err);
                return 1;
            }
            int result = concatenate(args.paths[0], args.paths[1], args.paths[2], &code);
            if (result) {
                manage_messages(code);
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
        int result = concatenate(args.paths[0], args.paths[1], args.paths[2], &code);
        if(result) {
            manage_messages(code);
            return 1;
        }
        return 0;
    } else {
        printf("\n");
        print_separator("┬");
        println("      │ Archivo: %s", args.paths[0]);
        print_separator("┴");

        if(display_file(args.paths[0], &code, args)) {
            manage_messages(code);
            return 1;
        }
        return 0;
    }
}