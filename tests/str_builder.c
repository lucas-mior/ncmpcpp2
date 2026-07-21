#if !defined(NCMPCPP_TESTS_STR_BUILDER_C)
#define NCMPCPP_TESTS_STR_BUILDER_C

#include "cbase/util.c"

static void
str_builder_test_init_and_empty_free(void) {
    StrBuilder str_builder;
    StrBuilder zeroed = {0};

    ASSERT(zeroed.data == NULL);
    ASSERT_EQUAL(zeroed.len, 0);
    ASSERT_EQUAL(zeroed.cap, 0);
    sb_append(&zeroed, STRLIT_ARGS("zero initialized"));
    ASSERT_EQUAL(zeroed.data, "zero initialized");
    sb_free(&zeroed);

    sb_init(&str_builder);
    ASSERT(str_builder.data == NULL);
    ASSERT_EQUAL(str_builder.len, 0);
    ASSERT_EQUAL(str_builder.cap, 0);

    sb_free(&str_builder);
    ASSERT(str_builder.data == NULL);
    ASSERT_EQUAL(str_builder.len, 0);
    ASSERT_EQUAL(str_builder.cap, 0);
    return;
}

static void
str_builder_test_append_clear_and_reuse(void) {
    StrBuilder str_builder;
    char *allocation;
    int32 capacity;

    sb_init(&str_builder);
    sb_append(&str_builder, STRLIT_ARGS("first"));
    ASSERT_EQUAL(str_builder.data, "first");
    ASSERT_EQUAL(str_builder.len, STRLIT_LEN("first"));
    ASSERT_EQUAL(str_builder.cap, STR_BUILDER_INITIAL_CAPACITY);

    allocation = str_builder.data;
    capacity = str_builder.cap;
    sb_clear(&str_builder);
    ASSERT(str_builder.data == allocation);
    ASSERT_EQUAL(str_builder.len, 0);
    ASSERT_EQUAL(str_builder.cap, capacity);
    ASSERT_EQUAL(str_builder.data, "");

    sb_append(&str_builder, STRLIT_ARGS("second"));
    ASSERT(str_builder.data == allocation);
    ASSERT_EQUAL(str_builder.data, "second");
    ASSERT_EQUAL(str_builder.len, STRLIT_LEN("second"));
    ASSERT_EQUAL(str_builder.cap, capacity);

    sb_free(&str_builder);
    return;
}

static void
str_builder_test_nonpositive_operations_are_noops(void) {
    StrBuilder str_builder;

    sb_init(&str_builder);
    sb_append(&str_builder, NULL, 0);
    sb_append(&str_builder, NULL, -1);
    sb_reserve(&str_builder, 0);
    sb_reserve(&str_builder, -1);
    ASSERT(str_builder.data == NULL);
    ASSERT_EQUAL(str_builder.len, 0);
    ASSERT_EQUAL(str_builder.cap, 0);

    sb_printf(&str_builder, "%.*s", 0, "");
    ASSERT(str_builder.data == NULL);
    sb_append(&str_builder, STRLIT_ARGS("value"));
    sb_printf(&str_builder, "%.*s", 0, "");
    ASSERT_EQUAL(str_builder.data, "value");
    ASSERT_EQUAL(str_builder.len, STRLIT_LEN("value"));

    sb_free(&str_builder);
    return;
}

static void
str_builder_test_copy_and_self_copy(void) {
    StrBuilder source;
    StrBuilder dest;
    char bytes[] = {'a', '\0', 'b'};

    sb_init(&source);
    sb_init(&dest);
    sb_append(&source, bytes, LENGTH(bytes));
    sb_append(&dest, STRLIT_ARGS("old destination"));

    ASSERT(sb_copy(&dest, &source));
    ASSERT(dest.data != source.data);
    ASSERT_EQUAL(dest.len, source.len);
    ASSERT(memcmp64(dest.data, source.data, source.len) == 0);
    ASSERT_EQUAL(dest.data[dest.len], '\0');

    ASSERT(sb_copy(&dest, &dest));
    ASSERT_EQUAL(dest.len, source.len);
    ASSERT(memcmp64(dest.data, source.data, source.len) == 0);

    {
        char *allocation = dest.data;
        int32 capacity = dest.cap;

        sb_clear(&source);
        ASSERT(sb_copy(&dest, &source));
        ASSERT(dest.data == allocation);
        ASSERT_EQUAL(dest.data, "");
        ASSERT_EQUAL(dest.len, 0);
        ASSERT_EQUAL(dest.cap, capacity);
    }

    ASSERT(sb_copy(&dest, NULL));
    ASSERT(dest.data == NULL);
    ASSERT_EQUAL(dest.len, 0);
    ASSERT_EQUAL(dest.cap, 0);
    ASSERT(!sb_copy(NULL, &source));

    sb_free(&source);
    sb_free(&dest);
    return;
}

