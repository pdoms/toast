#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define LOG_PREFIX  "[TOASTER] "
#define DEFAULT_CASES_CAP 1024
#define DEFAULT_STR_CAP 1024

#define CC "gcc"

typedef struct {
    char* file_name;
    char* function_name;
    char* function;
} Case;

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

void append_str(Str *str, char* data) {
    if (str->items == NULL) {
        str->items = malloc(DEFAULT_STR_CAP);
        if (str->items == NULL) {
            fprintf(stderr, LOG_PREFIX"[ERROR] allocating str\n");
            exit(1);
        }
        str->len = 0;
        str->cap = DEFAULT_STR_CAP;
    }

    size_t data_len = strlen(data);

    if (str->len + data_len > str->cap) {
        while ((str->len + data_len) > str->cap) {
                str->cap *= 2;
        }
        str->items = realloc(str->items, str->cap);
        if (str->items == NULL) {
            fprintf(stderr, LOG_PREFIX"[ERROR] reallocating str\n");
            exit(1);
        }

    }
    memcpy(str->items + str->len, data, data_len);
    str->len += data_len;
}


void append_case(Cases *cases, Case item) {
    if (cases->items == NULL) {
        //init
        cases->items = malloc(sizeof(Case)*DEFAULT_CASES_CAP);
        if (cases->items == NULL) {
            fprintf(stderr, LOG_PREFIX"[ERROR] allocating\n");
            exit(1);
        }
        cases->len = 0;
        cases->cap = DEFAULT_CASES_CAP;
    }
    if (cases->len > cases->cap/2) {
        //realloc 
        size_t new_cap = cases->cap*2;
        cases->items = realloc(cases->items, new_cap);
        if (cases->items == NULL) {
            fprintf(stderr, LOG_PREFIX"[ERROR] reallocating\n");
            exit(1);
        }
    }
    //append
    cases->items[cases->len] = item;
    cases->len++;
    return;
}

typedef enum {
    Void,
    CurlyOpen,
    CurlyClose,
    FnName
} Token;

void trim_function_buf(char* buf, size_t *start, size_t *buf_idx) {
    size_t i=0;
    char trgt[] = {'v', 'o', 'i', 'd'};
    size_t window_len = 4;
    
    while (i < *buf_idx) {
        if (memcmp(buf + i, trgt, window_len) == 0) {
            *start = i;
            break;
        }
        i++;
    }
    i = *buf_idx-1;
    while (i >= 0) {
        if (buf[i] == '}') {
            *buf_idx = i+1;
            break;
        }
        i--;
    }
}

bool is_void(char* probe) {
    return strcmp(probe, "void") == 0 || strcmp(probe, "void*") == 0;
}

Token to_next_known_token(FILE *file, char* buf, size_t *buf_idx, char* token, int is_fn_name, int *eof) {
    int size = 0;
    char ch;
    while ((ch = (char)getc(file)) != EOF) {

        buf[*buf_idx] = ch;
        *buf_idx += 1;

        if (is_fn_name) {
            if (ch == '(') {
                token[size] = '\0';
                return FnName;
            } else {
                token[size] = ch;
                size++;
                continue;
            }
        } else {
            if (ch == '{') {
                return CurlyOpen;
            } else if (ch == '}') {
                return CurlyClose;
            } else if (ch == ' ' || ch == '\n') {
                if (size > 0) {
                    token[size] = '\0';
                    if (is_void(token)) {
                        return Void;
                    } 
                }
                size = 0;
            } else {
                    token[size] = ch;
                    size++;
            }
        }
    }
    *eof = 1;
}

