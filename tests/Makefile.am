TESTS = c_foundation_test c_mpd_model_test c_mutable_song_test
check_PROGRAMS = c_foundation_test c_mpd_model_test c_mutable_song_test

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

c_mpd_model_test_SOURCES = \
	c_mpd_model_test.c \
	../src/c/ncm_base.c \
	../src/c/ncm_directory.c \
	../src/c/ncm_mpd_item.c \
	../src/c/ncm_path.c \
	../src/c/ncm_playlist.c \
	../src/c/ncm_song.c \
	../src/c/ncm_string.c

c_mpd_model_test_LDADD = \
	$(top_builddir)/cbase/libcbase.la \
	$(libmpdclient_LIBS)

c_mutable_song_test_SOURCES = \
	c_mutable_song_test.c \
	../src/c/ncm_mutable_song.c

c_mutable_song_test_LDADD = \
	$(top_builddir)/cbase/libcbase.la

