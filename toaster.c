#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>

#define VERSION "1.0.0"
#define DEFAULT_SRC_PATH "./tests"
#define LOG_PREFIX  "[TOASTER]"
#define DEFAULT_CAP 1024
#define CC "gcc"
#define GEN_FILE "tmp_toast.c"
#define EXECUTABLE "tmp_toast"
#define DEFIN_FILE "defin.test.c"
#define LOGS "logs"
#define NUM_GEN_FILES 3
#define FILE_HEADER_LEN 108
#define MAIN_DECL_LEN 64
#define MAIN_CLOSE_LEN 55
#define NUM_FLAGS 4
#define shift_arg(data, count) (assert((count) > 0), (count)--, *(data)++)

#define append_one(ds, item)                            \
    do {                                                \
        if ((ds)->len >= (ds)->cap) {                   \
            if ((ds)->cap == 0) {                        \
                (ds)->cap = DEFAULT_CAP;                \
                (ds)->items = malloc((ds)->cap*sizeof(*(ds)->items));    \
            } else {                                    \
                (ds)->cap *= 2;                         \
                (ds)->items = realloc(                      \
                    (ds)->items,                        \
                    (ds)->cap*sizeof(*(ds)->items));    \
            }                                           \
            if ((ds)->items == NULL) {                  \
                fprintf(stderr,                         \
                        "[TOASTER][ERROR] could not reallocate\n"); \
                exit(1);                               \
            }                                           \
        }                                               \
        (ds)->items[(ds)->len] = (item);              \
        (ds)->len++;                                   \
    } while (0)                                         \


#define append_many(ds, new_items, new_items_count)            \
    do { if ((ds)->len + (new_items_count) >= (ds)->cap) {         \
            if ((ds)->cap == 0) {                                  \
                (ds)->cap = DEFAULT_CAP;                           \
            }                                                      \
            while ((ds)->len + (new_items_count) >= (ds)->cap) {   \
                (ds)->cap *= 2;                                    \
            }                                                      \
            (ds)->items = realloc((ds)->items,                     \
                    (ds)->cap*sizeof(*(ds)->items));               \
            if ((ds)->items == NULL) {                             \
                fprintf(stderr, "[TOASTER][ERROR] could not reallocate\n"); \
                exit(1);                                           \
            }}                                                     \
        memcpy((ds)->items + (ds)->len,                            \
                (new_items),                                       \
                (new_items_count)*sizeof(*(ds)->items));          \
        (ds)->len += new_items_count;                              \
    } while (0)                                                    \

#define PRINT_STR(str)                                  \
    do {                                                 \
        char tmp[(str)->len+1];                           \
        for (int i = 0; i < (str)->len; ++i) {             \
            tmp[i] = (str)->items[i];                      \
        }                                                \
        tmp[(str)->len] = '\0';                            \
        printf("%s\n", tmp);                                  \
    } while (0)                                          \

char* flags[NUM_FLAGS*2] = {
    "-d", "--dir", 
    "-k", "--keep", 
    "-v", "--version", 
    "-h", "--help"};
char* explanations[NUM_FLAGS] = {
    "-d|--dir <dir>.......... specifies the src directory. [Default: '"DEFAULT_SRC_PATH"']",
    "-k|--keep .............. toaster won't remove the files it generated",
    "-v|--version ........... print the current version of this toaster",
    "-h|--help .............. print this very text"
};

void usage(char* program, char* error) {
    printf("\nUsage:\n");
    printf("%s [OPTIONS]\n\n", program);
    for (size_t i = 0; i < NUM_FLAGS; ++i) {
        printf("%s\n", explanations[i]);
    }
    if (error != NULL) {
        printf(LOG_PREFIX"[ERROR] %s", error);
    } 
}

typedef struct {
    char* program;
    char* dir;
    int keep;
} Args;

Args args = {0};

void parse_args(int argc, char **argv) {
    char* program = shift_arg(argv, argc);
    args.program = program; 
    args.dir = DEFAULT_SRC_PATH;
    args.keep = 0;
    int parsed;
    while (argc > 0) {
        char* arg = shift_arg(argv, argc);
        parsed = 0;
        for (size_t i = 0; i < NUM_FLAGS*2; ++i) {
            if (strcmp(arg, flags[i]) == 0) {
                if (i < 2) {
                    if (argc == 0) {
                        usage(program, "Expected argument!\n");
                        exit(1);
                    } else {
                        args.dir = shift_arg(argv, argc);
                        parsed = 1;
                        break;
                    }
                } else {
                    size_t idx = i;
                    if (idx % 2 != 0) {
                        idx--;
                    }
                    switch (idx) {
                        case 2:
                            args.keep = 1;
                            parsed = 1;
                            break;
                        case 4:
                            printf("%s v%s\n", program, VERSION);
                            exit(0);
                        case 6:
                            usage(program, NULL);
                            exit(0);
                    }
                }
            } 
        }
        if (parsed == 0) {
            usage(program, "Unknown flag");
            printf(" '%s'\n", arg);
            exit(1);
        }
    }
}