int parse_file(Cases *cases, char* dir_name, char* file_name) {
    char* file_path = malloc(strlen(dir_name) + strlen(file_name) + 2); //delimiter + terminator
    strcpy(file_path, dir_name);
    strcat(file_path, "/");
    strcat(file_path, file_name), 

    printf(LOG_PREFIX"parsing %s\n", file_path);

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, LOG_PREFIX"[ERROR] could not open file '%s'\n", file_path);
        return 0;
    }

    if (fseek(file, 0, SEEK_END) < 0) {
        fprintf(stderr, LOG_PREFIX"[ERROR] could not fseek for '%s'\n", file_path);
        fclose(file);
        return 0;
    }

    size_t len = ftell(file);
    rewind(file);

    size_t file_name_len = strlen(file_name) + 1;

    int braces = 0;
    int should_stop = 0;
    char *token = (char*)malloc(1024);
    char *fn_name = (char*)malloc(1024);
    int is_fn_name = 0;
    char *buf = (char*)calloc(len, 1);
    size_t buf_idx=0;
    
    while (1) {
        Token tkn = to_next_known_token(file, buf, &buf_idx, token, is_fn_name, &should_stop);
        if (should_stop == 1) {
            break;
        }
        switch (tkn) {
            case Void:
                is_fn_name = 1;
                break;
            case CurlyOpen:
                braces++;
                break;
            case CurlyClose:
                braces--;
                if (braces == 0) {
                    Case* item = malloc(sizeof(Case));
                    
                    item->file_name = malloc(file_name_len);
                    memcpy(item->file_name, file_name, file_name_len);
                    size_t fn_name_len = strlen(fn_name)+1;
                    item->function_name = malloc(fn_name_len);
                    memcpy(item->function_name, fn_name, fn_name_len);
                    size_t buf_start = 0;
                    trim_function_buf(buf, &buf_start, &buf_idx);
                    buf[buf_idx] = '\n';
                    buf_idx++;
                    buf[buf_idx] = '\n';
                    item->function = malloc(buf_idx);
                    memcpy(item->function, buf+buf_start, buf_idx-buf_start);
                    append_case(cases, *item);
                    buf_idx = 0;
                }
                break;
            case FnName:
                strcpy(fn_name, token);
                is_fn_name = 0;
                break;
            default:
                fprintf(stderr, LOG_PREFIX"[ERROR] unknown token\n");
                exit(1);
        }
    }
    fclose(file);
}






