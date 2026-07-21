#if !defined(NCMPCPP_TESTS_NCM_BUFFER_COMPAT_C)
#define NCMPCPP_TESTS_NCM_BUFFER_COMPAT_C

#include "c/ncm_base.c"

static void
ncm_buffer_compat_test_alias_and_direct_access(void) {
    StrBuilder buffer;
    StrBuilder *str_builder = &buffer;

    ncm_buffer_init(&buffer);
    ncm_buffer_append(&buffer, STRLIT_ARGS("alias"));

    ASSERT(str_builder == &buffer);
    ASSERT_EQUAL(str_builder->data, "alias");
    ASSERT_EQUAL(str_builder->len, STRLIT_LEN("alias"));
    ASSERT_EQUAL(str_builder->cap, STR_BUILDER_INITIAL_CAPACITY);

    buffer.data[0] = 'A';
    ASSERT_EQUAL(str_builder->data, "Alias");

    ncm_buffer_destroy(&buffer);
    return;
}

static void
ncm_buffer_compat_test_wrappers(void) {
    StrBuilder source;
    StrBuilder dest;
    char bytes[] = {'x', '\0', 'y'};
    char *allocation;
    char *stolen;
    int32 capacity;
    int32 len;

    ncm_buffer_init(&source);
    ncm_buffer_init(&dest);

    ncm_buffer_append(&source, bytes, LENGTH(bytes));
    ncm_buffer_append_byte(&source, '!');
    ASSERT_EQUAL(source.len, LENGTH(bytes) + 1);
    ASSERT_EQUAL(source.data[source.len], '\0');

    ASSERT(ncm_buffer_copy(&dest, &source));
    ASSERT(dest.data != source.data);
    ASSERT_EQUAL(dest.len, source.len);
    ASSERT(memcmp64(dest.data, source.data, source.len) == 0);

    allocation = dest.data;
    capacity = dest.cap;
    ncm_buffer_clear(&dest);
    ASSERT(dest.data == allocation);
    ASSERT_EQUAL(dest.len, 0);
    ASSERT_EQUAL(dest.cap, capacity);

    ASSERT(ncm_buffer_set(&dest, STRLIT_ARGS("abcdef")));
    ASSERT(ncm_buffer_set(&dest, dest.data, 3));
    ASSERT_EQUAL(dest.data, "abc");

    ncm_buffer_reserve(&dest, 64);
    ASSERT(dest.cap >= dest.len + 65);

    allocation = source.data;
    capacity = source.cap;
    ncm_buffer_move(&dest, &source);
    ASSERT(dest.data == allocation);
    ASSERT_EQUAL(dest.cap, capacity);
    ASSERT(source.data == NULL);
    ASSERT_EQUAL(source.len, 0);
    ASSERT_EQUAL(source.cap, 0);

    stolen = ncm_buffer_steal(&dest, &len, &capacity);
    ASSERT(stolen == allocation);
    ASSERT_EQUAL(len, LENGTH(bytes) + 1);
    ASSERT_EQUAL(stolen[len], '\0');
    ASSERT(dest.data == NULL);
    ASSERT_EQUAL(dest.len, 0);
    ASSERT_EQUAL(dest.cap, 0);

    free2(stolen, capacity);
    ncm_buffer_destroy(&source);
    ncm_buffer_destroy(&dest);
    return;
}

int
main(void) {
    ncm_buffer_compat_test_alias_and_direct_access();
    ncm_buffer_compat_test_wrappers();
    return 0;
}

#endif /* NCMPCPP_TESTS_NCM_BUFFER_COMPAT_C */
