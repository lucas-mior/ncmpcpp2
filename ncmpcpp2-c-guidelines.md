# C guidelines specific to ncmpcpp2
Use this as an addendum to cbase/c-guidelines.md

## Declaration with initialization
For StrBuilder on the stack, prefer to initialize them to zero at the
declaration instead of postponing it:
```c
// good
static void
function(void) {
    StrBuilder var = {0};
    
    // other stuff ...
}

// bad
static void
function(void) {
    StrBuilder var;

    // other stuff ...

    var = (StrBuilder){0};
}
```
