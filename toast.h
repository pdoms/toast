//MIT Licence Agreemeent can be found at the end of the file

/*
toast.h is a simple test suite, stb-style - header only library written in and
for the C programming language. 
*/

#ifndef TOAST_H_
#define TOAST_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define INITIAL_SLOTS 2 // has to be two because of standard toasters
#define ERROR_BUFFER_CAP 1024
#define YUMMY 0 //means success
#define BURNT 1 //means failure
#define RAW -1  //means unexecuted



//Type that represents a test case function;
typedef void(*Toasting)(BurntToast*);

//Struct that holds the test case function and it's metadata. Both on user 
//side and internally
typedef struct {
    //Test case function.
    Toasting toast;
    //Name of the test
    const char* name;
    //Restult identifier
    int result;
    //Time it took to run
    double time;
    //Time unit. Can either be us, ms, or s.
    int time_unit;
} SliceOfToast;

//A Test Suite, with an array of tests (slices)
typedef struct {
    //Collection of test cases
    SliceOfToast *slices;
    //Num of test cases
    size_t size;
    //Capacity it holds.
    size_t cap;
    //Name of test-suite
    const char* brand;
    //time all tests took
    double time;
    //Time unit. Can either be us, ms, or s.
    int time_unit;
} PackOfToast;

//This struct is passed to each test case function, provided is only the 
//[index] of the current test case. All other fields can be set within a 
//test case function. Short-cut functions for either success or failure are
//provided -> `burn_toast` or `eat_toast` below.
typedef struct {
    size_t index;
    /* use this to set the result, to either YUMMY (0) for success, or BURNT (1)
     for a failed test. */
    int yummy_or_burnt;
    /*Set this to hold a message to be printed in the diagnostics section for
     each failed test case.
     */
    char *diagnostic;
    /*Since the struct is reused over each test case, and reset in-between,
     it is just easier to set print_diagnostic to tell the suite that it needs
     to print the message. */
    int print_diagnostic;
} BurntToast;


//Initializer function for a test case.
SliceOfToast pre_bake_toast(const char* name, Toasting toast);

//Initializer functoin for the test suite
PackOfToast plug_in_toaster(const char* brand);

//Insert array of test cases into test suites
void insert_toasts(PackOfToast *pack, SliceOfToast *slices, size_t len);
//Insert singe test case into test suites
void insert_toast(PackOfToast *pack, SliceOfToast slice);

//Run the test suite
int toast(PackOfToast pack);
//Clean/free memory
void unplug_toaster(PackOfToast pack);

//Helper function to populate a the test case functoin argument with a negative 
//result
void burn_toast(BurntToast *burnt, char* diagnostic);
//Helper function to populate a the test case functoin argument with a positive 
//result
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


/* Copyright 2025 Paulo Doms
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
 */