static void
str_builder_test_move_transfers_ownership(void) {
    StrBuilder source;
    StrBuilder dest;
    char *allocation;
    int32 capacity;

    sb_init(&source);
    sb_init(&dest);
    sb_append(&source, STRLIT_ARGS("source value"));
    sb_append(&dest, STRLIT_ARGS("destination value"));
    allocation = source.data;
    capacity = source.cap;

    sb_move(&dest, &source);
    ASSERT(dest.data == allocation);
    ASSERT_EQUAL(dest.data, "source value");
    ASSERT_EQUAL(dest.len, STRLIT_LEN("source value"));
    ASSERT_EQUAL(dest.cap, capacity);
    ASSERT(source.data == NULL);
    ASSERT_EQUAL(source.len, 0);
    ASSERT_EQUAL(source.cap, 0);

    sb_move(&dest, &dest);
    ASSERT(dest.data == allocation);
    ASSERT_EQUAL(dest.data, "source value");

    sb_move(&dest, NULL);
    ASSERT(dest.data == NULL);
    ASSERT_EQUAL(dest.len, 0);
    ASSERT_EQUAL(dest.cap, 0);

    sb_append(&source, STRLIT_ARGS("kept"));
    allocation = source.data;
    sb_move(NULL, &source);
    ASSERT(source.data == allocation);
    ASSERT_EQUAL(source.data, "kept");

    sb_free(&source);
    sb_free(&dest);
    return;
}

static void
str_builder_test_set_validation_and_self_shrink(void) {
    StrBuilder str_builder;
    char *allocation;
    int32 capacity;

    sb_init(&str_builder);
    ASSERT(sb_set(&str_builder, STRLIT_ARGS("abcdef")));
    allocation = str_builder.data;
    capacity = str_builder.cap;

    ASSERT(!sb_set(NULL, STRLIT_ARGS("value")));
    ASSERT(!sb_set(&str_builder, "value", -1));
    ASSERT(!sb_set(&str_builder, NULL, 1));
    ASSERT_EQUAL(str_builder.data, "abcdef");
    ASSERT_EQUAL(str_builder.len, 6);

    ASSERT(sb_set(&str_builder, str_builder.data, 3));
    ASSERT(str_builder.data == allocation);
    ASSERT_EQUAL(str_builder.data, "abc");
    ASSERT_EQUAL(str_builder.len, 3);
    ASSERT_EQUAL(str_builder.cap, capacity);

    ASSERT(!sb_set(&str_builder, str_builder.data, 4));
    ASSERT_EQUAL(str_builder.data, "abc");
    ASSERT_EQUAL(str_builder.len, 3);

    ASSERT(sb_set(&str_builder, NULL, 0));
    ASSERT(str_builder.data == allocation);
    ASSERT_EQUAL(str_builder.data, "");
    ASSERT_EQUAL(str_builder.len, 0);
    ASSERT_EQUAL(str_builder.cap, capacity);

    sb_free(&str_builder);
    return;
}

static void
str_builder_test_embedded_null_and_append_byte(void) {
    StrBuilder str_builder;

    sb_init(&str_builder);
    sb_append_byte(&str_builder, 'A');
    sb_append_byte(&str_builder, '\0');
    sb_append_byte(&str_builder, 'B');

    ASSERT_EQUAL(str_builder.len, 3);
    ASSERT_EQUAL(str_builder.data[0], 'A');
    ASSERT_EQUAL(str_builder.data[1], '\0');
    ASSERT_EQUAL(str_builder.data[2], 'B');
    ASSERT_EQUAL(str_builder.data[3], '\0');

    sb_free(&str_builder);
    return;
}

