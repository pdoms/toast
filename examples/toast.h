#ifndef TOAST_H_
#define TOAST_H_
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define INITIAL_SLOTS 2 // has to be two because of standard toasters
#define ERROR_BUFFER_CAP 1024
#define YUMMY 0
#define BURNT 1
#define RAW -1

typedef struct {
    size_t index;
    int yummy_or_burnt;
    char *diagnostic;
    int print_diagnostic;
} BurntToast;

typedef void(*Toasting)(BurntToast*);
typedef struct {
    Toasting toast;
    const char* name;
    int result;
    double time;
    int time_unit;
} SliceOfToast;

typedef struct {
    SliceOfToast *slices;
    size_t size;
    size_t cap;
    const char* brand;
    double time;
    int time_unit;
} PackOfToast;

SliceOfToast pre_bake_toast(const char* name, Toasting toast);

PackOfToast plug_in_toaster(const char* brand);

void insert_toasts(PackOfToast *pack, SliceOfToast *slices, size_t len);
void insert_toast(PackOfToast *pack, SliceOfToast slice);

int toast(PackOfToast pack);
void unplug_toaster(PackOfToast pack);

void burn_toast(BurntToast *burnt, char* diagnostic);
void eat_toast(BurntToast *burnt);

#endif //TOAST_H_
       
#ifdef TOAST_IMPLEMENTATION

#define ESC "\x1B["
#define RES "\x1B[0m"
#define BOLD "1"

#define ERROR "196"
#define SUCCESS "34" 
#define INFO "220"
#define LIGHT_RED = "9"
#define CLR "\x1B[38;5"

struct timeval get_time_stamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv;
}

/*
 * Returns 0 if result should be represented as micorseconds (us), 
 * 1 if represented as milliseconds (ms)
 * 2 if represend as seconds
 * */
double delta_time(struct timeval start, struct timeval end, int *unit) {
    long delta, seconds, useconds;    
    seconds  = end.tv_sec  - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    delta = (seconds*1000*1000) + useconds; //in microseconds
    if (delta > 1000 && delta < 1000000) {
        *unit = 1;
        return (double)delta / 1000.0;
    } else if (delta >= 1000000) {
        *unit = 2;
        return (double)delta / (1000.0*1000.0);
    }
    *unit = 0;
    return (double)delta;
}


void report_error(char* msg) {
    fprintf(stderr, "[TOAST]["ESC"31mERROR"RES"] %s\n", msg);
}

SliceOfToast pre_bake_toast(const char* name, Toasting toast) {
    return (SliceOfToast){
        .toast = toast,
        .name = name,
        .result = -1,
    };
}

PackOfToast plug_in_toaster(const char* brand) {
    return (PackOfToast){
        .slices = malloc(sizeof(SliceOfToast)*INITIAL_SLOTS),
        .size = 0,
        .cap = INITIAL_SLOTS,
        .brand =  brand
    };
}

void insert_toasts(PackOfToast *pack, SliceOfToast *slices, size_t len) {
    
    if ((pack->size + len) > pack->cap) {
        size_t new_cap = pack->cap*pack->cap;
        while (new_cap < len) {
            new_cap = new_cap*new_cap; 
        }
        pack->slices = realloc(pack->slices, sizeof(SliceOfToast)*new_cap);
        pack->cap = new_cap;
        if (pack->slices == NULL) {
            report_error(strerror(errno));
            exit(1);
        }
    }

    for (size_t i = 0; i < len; ++i) {
        pack->slices[pack->size] = slices[i];
        pack->size += 1;
    }
    return;
}

void insert_toast(PackOfToast *pack, SliceOfToast slice) {
    if (pack->size >= pack->cap) {
        size_t new_cap = pack->cap*pack->cap;
        pack->slices = realloc(pack->slices, sizeof(SliceOfToast)*new_cap);
        pack->cap = new_cap;
    }

    pack->slices[pack->size] = slice;
    pack->size += 1;
    return;
}