const char file_header[FILE_HEADER_LEN] = "/*\nThis is an auto-generated file. Produced by toaster.\n*/\n#define TOAST_IMPLEMENTATION\n#include \"toast.h\"\n\n";
const char main_decl[MAIN_DECL_LEN] = "int main() {\n  PackOfToast pack = plug_in_toaster(\"Toaster\");\n\n";
const char main_close[MAIN_CLOSE_LEN] = "\n  toast(pack);\n  unplug_toaster(pack);\n  return 0;\n}\n";

const char *gen_files[NUM_GEN_FILES] = {GEN_FILE, LOGS, EXECUTABLE};

typedef struct {
    char* file_name;
    size_t s; //function name start
    size_t l; //function name len
    char* function;
} Case;

char* case_get_fn_name(Case *item) {
    char* name = malloc(item->l+1);
    memcpy(name, item->function+item->s, item->l);
    name[item->l] = '\0';
    return name;
}

typedef struct {
    Case* items;
    size_t len;
    size_t cap;
} Cases;

typedef struct {
    char* items;
    size_t len;
    size_t cap;
} Str;

void free_cases(Cases cases) {
    free(cases.items);
}
void str_clear(Str *s) {
    s->len = 0;
}
Str str_copy(Str str) {
    Str new = {0};
    new.cap = str.cap;
    new.len = str.len;
    new.items = malloc(str.cap * sizeof(*str.items));
    memcpy(new.items, str.items, str.len);
    return new;
}

void str_trim_left_chunk(Str *s, const char* target, size_t target_len) {
    for (size_t i = 0; i < s->len; i++) {
        if (memcmp(s->items + i, target, target_len) == 0) {
            s->items = s->items + i;
            s->len -= i;
            return;
        }
    }
};
void str_trim_right(Str *s, char target) {
    for (int i = (int)s->len-1; i >= 0; i--) {
        if (s->items[i] == target) {
            s->len = i+1;
            return;
        }
    }
};

bool str_cmp_with_cstr(Str *s, char* other) {
    return memcmp(s->items, other, strlen(other)) == 0;
}

void str_free(Str s) {
    free(s.items);
}

typedef enum {
    Void,
    CurlyOpen,
    CurlyClose,
    FnName
} Token;

static const char v[4] = {'v','o','i', 'd'};
char v_tmp[4] = {'v'};

void three(FILE *file) {
    size_t i = 1;
    while ((v_tmp[i] = (char)getc(file)) != EOF) {
        i++;
        if (i==4) {
            break;
        }
    }
}

typedef enum {
    VOID,
    START_FN_NAME,
    COUNT_FN_NAME,
    END
} FileParserState;

char* read_defin(char* file_path) {
    char full_path[strlen(args.dir) + strlen(file_path)+2];
    sprintf(full_path, "%s/%s", args.dir, file_path);
    printf(LOG_PREFIX" reading %s\n", full_path);
    FILE *file = fopen(full_path, "r");
    if (file == NULL) {
        fprintf(stderr, LOG_PREFIX"[ERROR] could not open for '%s' because: %s\n", full_path, strerror(errno));
        exit(1);

    } 
    if (fseek(file, 0, SEEK_END) < 0) {
        fprintf(stderr, LOG_PREFIX"[ERROR] could not fseek for '%s' because: %s\n", full_path, strerror(errno));
        fclose(file);
        exit(1);
    }
    long len = ftell(file);
    if (len < 0) {
        fprintf(stderr, LOG_PREFIX"[ERROR] could not ftell for '%s' because: %s\n", full_path, strerror(errno));
        fclose(file);
        exit(1);
    }
    rewind(file);
    char* buf = malloc(len);
    if (fread(buf, len, 1, file) != 1) {
        fprintf(stderr, LOG_PREFIX"[ERROR] could not read '%s'\n", file_path);
        exit(1);
    }
    fclose(file);
    return buf;
}

