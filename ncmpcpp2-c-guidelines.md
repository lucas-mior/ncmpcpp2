# Extra C guidelines specific for this project
Use this as addendum to cbase/c-guidelines.md

For structs on the stack, initialize at declaration:
```c
// bad
void
function(void *arg) {
    MyStruct s;

    s = (MyStruct){0};
}

// good
void
function(void *arg) {
    MyStruct s = {0};
}

// good
void
function(void *arg) {
    MyStruct s = init_function(arg);
}
```