char **shift_arg(int *argc, char **argv) {
    if (argc > 0) {
        argv = argv + 1;
        *argc -= 1;
        return argv;
    } else {
        return NULL;
    }
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

int write_test_file(Cases *cases, FILE *file) {
    Str *data = malloc(sizeof(Str));
    data->items = NULL;
    append_str(data, "#define TOAST_IMPLEMENTATION\n#include \"toast.h\"\n\n");
    for (size_t i = 0; i < cases->len; ++i) {
        append_str(data, cases->items[i].function);
    }
    
    append_str(data, "int main() {\n  PackOfToast pack = plug_in_toaster(\"Toaster\");\n\n");
    
    char identifier[128];
    char* slice = "slice_";

    for (size_t i = 0; i < cases->len; ++i) {
        append_str(data, "  SliceOfToast ");
        strcpy(identifier, slice);
        sprintf(identifier + 6, "%ld", i);
        append_str(data, identifier);
        append_str(data, "= {.toast = ");
        append_str(data, cases->items[i].function_name);
        append_str(data, ", .name = \"");
        append_str(data, cases->items[i].function_name);
        append_str(data, "\"};\n  insert_toast(&pack, ");
        append_str(data, identifier);
        append_str(data, ");\n\n");
    }

    append_str(data, "\n  toast(pack);\n  unplug_toaster(pack);\n  return 0;\n}\n");


    if (fwrite(data->items, 1, data->len, file) < data->len) {
        fprintf(stderr, LOG_PREFIX"[ERROR] writing to tmp_toast.src failed\n");
        exit(1);
    };
}

int main(int argc, char **argv) {
    char* program = argv[0];
    argv = shift_arg(&argc, argv);
    char* dir;
    struct dirent *de;
    if (argc == 0) {
        dir = "./tests";
    } else {
        dir = argv[0]; 
    }
    printf(LOG_PREFIX"Reading dir '%s'.\n", dir);

    DIR *source_dir = opendir(dir);
    if (source_dir == NULL) {
        printf("Source director %s was not found\n");
        exit(1);
    }
    
    Cases *cases = malloc(sizeof(Cases));
    cases->items = NULL;

    while ((de = readdir(source_dir)) != NULL) {
        if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0)) {
            continue;
        }

        if (is_test_file(de->d_name)) {

            if (parse_file(cases, dir, de->d_name) == 0) {
                continue;
            }           
            
        } 
    }
    closedir(source_dir);
    FILE *tmp_file = fopen("tmp_toast.c", "w");
    if (tmp_file == NULL) {
        fprintf(stderr, LOG_PREFIX"[ERROR] opening tmp_toast.src failed\n");
        exit(1);
    }

    write_test_file(cases, tmp_file);

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
    
        printf(LOG_PREFIX"spawning child process\n");
        printf(LOG_PREFIX"compiling test suite: 'tmp_toast.c'\n");
       char *args[] = {CC, "-o", "tmp_toast", "tmp_toast.c", NULL};
       execvp(CC, args);
    }
    
   if (cpid > 0) {
       pid_t pid = wait(&status);
       int success = WEXITSTATUS(status);
       if (WIFEXITED(status)) {
           if (success == 0) {
                printf(LOG_PREFIX"Compilation successful\n");
           } else {
                printf(LOG_PREFIX"Compilation failed. Process exited with %d\n", success);
           off_t fsize = lseek(log_fd, 0, SEEK_END);
           char log_buf[fsize+1];
           lseek(log_fd, 0, SEEK_SET);
           read(log_fd, log_buf, fsize);
           log_buf[fsize] = '\0';
           printf("%s", log_buf);
           close(log_fd);

           if (remove("logs") < 0) {
               fprintf(stderr, LOG_PREFIX"[ERROR] deleting 'logs' file (%s)\n", strerror(errno));
               return 1;
           }
           if (remove("tmp_toast.c") < 0) {
               fprintf(stderr, LOG_PREFIX"[ERROR] deleting 'tmp_toast.c' file (%s)\n", strerror(errno));
               return 1;
           }
           if (remove("tmp_toast") < 0) {
               fprintf(stderr, LOG_PREFIX"[ERROR] deleting 'tmp_toast' binary (%s)\n", strerror(errno));
               return 1;
           }
           printf(LOG_PREFIX"Clean up successful\n");
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
            printf(LOG_PREFIX"running test suite\n");
            char* cmd[] = {"./tmp_toast"};
            execvp(cmd[0], cmd);
       }

       if (pid > 0) {
           pid_t pid = wait(&status);
            int success = WEXITSTATUS(status);
           off_t fsize = lseek(log_fd, 0, SEEK_END);
           char log_buf[fsize+1];
           lseek(log_fd, 0, SEEK_SET);
           read(log_fd, log_buf, fsize);
           log_buf[fsize] = '\0';
           printf("%s", log_buf);
           close(log_fd);

           if (remove("logs") < 0) {
               fprintf(stderr, LOG_PREFIX"[ERROR] deleting 'logs' file (%s)\n", strerror(errno));
               return 1;
           }
           if (remove("tmp_toast.c") < 0) {
               fprintf(stderr, LOG_PREFIX"[ERROR] deleting 'tmp_toast.c' file (%s)\n", strerror(errno));
               return 1;
           }
           if (remove("tmp_toast") < 0) {
               fprintf(stderr, LOG_PREFIX"[ERROR] deleting 'tmp_toast' binary (%s)\n", strerror(errno));
               return 1;
           }
           printf(LOG_PREFIX"Clean up successful\n");
       }
    }
    return 0;
}