void print_stats(PackOfToast *pack) {
    char name[14];
    int success = 0;
    int failed = 0;
    int not_run = 0;
    double tests_total = 0.0;

    printf("\n  ++ "ESC"1mOverview"RES"\n\n");     
    printf("           | Test Id | Test Name     | Outcome | Time       | T Unit |\n");
    printf("           | ======= | ============= | ======= | ========== | ====== |\n");
    

    for (size_t i = 0; i < pack->size; ++i) {
        SliceOfToast slice = pack->slices[i];
        if (slice.result == 1) {
            failed += 1;
        } else if (slice.result == 0) {
            success += 1;
        } else {
            not_run += 1;
        }
        size_t j = 0;
        while (j < 14) {
            name[j] = slice.name[j];
            if (name[j] == '\0') {
                break;
            }
            j++;
        }
        char unit[3] = "us";
        double added_time = slice.time / 1000;
        if (slice.time_unit == 1) {
            unit[0] = 'm';
            added_time = slice.time;
        } else if (slice.time_unit == 2) {
            unit[0] = 's'; unit[1] = ' ';
            added_time = slice.time*1000;
        }
        tests_total += added_time;

        printf("           | %-8ld| %-14s| %-8s| %-11.4f| %-7s|\n", i+1, name, slice.result == 0 ? "pass" : "fail", slice.time, unit);
        printf("           | ------- | ------------- | ------- | ---------- | ------ |\n");

    }

    
    char unit[3] = "us";
    if (pack->time_unit == 1) {
        unit[0] = 'm';
    } else if (pack->time_unit == 2) {
        unit[0] = 's';
        unit[1] = ' ';
    }
    printf("     Total Time:       %.4f%s\n", pack->time, unit);
    printf("     Avg. Time/Test:   %.4fms\n", tests_total/pack->size);
    printf("     "CLR";"SUCCESS"mSuccess:          %d"RES"\n", success);

    printf("     "CLR";"ERROR"mFailed:           %d"RES"\n", failed);
    if (not_run > 0) {
        printf("     "CLR";"INFO"mNot Run:          %d"RES"\n", not_run);
}
    printf("\n");
    return;
};

void reset_burnt(BurntToast *burnt, int index) {
   burnt->index = index;
   burnt->yummy_or_burnt = RAW;
   burnt->diagnostic = " ";
   burnt->print_diagnostic = 0;
}

int toast(PackOfToast pack) {
    struct timeval suite_start = get_time_stamp();

    printf("\n\n +++ "ESC"1mTOASTER BRAND: %s"RES" +++\n", pack.brand);     
    printf("     Inserted %ld toasts\n\n", pack.size);

    BurntToast *burnt = malloc(sizeof(BurntToast));
    burnt->diagnostic = malloc(ERROR_BUFFER_CAP);
    reset_burnt(burnt, -1);

    for (size_t i = 0; i < pack.size; ++i) {
        burnt->index = i;
        struct timeval test_start = get_time_stamp();
        SliceOfToast *slice = &pack.slices[i];
        printf("  %ld) %s\n", i+1, slice->name);

        //run test function
        slice->toast(burnt);
        slice->result = burnt->yummy_or_burnt;
        if (slice->result > 0) {
            printf("    "CLR";"ERROR"m >> fail"RES"\n");
            if (burnt->print_diagnostic) {
                printf("        Diagnostic: %s\n\n", burnt->diagnostic);
            } else {
                printf("\n");
            }
        } else {
            printf("    "CLR";"SUCCESS"m >> success"RES"\n\n");
        }
        reset_burnt(burnt, -1);
        struct timeval test_end = get_time_stamp();
        slice->time = delta_time(test_start, test_end, &slice->time_unit); 
    }
    struct timeval suite_end = get_time_stamp();
    pack.time = delta_time(suite_start, suite_end, &pack.time_unit);
    print_stats(&pack);
    printf(" --- Toasts are done ---\n\n");
    free(burnt);
    return 0;
}

void burn_toast(BurntToast *burnt, char* diagnostic) {
    burnt->yummy_or_burnt = BURNT;
    burnt->diagnostic = diagnostic;
    burnt->print_diagnostic = 1;
}
void eat_toast(BurntToast *burnt) {
    burnt->yummy_or_burnt = YUMMY;
}

void unplug_toaster(PackOfToast pack) {
    free(pack.slices);
}


#endif //TOAST_IMPLEMENTATION
