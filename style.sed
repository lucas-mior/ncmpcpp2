#!/usr/bin/env -S sed -E -f

s/for \(;;\)/while (true)/g;
s/while \(1\)/while (true)/g;
s/\<memset\>/memset64/g;
s/\<memcpy\>/memcpy64/g;
s/\<memmove\>/memmove64/g;
s/\<memmem\>/memmem64/g;
s/\<memchr\>/memchr64/g;
s/\<memcmp\>/memcmp64/g;
s/\<strlen\>/strlen32/g;
s/\<strncmp\>/strncmp32/g;
s/\<strncpy\>/strncpy32/g;
s/\<fread\>/fread64/g;
s/\<fwrite\>/fwrite64/g;
s/\<qsort\>/qsort64/g;

s/([^"])\<unsigned ([a-z]+)/\1u\2/g;

s/([^"])\<int8_t\>([^.])/\1int8\2/g;
s/([^"])\<int16_t\>([^.])/\1int16\2/g;
s/([^"])\<int32_t\>([^.])/\1int32\2/g;
s/([^"])\<int64_t\>([^.])/\1int64\2/g;
s/([^"])\<uint8_t\>([^.])/\1uint8\2/g;
s/([^"])\<uint16_t\>([^.])/\1uint16\2/g;
s/([^"])\<uint32_t\>([^.])/\1uint32\2/g;
s/([^"])\<uint64_t\>([^.])/\1uint64\2/g;

s/([^"])\<uchar\>([^.])/\1uint8\2/g;
s/([^"])\<ushort\>([^.])/\1uint16\2/g;
s/([^"])\<short\>([^.])/\1int16\2/g;
s/([^"])\<uint\>([^.])/\1uint32\2/g;
s/([^"])\<int\>([^.])/\1int32\2/g;
s/([^"])\<ulong\>([^.])/\1uint64\2/g;
s/([^"])\<long\>([^.])/\1int64\2/g;

s/([^"(])\<size_t\>([^")])/\1int64\2/g;
s/([^"])\<ssize_t\>/\1int64/g;
s/([^"])\<ptrdiff_t\>/\1int64/g;

s/too int64/too long/g;
s/too int16/too short/g;

s/(\S+) \* (\S+)/\1*\2/g;
