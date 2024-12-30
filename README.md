# toast.h

A small stb-style/header-only library for testing *c* code.
This is still a work-in-progress, but fully usable.

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
Or just use the corresponding `int`. [BurntToast.yummy\_or\_burnt](###BurntToast) always
defaults to [RAW](### RAW).

#### YUMMY
`YUMMY` - a successful test case as _int_ `0`

#### BURNT
`BURNT` - a failed test case as _int_ `1`

#### RAW
`RAW` - a yet to run or not run state _int_ `-1`

### 2. Structs

### SliceOfToast
### PackOfToast
### BurntToast

### Functions

### Toasting
### pre\_bake\_toast
### plug\_in\_toaster
### insert\_toasts
### insert\_toast
### toast
### unplug\_toaster
### burn\_toast
### eat\_toast