static void
str_builder_test_reserve_direct_write_and_growth(void) {
    StrBuilder str_builder;
    char long_string[] = "0123456789abcdefghijklmnopqrstuv";
    int32 long_string_len = LENGTH(long_string) - 1;
    int32 direct_len = STRLIT_LEN("direct");

    sb_init(&str_builder);
    sb_reserve(&str_builder, 8);
    ASSERT(str_builder.data);
    ASSERT_EQUAL(str_builder.cap, STR_BUILDER_INITIAL_CAPACITY);

    memcpy64(str_builder.data, "direct", direct_len);
    str_builder.len = direct_len;
    str_builder.data[str_builder.len] = '\0';
    sb_append_byte(&str_builder, '!');
    ASSERT_EQUAL(str_builder.data, "direct!");
    ASSERT_EQUAL(str_builder.len, direct_len + 1);

    sb_append(&str_builder, long_string, long_string_len);
    ASSERT_EQUAL(str_builder.len, direct_len + 1 + long_string_len);
    ASSERT(str_builder.cap >= str_builder.len + 1);
    ASSERT_EQUAL(str_builder.data[str_builder.len], '\0');
    ASSERT(memcmp64(str_builder.data + direct_len + 1, long_string,
                    long_string_len) == 0);

    sb_free(&str_builder);
    return;
}

static void
str_builder_test_printf_appends(void) {
    StrBuilder str_builder;

    sb_init(&str_builder);
    sb_append(&str_builder, STRLIT_ARGS("prefix "));
    sb_printf(&str_builder, "%s %d", "value", 42);
    ASSERT_EQUAL(str_builder.data, "prefix value 42");
    ASSERT_EQUAL(str_builder.len, STRLIT_LEN("prefix value 42"));

    sb_free(&str_builder);
    return;
}

static void
str_builder_test_steal_transfers_exact_allocation(void) {
    StrBuilder str_builder;
    char bytes[] = {'x', '\0', 'y'};
    char *allocation;
    char *stolen;
    int32 expected_len;
    int32 expected_cap;
    int32 len = -1;
    int32 cap = -1;

    sb_init(&str_builder);
    stolen = sb_steal(&str_builder, &len, &cap);
    ASSERT(stolen == NULL);
    ASSERT_EQUAL(len, 0);
    ASSERT_EQUAL(cap, 0);

    sb_append(&str_builder, bytes, LENGTH(bytes));
    allocation = str_builder.data;
    expected_len = str_builder.len;
    expected_cap = str_builder.cap;
    stolen = sb_steal(&str_builder, &len, &cap);

    ASSERT(stolen == allocation);
    ASSERT_EQUAL(len, expected_len);
    ASSERT_EQUAL(cap, expected_cap);
    ASSERT(memcmp64(stolen, bytes, LENGTH(bytes)) == 0);
    ASSERT_EQUAL(stolen[len], '\0');
    ASSERT(str_builder.data == NULL);
    ASSERT_EQUAL(str_builder.len, 0);
    ASSERT_EQUAL(str_builder.cap, 0);

    free2(stolen, cap);

    sb_append(&str_builder, STRLIT_ARGS("reused"));
    expected_cap = str_builder.cap;
    stolen = sb_steal(&str_builder, NULL, NULL);
    ASSERT_EQUAL(stolen, "reused");
    ASSERT(str_builder.data == NULL);
    ASSERT_EQUAL(str_builder.len, 0);
    ASSERT_EQUAL(str_builder.cap, 0);
    free2(stolen, expected_cap);
    return;
}

