# toast.h

A small stb-style/header-only library for testing *c* code.
This is still a work-in-progress, but fully usable for small projects, as every
test file needs to be compiled. Next steps would include to implement more files for tidier project structure. But is not needed as of now.

## Example
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

## Toast Basic Terminology

- `SliceOfToast` - synonymous with a test case.
- `PackOfToast` - synonymous with a test suite.
- `Toasting` - the executor for a test case or simply a _test-case-function_
- `YUMMY` - a success or as _int_ `0`
- `BURNT` - a failure or as _int_ `1`
- `RAW` - not run yet _int_ `-1`
- `BurnToast` - Metadata/Result to be set in side the a `Toast`

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