Case parse_case(FILE *file, char* file_name, int *done) {
    
    char ch;
    Str buf = {0};
    FileParserState state = VOID;
    size_t fn_name_start = 0;
    size_t fn_name_len = 0;
    size_t braces_count = 0;
    int run = 1;
    while (run > 0) {
        
        ch = (char)getc(file);
        if (ch == EOF) {
            *done = 1;
            run = 0;
            break;
        }
        switch (state) {
            case VOID:
                {
                    if (ch == 'v') {
                        v_tmp[0] = 'v';
                        three(file);
                        if (memcmp(v_tmp, v, 4) == 0) {
                            append_many(&buf, v_tmp, 4);
                            state = START_FN_NAME;
                            break;
                        }
                    }
                }
                break;
            case START_FN_NAME:
                {
                    append_one(&buf, ch);
                    if (ch == ' ' || ch == '\n') {
                        break; 
                    } else {
                        fn_name_start = buf.len-1; 
                        state = COUNT_FN_NAME;
                        break;
                    }
                }
             case COUNT_FN_NAME:
                {
                    append_one(&buf, ch);
                    if (ch == '(') {
                        fn_name_len = (buf.len -1) - fn_name_start;
                        state = END;
                        break;
                    } 
                }
                break;
             case END:
                {
                    append_one(&buf, ch);
                    if (ch == '{') {
                        braces_count++;
                    }
                    if (ch == '}') {
                        braces_count--;
                        if (braces_count == 0) {
                            run = 0;
                        }
                    }
                }
                break;
            default:
                continue;
        }
    }
    if (run == 0) {
        append_one(&buf, '\0');
        Case item = {
            .file_name = file_name,
            .s = fn_name_start,
            .l = fn_name_len,
            .function = malloc(buf.len),
        };
        memcpy(item.function, buf.items, buf.len);
        str_free(buf);
        return item;
    } else {
        fprintf(stderr, LOG_PREFIX"[ERROR] parsing test case\n");
        exit(1);
    }
}


int parse_file(Cases *cases, char* dir_name, char* file_name) {
    char* file_path = malloc(strlen(dir_name) + strlen(file_name) + 2); //delimiter + terminator
    strcpy(file_path, dir_name);
    strcat(file_path, "/");
    strcat(file_path, file_name), 

    printf(LOG_PREFIX" parsing %s\n", file_path);

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, LOG_PREFIX"[ERROR] could not open file '%s' because: %s\n", file_path, strerror(errno));
        return 1;
    }


    size_t file_name_len = strlen(file_name) + 1;
    int done = 0;

    while (1) {
        char *current_filename = malloc(file_name_len);
        memcpy(current_filename, file_name, file_name_len);
        Case item = parse_case(file, current_filename, &done);
        if (done == 1) {
            break;
        }
        append_one(cases, item);
    }
    fclose(file);
    return 0;
}



bool is_test_file(char *file_name) {
    char ch;
    size_t i = 0;
    int check = 0;
    char *target = ".test.";
    size_t target_max_idx = 6;
    size_t target_index = 0;
    while ((ch = file_name[i]) != '\0') {
        if (check == 1) {
            if (ch == target[target_index]) {
                target_index++;
                if (target_index == target_max_idx) {
                    return true;
                }
            } else {
                check = 0;
            }
            i++;
            continue;
        }
        if (ch == '.') {
            if (check == 0) {
                check = 1;
                target_index = 1;
            } else {
                check = 0;
            }
            i++;
            continue;
        }
        i++;
    }
    return false;
}

int write_test_file(Cases *cases, FILE *file, char* defines) {
    Str data = {0};
    
    append_many(&data, file_header, FILE_HEADER_LEN);
    if (defines[0] != '\0') {
        append_many(&data, defines, strlen(defines));
        append_one(&data, '\n');
        free(defines);
    }
    for (size_t i = 0; i < cases->len; ++i) {
        append_many(&data, cases->items[i].function, strlen(cases->items[i].function));
    }
    
    append_many(&data, main_decl, MAIN_DECL_LEN-1);
    
    char identifier[128];
    for (size_t i = 0; i < cases->len; ++i) {
        append_many(&data, "  SliceOfToast ", 15);
        sprintf(identifier, "slice_%ld", i);
        append_many(&data, identifier, strlen(identifier));
        char* fn_name = case_get_fn_name(&cases->items[i]);
        append_many(&data, " = {.toast = ", 13);
        append_many(&data, 
                fn_name, 
                cases->items[i].l);
        append_many(&data, ", .name = \"", 11);
        append_many(&data, 
                fn_name, 
                cases->items[i].l);
        append_many(&data, "\"};\n  insert_toast(&pack, ", 26);
        append_many(&data, identifier, strlen(identifier));
        append_many(&data, ");\n\n", 4);
    }

    append_many(&data, main_close, MAIN_CLOSE_LEN-1);

   if (fwrite(data.items, 1, data.len, file) < data.len) {
        fprintf(stderr, LOG_PREFIX"[ERROR] writing to tmp_toast.c failed\n");
        return 1;
   };
    str_free(data);
    return 0;
}