static void
str_builder_test_array_append_clear_and_destroy(void) {
    StrBuilderArray array;
    StrBuilder *item;
    StrBuilder source;
    char bytes[] = {'a', '\0', 'b'};
    StrBuilder *allocation;
    int32 capacity;

    str_builder_array_init(&array);
    sb_init(&source);
    ASSERT(str_builder_array_reserve(&array, 2));
    allocation = array.items;
    capacity = array.cap;

    item = str_builder_array_append(&array);
    ASSERT(item);
    sb_append(item, STRLIT_ARGS("first"));

    sb_append(&source, bytes, LENGTH(bytes));
    ASSERT(str_builder_array_append_copy(&array, &source));
    ASSERT_EQUAL(array.len, 2);
    ASSERT(array.items == allocation);
    ASSERT_EQUAL(array.items[0].data, "first");
    ASSERT_EQUAL(array.items[1].len, LENGTH(bytes));
    ASSERT(memcmp64(array.items[1].data, bytes, LENGTH(bytes)) == 0);
    ASSERT(array.items[1].data != source.data);

    str_builder_array_clear(&array);
    ASSERT(array.items == allocation);
    ASSERT_EQUAL(array.len, 0);
    ASSERT_EQUAL(array.cap, capacity);
    ASSERT(array.items[1].data == NULL);

    sb_free(&source);
    str_builder_array_destroy(&array);
    ASSERT(array.items == NULL);
    ASSERT_EQUAL(array.len, 0);
    ASSERT_EQUAL(array.cap, 0);
    return;
}

static void
str_builder_test_array_copy_move_and_swap(void) {
    StrBuilderArray source;
    StrBuilderArray dest;
    StrBuilderArray other;
    StrBuilder *item;
    StrBuilder *source_allocation;

    str_builder_array_init(&source);
    str_builder_array_init(&dest);
    str_builder_array_init(&other);

    item = str_builder_array_append(&source);
    ASSERT(item);
    sb_append(item, STRLIT_ARGS("source"));
    item = str_builder_array_append(&source);
    ASSERT(item);
    sb_append(item, STRLIT_ARGS("second"));

    ASSERT(str_builder_array_copy(&dest, &source));
    ASSERT_EQUAL(dest.len, source.len);
    ASSERT(dest.items != source.items);
    ASSERT(dest.items[0].data != source.items[0].data);
    ASSERT_EQUAL(dest.items[0].data, "source");
    ASSERT_EQUAL(dest.items[1].data, "second");

    ASSERT(str_builder_array_copy(&dest, &dest));
    ASSERT_EQUAL(dest.len, 2);
    ASSERT_EQUAL(dest.items[0].data, "source");

    source_allocation = source.items;
    str_builder_array_move(&other, &source);
    ASSERT(other.items == source_allocation);
    ASSERT_EQUAL(other.len, 2);
    ASSERT(source.items == NULL);
    ASSERT_EQUAL(source.len, 0);
    ASSERT_EQUAL(source.cap, 0);

    str_builder_array_swap(&dest, &other);
    ASSERT(dest.items == source_allocation);
    ASSERT_EQUAL(dest.items[0].data, "source");
    ASSERT_EQUAL(other.items[0].data, "source");
    ASSERT(other.items != dest.items);

    str_builder_array_move(&dest, NULL);
    ASSERT(dest.items == NULL);
    ASSERT_EQUAL(dest.len, 0);
    ASSERT_EQUAL(dest.cap, 0);

    ASSERT(str_builder_array_append_copy(&source, NULL) == false);

    str_builder_array_destroy(&source);
    str_builder_array_destroy(&dest);
    str_builder_array_destroy(&other);
    return;
}

int
main(void) {
    str_builder_test_init_and_empty_free();
    str_builder_test_append_clear_and_reuse();
    str_builder_test_nonpositive_operations_are_noops();
    str_builder_test_copy_and_self_copy();
    str_builder_test_move_transfers_ownership();
    str_builder_test_set_validation_and_self_shrink();
    str_builder_test_embedded_null_and_append_byte();
    str_builder_test_reserve_direct_write_and_growth();
    str_builder_test_printf_appends();
    str_builder_test_steal_transfers_exact_allocation();
    str_builder_test_array_append_clear_and_destroy();
    str_builder_test_array_copy_move_and_swap();
    return 0;
}

#endif /* NCMPCPP_TESTS_STR_BUILDER_C */
