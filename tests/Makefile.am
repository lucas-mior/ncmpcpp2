TESTS = c_foundation_test c_mpd_model_test c_mutable_song_test
check_PROGRAMS = c_foundation_test c_mpd_model_test c_mutable_song_test

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(libmpdclient_CFLAGS)

c_foundation_test_SOURCES = \
	c_foundation_test.c

c_foundation_test_LDADD = \
	$(top_builddir)/src/libncmpcpp_c.la \
	$(top_builddir)/cbase/libcbase.la \
	$(libmpdclient_LIBS) \
	$(taglib_LIBS)

c_mpd_model_test_SOURCES = \
	c_mpd_model_test.c

c_mpd_model_test_LDADD = \
	$(top_builddir)/src/libncmpcpp_c.la \
	$(top_builddir)/cbase/libcbase.la \
	$(libmpdclient_LIBS) \
	$(taglib_LIBS)

c_mutable_song_test_SOURCES = \
	c_mutable_song_test.c

c_mutable_song_test_LDADD = \
	$(top_builddir)/src/libncmpcpp_c.la \
	$(top_builddir)/cbase/libcbase.la \
	$(libmpdclient_LIBS) \
	$(taglib_LIBS)
