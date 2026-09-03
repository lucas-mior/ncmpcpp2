# Prompt

For the first of the problems below (excessive NULL checking),
identify instances of it in the first file in the list below it. Then fix those
instances and remove the file from the list.

## Excessive NULL checking and error checking
The source code in src/ is at paranoia levels of NULL pointer checking.  For
instance, lots of functions check if (screen == NULL), but they are called
internally, *after* the external API of its module has already validated input.
Another bad pattern comes in the form of errors. Functions that should never
return negative because the input was already validated are always checked,
which adds lots of unnecessary `if (status < 0)` in the code. As explained in
detail in cbase/c-guidelines.md, only the external API of each module does input
validation. If a utility function like `sb_set` might return negative because we
passed NULL pointers to it, does not mean that we need to check it, because the
pointers should have been validated at a higher level in the code. Also, some
functions are made to return int32 only to conform because some lower level
function that they call returns int32. But again, this is only needed if an
error is expected. Most internal functions should never fail. Make them return
void in that case (or the result if they need to return some data). Also, some
functions need to checked for errors in *some* calls, while not in others. You
need to know which calls have to be checked by the context. It is okay to assume
stuff sometimes (add assertions if you are not 100% sure).

Identify instances of the anti-patterns above (excessive NULL pointer checking
and excessive error checking).  Remove all the instances of this anti-pattern.
You can leave an ASSERT(pointer != NULL) or in 20% of the cases, just as an
occasional sanity check.  But most internal functions can simply assume that
they have valid input because the external API of the module has already
validated it.

## Functions that are never called (dead code)
## Excessive error checking
## Utility function creep
Functions that do the same thing are redefined in different

## some unnecessary static function declarations at the top of the files
## functions definitions could be reordered to not need declarations at the top

## Style: lines broken prematurely

## Style: checking return value after the call (call should be inside if().

## Style: breaking function calls before the first argument and not alining
This is bad:
```c
status = ncm_fs_rename(
    old_real_path.data, old_real_path.len,
    new_real_path.data, new_real_path.len, ncm_error);
```
Replace with:
```c
status = ncm_fs_rename(old_real_path.data, old_real_path.len,
                       new_real_path.data, new_real_path.len,
                       ncm_error);
```

## Strings unecessary conversion to and from StrBuilder
Investigate:
- ncm_conversion_copy_source()
- there are multiple confusing string types