int remove_generated_files(int skip_bin) {
    int success = 0;
    for (int i = 0; i < NUM_GEN_FILES-skip_bin; ++i) {
           if (remove(gen_files[i]) < 0) {
               fprintf(stderr, LOG_PREFIX"[ERROR] deleting '%s' file (%s)\n", gen_files[i], strerror(errno));
            } else {
                success++;
            }
    }
   if (success == NUM_GEN_FILES-skip_bin) {
       printf(LOG_PREFIX" clean-up of generated files successful\n");
       return 0;
   } else {
       printf(LOG_PREFIX" clean-up of generated files (partially) failed. %d/%d were deleted\n", success, NUM_GEN_FILES-skip_bin);
       return 1;
   }
}


int main(int argc, char **argv) {
    parse_args(argc, argv);
    struct dirent *de;
    printf(LOG_PREFIX" Reading dir '%s'.\n", args.dir);

    DIR *source_dir = opendir(args.dir);
    if (source_dir == NULL) {
        printf(LOG_PREFIX"[ERROR] Source directory %s was not found\n", args.dir);
        exit(1);
    }
    
    Cases cases = {0};
    char* defines = {0};
    while ((de = readdir(source_dir)) != NULL) {
        if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0)) {
            continue;
        }
        if (strcmp(de->d_name, DEFIN_FILE) == 0) {
            defines = read_defin(de->d_name);
        } else if (is_test_file(de->d_name)) {
            if (parse_file(&cases, args.dir, de->d_name) == 1) {
                continue;
            }           
        } 
    }
    closedir(source_dir);

    FILE *tmp_file = fopen("tmp_toast.c", "w");
    if (tmp_file == NULL) {
        fprintf(stderr, LOG_PREFIX"[ERROR] opening tmp_toast.c failed\n");
        return 1;
    }
    write_test_file(&cases, tmp_file, defines);
    free_cases(cases);
    fclose(tmp_file);
    int status;
    int log_fd = open("logs", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    //fork and run
    pid_t cpid = fork();
    if (cpid == 0) {
        if (dup2(log_fd, STDOUT_FILENO) < 0) {
            exit(1);
        }
        if (dup2(log_fd, STDERR_FILENO) < 0) {
            exit(1);
        }
        printf(LOG_PREFIX" spawning child process\n");
        printf(LOG_PREFIX" compiling test suite: 'tmp_toast.c'\n");
        char *args[] = {CC, "-o", "tmp_toast", "tmp_toast.c", NULL};
        execvp(CC, args);
    }
    
   if (cpid > 0) {
       pid_t pid = wait(&status);
       int success = WEXITSTATUS(status);
       if (WIFEXITED(status)) {
           if (success == 0) {
                printf(LOG_PREFIX" Compilation successful\n");
           } else {
                printf(LOG_PREFIX" Compilation failed. Process exited with %d\n", success);
               off_t fsize = lseek(log_fd, 0, SEEK_END);
               char log_buf[fsize+1];
               lseek(log_fd, 0, SEEK_SET);
               read(log_fd, log_buf, fsize);
               log_buf[fsize] = '\0';
               printf("%s", log_buf);
               close(log_fd);
               if (args.keep == 0) {
                    remove_generated_files(1);
               }
               return 1;
           }
       }

       //fork again
       pid = fork();

       if (pid == 0) {
            if (dup2(log_fd, STDOUT_FILENO) < 0) {
                exit(1);
            }
            if (dup2(log_fd, STDERR_FILENO) < 0) {
                exit(1);
            }
            printf(LOG_PREFIX " Running test suite\n");
            char* cmd[] = {"./tmp_toast"};
            execvp(cmd[0], cmd);
       }

       if (pid > 0) {
           wait(&status);
           off_t fsize = lseek(log_fd, 0, SEEK_END);
           char log_buf[fsize+1];
           lseek(log_fd, 0, SEEK_SET);
           read(log_fd, log_buf, fsize);
           log_buf[fsize] = '\0';
           printf("%s", log_buf);
           close(log_fd);
           if (args.keep == 0) {
               if (remove_generated_files(0) != 0) {
                   return 1;
               }
           }
       }
       return 0;
   }
   return 1;
}
