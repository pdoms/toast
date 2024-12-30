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
