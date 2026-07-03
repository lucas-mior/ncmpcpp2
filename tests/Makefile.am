TESTS = c_foundation_test
check_PROGRAMS = c_foundation_test

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(libmpdclient_CFLAGS)

c_foundation_test_SOURCES = \
	c_foundation_test.c \
	../src/c/ncm_base.c \
	../src/c/ncm_error.c \
	../src/c/ncm_html.c \
	../src/c/ncm_path.c \
	../src/c/ncm_sample_buffer.c \
	../src/c/ncm_string.c \
	../src/c/ncm_type_conversions.c \
	../src/c/ncm_utf8.c

c_foundation_test_LDADD = \
	$(top_builddir)/cbase/libcbase.la
