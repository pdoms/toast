# Toaster v1.0.0

A one part small stb-style/header-only library for testing *c* code and a *.c* file to be compiled which parsed, compiles, and runs test files.
This is still a work-in-progress, thus only usable on linux/posix machines.

## toast.h
### Example
```c
//file test.c
#define TOAST_IMPLEMENTATION
#include "toast.h"

void add(BurntToast *burnt) { 
    if ((1 + 2) == 3) {
        eat_toast(burnt);
        return;
    }  
    burn_toast(burnt, "Adding went wrong");
    return; 
}

void sub(BurntToast *burnt) { 
    if ((4 - 2) == 1) {
        eat_toast(burnt);
        return;
    }  
    burn_toast(burnt, "Substracting went wrong");
    return; 
}

void mul(BurntToast *burnt) { 
    if ((2 * 2) == 4) {
        eat_toast(burnt);
        return;
    }
    burn_toast(burnt, "Multiplying went wrong");
    return; 
}


int main() {
    //initialize test suite
    PackOfToast pack = plug_in_toaster("toast.h");
    
    //prepare an array of tests
    SliceOfToast toasts[2] = {
        (SliceOfToast){
        .toast = add,
        .name = "add",
        },
        (SliceOfToast){
        .toast = sub,
        .name = "sub",
        },
    };
    
    //and insert them as an array 
    insert_toasts(&pack, toasts, 2);

    SliceOfToast slice = {
        .toast = mul,
        .name = "mul",
    };
    //or just insert one
    insert_toast(&pack, slice);

    //run the test suite
    toast(pack);

    //clean up
    unplug_toaster(pack);
    return 0;
}
```

### Toast Basic Terminology

- `SliceOfToast` - synonymous with a test case.
- `PackOfToast` - synonymous with a test suite.
- `Toasting` - the executor for a test case or simply a _test-case-function_
- `YUMMY` - a success or as _int_ `0`
- `BURNT` - a failure or as _int_ `1`
- `RAW` - not run yet _int_ `-1`
- `BurnToast` - Metadata/Result to be set in side the a `Toast`


## toaster.c

### build
```console
$ make 
```
Clean up can be performed with `$ make clean`.

### test setup
1. As of now test files need to be located in a _source directory_, which defaults to `./tests`. 
2. Testfiles should be named like so `*.test.c`, e.g. `foo.test.c` (as a path relative to the executable it would be `./tests/foo.test.c`
3. Testfiles contain a collection of `SliceOfToast`s. In the form of
```c
typedef void(*Toasting)(BurntToast*);
```
for example:
```c 
void add(BurntToast *burnt) { 
    if ((1 + 2) == 3) {
        eat_toast(burnt);
        return;
    }  
    burn_toast(burnt, "Adding went wrong");
    return; 
}
```
*NOTE:* no header files for `toast` need to be included nor a `main()` function is required as these cases get parsed and written to an actual .c file 
4. Create a `defin.test.c` file in the same directory. 
Here you can put all your `#define`s and `#include`s, which will placed *after* the stb-style `#define`s and `#include`s of `toast.h`

5. Run the test like so:
```console
./toaster [OPTIONS]
-d|--dir <dir>.......... specifies the src directory. [Default: './tests']
-k|--keep .............. toaster won't remove the files it generated
-v|--version ........... print the current version of this toaster
-h|--help .............. print this very text
```

### Run the example
From the root of the project:
1.  `$ cd ./examples`
2.  `$ make harness`

The exectution is triggered by the `harness` recipe. Comment the last line out if you want to run it with any arguments.


## Documentation

### 1. Constants/Macros

Use these to set the state/result of the outcome of a single test case.
Or just use the corresponding `int`. `BurntToast.yummy\_or\_burnt` always
defaults to `RAW`.

#### YUMMY
`YUMMY` - a successful test case as _int_ `0`

#### BURNT
`BURNT` - a failed test case as _int_ `1`

#### RAW
`RAW` - a yet to run or not run state _int_ `-1`

### 2. Structs


### SliceOfToast

This is basically a test case.

| Field      | Type          | Domain       | Description                                                                            |
|------------|---------------|--------------| ---------------------------------------------------------------------------------------|
| toast      | `Toasting`    | user-defined | The test case function                                                                 |
| name       | `const char*` | user-defined | The name of the test-case. Will be printed to stdout.                                  |
| result     | `int`         | user-defined | Internally set to RAWor set to the result of each test case in the test case function. | 
| time       | `double`      | internal     | The time a test-case took to finish.                                                   |
| time\_unit | `int`         | internal     | The unit to interpret the time|

### PackOfToast

This is basically the test-suite.

| Field      | Type           | Domain       | Description                                                                           |
|------------|----------------|--------------| --------------------------------------------------------------------------------------|
| slices     | `SliceOfToast*`| user-defined | Array of test cases.                                                                  |
| brand      | `const char*`  | user-defined | The name of the set of test-cases                                                     |
| size       | `size_t`       | internal     | The size of `slices`                                                                  |
| cap        | `size_t`       | internal     | Current capacity of `slices`                                                          |
| time       | `double`       | internal     | The time all test-cases took to finish.                                               |
| time\_unit | `int`          | internal     | The unit to interpre t the time                                                       |

### BurntToast

This is struct is passed a reference to each test function as an argument in order to to set diagnostics and result identifiers.

| Field             | Type   | Domain                 | Description                                                                          |
|-------------------|----------------|------------------------|------------------------------------------------------------------------------|
| index             | `int`   | toast-defined         | The index the test-case has inside `PackOfToast`.                                                                                                       |
| yummy\_or\_burnt  | `int`   | internal/user-defined | The actual result, initialized to `Raw`, should be set inside the test-case-function |
| diagnostic        | `char*` | user-defined          | The message to be printed in case of an error                                        |
| print\_diagnostic | `int` | user-defined            | Whether or not to print the message. This is required internally.                    |

### Functions

### Toasting

A type definition for the test-case-function.
```c
typedef void(*Toasting)(BurntToast*);
```

### pre\_bake\_toast

Initializer function for a test-case/`SliceOfToast`.
```c
SliceOfToast pre_bake_toast(const char* name, Toasting toast);
```

### plug\_in\_toaster

Initializer function for a test-suite/`PackOfToast`.
```c
PackOfToast plug_in_toaster(const char* brand);
```

### insert\_toasts

Insert an array of test-cases/`SliceOfToast`s.
```c
void insert_toasts(PackOfToast *pack, SliceOfToast *slices, size_t len);
```

### insert\_toast

Insert a single test-cases/`SliceOfToast`.
```c
void insert_toasts(PackOfToast *pack, SliceOfToast *slices, size_t len);
```

### toast

This function actually runs a `PackOfToast`.
```c
int toast(PackOfToast pack);
```

### burn\_toast

Short-cut helper function to set a `BurntToast`, i.e. a result of a test case.
It always marks the test case as failed and alaways sets `print_diagnostic` to `1` as well as assigning the provided diagnostic string.
```c
void burn_toast(BurntToast *burnt, char* diagnostic);
```

### eat\_toast

Short-cut helper function to set a `BurntToast`, i.e. a result of a test case. 
It simply sets the `yummy_or_burnt` to `YUMMY`.
```c
void eat_toast(BurntToast *burnt);
```

### unplug\_toater
Frees allocated memory in the `PackOfToast`
```c
void unplug_toaster(PackOfToast pack);
```
