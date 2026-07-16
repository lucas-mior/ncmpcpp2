PACKAGE := ncmpcpp
VERSION := 0.10.2_dev

BUILD_DIR ?= build
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DOCDIR ?= $(PREFIX)/share/doc/ncmpcpp
MANDIR ?= $(PREFIX)/share/man
DESTDIR ?=

PKG_CONFIG ?= pkg-config
CC ?= cc
CXX ?= c++
AR ?= ar

CPPFLAGS ?=
CFLAGS ?= -O0 -g3
CXXFLAGS ?= -O0 -g3
LDFLAGS ?=
LDLIBS ?= -lm
CXXSTD ?= -std=c++20

PKG_DEPS := 'libmpdclient >= 2.8' ncursesw 'fftw3 >= 3' libcurl taglib_c
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKG_DEPS) 2>/dev/null)
PKG_LIBS := $(shell $(PKG_CONFIG) --libs $(PKG_DEPS) 2>/dev/null)
READLINE_CFLAGS := $(shell if $(PKG_CONFIG) --exists readline 2>/dev/null; then $(PKG_CONFIG) --cflags readline; fi)
READLINE_LIBS := $(shell if $(PKG_CONFIG) --exists readline 2>/dev/null; then $(PKG_CONFIG) --libs readline; else printf '%s' '-lreadline -lhistory'; fi)
CSTD ?= $(shell if printf 'int main(void) { return 0; }\n' | $(CC) -std=c23 -x c -c -o /dev/null - >/dev/null 2>&1; then printf '%s' -std=c23; elif printf 'int main(void) { return 0; }\n' | $(CC) -std=c2x -x c -c -o /dev/null - >/dev/null 2>&1; then printf '%s' -std=c2x; fi)

COMMON_CPPFLAGS := \
	-I$(BUILD_DIR) \
	-I. \
	-Isrc \
	-D_GNU_SOURCE \
	-D_DEFAULT_SOURCE \
	$(CPPFLAGS) \
	$(PKG_CFLAGS) \
	$(READLINE_CFLAGS)
WARNINGS := -Wall -Wextra -Wfatal-errors
THREAD_FLAGS := -pthread

OBJ_DIR := $(BUILD_DIR)/obj
TOOLS_STAMP := $(BUILD_DIR)/.tools-ok
CONFIG_H := $(BUILD_DIR)/config.h
BINARY := $(BUILD_DIR)/ncmpcpp
CBASE_LIB := $(BUILD_DIR)/libcbase.a
NCMPCPP_C_LIB := $(BUILD_DIR)/libncmpcpp_c.a
NCMPCPP_APP_C_LIB := $(BUILD_DIR)/libncmpcpp_app_c.a

CBASE_SRCS := cbase/cbase.c
NCMPCPP_C_SRCS := $(shell find src/c -type f -name '*.c' | sort)
APP_C_SRCS := $(shell find src -type f -name '*.c' ! -path 'src/c/*' | sort)
APP_CXX_SRCS := \
	src/actions_legacy.cpp \
	src/mpdpp.cpp \
	src/settings_legacy.cpp
C_TEST_SRCS := $(sort $(wildcard tests/*_test.c))
CXX_TEST_SRCS := $(sort $(wildcard tests/*_test.cpp))

CBASE_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(CBASE_SRCS))
NCMPCPP_C_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(NCMPCPP_C_SRCS))
APP_C_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(APP_C_SRCS))
APP_CXX_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.cpp.o,$(APP_CXX_SRCS))
C_TEST_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.c.o,$(C_TEST_SRCS))
CXX_TEST_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.cpp.o,$(CXX_TEST_SRCS))
CXX_TEST_SUPPORT_OBJS := \
	$(OBJ_DIR)/src/settings_legacy.cpp.o
C_TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/tests/%,$(C_TEST_SRCS))
CXX_TEST_BINS := $(patsubst tests/%.cpp,$(BUILD_DIR)/tests/%,$(CXX_TEST_SRCS))
TEST_BINS := $(C_TEST_BINS) $(CXX_TEST_BINS)

PLAYLIST_SCREEN_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_set_title \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_display \
	-Wl,--wrap=nc_window_has_coords \
	-Wl,--wrap=nc_window_print_char \
	-Wl,--wrap=nc_buffer_apply_property \
	-Wl,--wrap=nc_menu_refresh \
	-Wl,--wrap=ncm_status_state_player \
	-Wl,--wrap=ncm_status_state_current_song_position \
	-Wl,--wrap=ncm_mpd_client_get_queue \
	-Wl,--wrap=ncm_mpd_client_get_queue_changes \
	-Wl,--wrap=ncm_mpd_client_play_id
$(BUILD_DIR)/tests/c_playlist_screen_test: \
	LDFLAGS += $(PLAYLIST_SCREEN_TEST_WRAP_FLAGS)
PLAYLIST_EDITOR_SCREEN_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_set_title \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_display \
	-Wl,--wrap=nc_window_has_coords \
	-Wl,--wrap=nc_window_get_x \
	-Wl,--wrap=nc_window_print_data \
	-Wl,--wrap=nc_window_print_char \
	-Wl,--wrap=nc_menu_refresh \
	-Wl,--wrap=nc_screen_draw_vertical_separator \
	-Wl,--wrap=ncm_statusbar_print \
	-Wl,--wrap=ncm_statusbar_print_cstring \
	-Wl,--wrap=ncm_status_update_full \
	-Wl,--wrap=ncm_action_add_song_to_playlist \
	-Wl,--wrap=ncm_mpd_client_get_playlists \
	-Wl,--wrap=ncm_mpd_client_get_playlist_content \
	-Wl,--wrap=ncm_mpd_client_get_playlist_content_no_info \
	-Wl,--wrap=ncm_mpd_client_save_playlist \
	-Wl,--wrap=ncm_mpd_client_rename_playlist \
	-Wl,--wrap=ncm_mpd_client_delete_playlist \
	-Wl,--wrap=ncm_mpd_client_load_playlist
$(BUILD_DIR)/tests/c_playlist_editor_screen_test: \
	LDFLAGS += $(PLAYLIST_EDITOR_SCREEN_TEST_WRAP_FLAGS)
SCREEN_REGISTRY_BRIDGE_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_set_title \
	-Wl,--wrap=nc_screen_draw_vertical_separator
$(BUILD_DIR)/tests/c_screen_registry_bridge_test: \
	LDFLAGS += $(SCREEN_REGISTRY_BRIDGE_TEST_WRAP_FLAGS)
TAG_EDITOR_NATIVE_REGISTRATION_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_set_title \
	-Wl,--wrap=nc_screen_draw_vertical_separator
$(BUILD_DIR)/tests/tag_editor_native_registration_test: \
	LDFLAGS += $(TAG_EDITOR_NATIVE_REGISTRATION_TEST_WRAP_FLAGS)
TAG_EDITOR_SCREEN_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_has_coords \
	-Wl,--wrap=nc_window_set_title \
	-Wl,--wrap=nc_window_display \
	-Wl,--wrap=nc_window_print_data \
	-Wl,--wrap=nc_window_print_char \
	-Wl,--wrap=nc_buffer_apply_property \
	-Wl,--wrap=nc_menu_refresh \
	-Wl,--wrap=nc_screen_draw_vertical_separator \
	-Wl,--wrap=ncm_mpd_client_get_directory_list \
	-Wl,--wrap=ncm_mpd_client_get_songs \
	-Wl,--wrap=ncm_mutable_song_write \
	-Wl,--wrap=ncm_fs_rename \
	-Wl,--wrap=ncm_statusbar_print_cstring
$(BUILD_DIR)/tests/c_tag_editor_screen_test: \
	LDFLAGS += $(TAG_EDITOR_SCREEN_TEST_WRAP_FLAGS)
C_BROWSER_SCREEN_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_has_coords \
	-Wl,--wrap=ncm_mpd_client_get_directory_entries \
	-Wl,--wrap=ncm_mpd_client_get_directory_recursive \
	-Wl,--wrap=ncm_mpd_client_get_supported_extensions
$(BUILD_DIR)/tests/c_browser_screen_test: \
	LDFLAGS += $(C_BROWSER_SCREEN_TEST_WRAP_FLAGS)
PLAYLIST_ACTIONS_TEST_WRAP_FLAGS := \
	-Wl,--wrap=app_controller_current_screen \
	-Wl,--wrap=native_c_screens_current_type \
	-Wl,--wrap=native_c_screen_playlist \
	-Wl,--wrap=current_screen_current_search_constraint \
	-Wl,--wrap=current_screen_search \
	-Wl,--wrap=ncm_mpd_client_connected \
	-Wl,--wrap=ncm_mpd_client_version \
	-Wl,--wrap=ncm_mpd_client_start_command_list \
	-Wl,--wrap=ncm_mpd_client_commit_command_list \
	-Wl,--wrap=ncm_mpd_client_move \
	-Wl,--wrap=ncm_mpd_client_swap \
	-Wl,--wrap=ncm_mpd_client_shuffle_range \
	-Wl,--wrap=ncm_mpd_client_clear_queue \
	-Wl,--wrap=ncm_mpd_client_add_song_value \
	-Wl,--wrap=ncm_mpd_client_play_id \
	-Wl,--wrap=ncm_mpd_client_set_priority_song \
	-Wl,--wrap=ncm_mpd_client_set_crossfade \
	-Wl,--wrap=ncm_mpd_client_set_volume \
	-Wl,--wrap=ncm_mpd_client_add_random_tag \
	-Wl,--wrap=ncm_mpd_client_add_random_songs \
	-Wl,--wrap=ncm_mpd_client_seek_pos \
	-Wl,--wrap=ncm_mpd_client_save_playlist \
	-Wl,--wrap=ncm_mpd_client_delete_playlist \
	-Wl,--wrap=ncm_mpd_client_server_error_code \
	-Wl,--wrap=ncm_status_update_full \
	-Wl,--wrap=ncm_status_state_current_song_position \
	-Wl,--wrap=ncm_status_state_playlist_length \
	-Wl,--wrap=ncm_status_state_volume \
	-Wl,--wrap=ncm_status_state_player \
	-Wl,--wrap=ncm_status_state_total_time \
	-Wl,--wrap=native_playlist_screen_locate_position \
	-Wl,--wrap=ncm_statusbar_scoped_lock_init \
	-Wl,--wrap=ncm_statusbar_scoped_lock_destroy \
	-Wl,--wrap=ncm_statusbar_put \
	-Wl,--wrap=ncm_statusbar_print \
	-Wl,--wrap=ncm_statusbar_print_cstring \
	-Wl,--wrap=ncm_statusbar_format \
	-Wl,--wrap=ncm_statusbar_prompt_return_one_of \
	-Wl,--wrap=nc_window_prompt \
	-Wl,--wrap=nc_window_prompt_result_destroy \
	-Wl,--wrap=nc_window_print_data \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_set_title
$(BUILD_DIR)/tests/c_playlist_actions_test: \
	LDFLAGS += $(PLAYLIST_ACTIONS_TEST_WRAP_FLAGS)
PLAYLIST_BINDING_DISPATCH_TEST_WRAP_FLAGS := \
	-Wl,--wrap=native_c_screens_current_type \
	-Wl,--wrap=ncm_action_runtime_can_run \
	-Wl,--wrap=ncm_action_runtime_run \
	-Wl,--wrap=actions_legacy_runtime_execute_binding \
	-Wl,--wrap=actions_legacy_runtime_execute_action
$(BUILD_DIR)/tests/c_playlist_binding_dispatch_test: \
	LDFLAGS += $(PLAYLIST_BINDING_DISPATCH_TEST_WRAP_FLAGS)
PLAYLIST_EDITOR_BINDING_DISPATCH_TEST_WRAP_FLAGS := \
	-Wl,--wrap=native_c_screens_current_type \
	-Wl,--wrap=ncm_action_runtime_can_run \
	-Wl,--wrap=ncm_action_runtime_run \
	-Wl,--wrap=actions_legacy_runtime_execute_binding \
	-Wl,--wrap=actions_legacy_runtime_execute_action
$(BUILD_DIR)/tests/c_playlist_editor_binding_dispatch_test: \
	LDFLAGS += $(PLAYLIST_EDITOR_BINDING_DISPATCH_TEST_WRAP_FLAGS)
BROWSER_BINDING_DISPATCH_TEST_WRAP_FLAGS := \
	-Wl,--wrap=native_c_screens_current_type \
	-Wl,--wrap=ncm_action_runtime_can_run \
	-Wl,--wrap=ncm_action_runtime_run \
	-Wl,--wrap=actions_legacy_runtime_execute_binding \
	-Wl,--wrap=actions_legacy_runtime_execute_action
$(BUILD_DIR)/tests/c_browser_binding_dispatch_test: \
	LDFLAGS += $(BROWSER_BINDING_DISPATCH_TEST_WRAP_FLAGS)
TAG_EDITOR_BINDING_DISPATCH_TEST_WRAP_FLAGS := \
	-Wl,--wrap=native_c_screens_current_type \
	-Wl,--wrap=ncm_action_runtime_can_run \
	-Wl,--wrap=ncm_action_runtime_run \
	-Wl,--wrap=actions_legacy_runtime_execute_binding \
	-Wl,--wrap=actions_legacy_runtime_execute_action
$(BUILD_DIR)/tests/c_tag_editor_binding_dispatch_test: \
	LDFLAGS += $(TAG_EDITOR_BINDING_DISPATCH_TEST_WRAP_FLAGS)
SEARCH_ENGINE_SCREEN_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_set_title \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_width \
	-Wl,--wrap=nc_window_height \
	-Wl,--wrap=nc_window_display \
	-Wl,--wrap=nc_window_has_coords \
	-Wl,--wrap=nc_window_print_char \
	-Wl,--wrap=nc_buffer_apply_property \
	-Wl,--wrap=nc_menu_refresh \
	-Wl,--wrap=nc_screen_get_resize_params \
	-Wl,--wrap=ui_state_main_start_y \
	-Wl,--wrap=ui_state_main_height \
	-Wl,--wrap=ncm_mpd_client_start_search \
	-Wl,--wrap=ncm_mpd_client_add_search_tag \
	-Wl,--wrap=ncm_mpd_client_add_search_any \
	-Wl,--wrap=ncm_mpd_client_add_search_uri \
	-Wl,--wrap=ncm_mpd_client_commit_search_songs
$(BUILD_DIR)/tests/c_search_engine_screen_test: \
	LDFLAGS += $(SEARCH_ENGINE_SCREEN_TEST_WRAP_FLAGS)
PLAYLIST_SORT_TEST_WRAP_FLAGS := \
	-Wl,--wrap=ncm_mpd_client_start_command_list \
	-Wl,--wrap=ncm_mpd_client_swap \
	-Wl,--wrap=ncm_mpd_client_commit_command_list
$(BUILD_DIR)/tests/c_playlist_sort_test: \
	LDFLAGS += $(PLAYLIST_SORT_TEST_WRAP_FLAGS)
SORT_PLAYLIST_DIALOG_TEST_WRAP_FLAGS := \
	-Wl,--wrap=ncm_mpd_client_connected \
	-Wl,--wrap=ncm_playlist_sort_range \
	-Wl,--wrap=ncm_status_update_full \
	-Wl,--wrap=ncm_statusbar_print_cstring \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_display \
	-Wl,--wrap=nc_window_print_data \
	-Wl,--wrap=nc_window_has_coords \
	-Wl,--wrap=nc_menu_refresh
$(BUILD_DIR)/tests/c_sort_playlist_dialog_test: \
	LDFLAGS += $(SORT_PLAYLIST_DIALOG_TEST_WRAP_FLAGS)
PLAYLIST_MANAGEMENT_DIALOGS_TEST_WRAP_FLAGS := \
	-Wl,--wrap=ncm_mpd_client_get_playlists \
	-Wl,--wrap=ncm_mpd_client_start_command_list \
	-Wl,--wrap=ncm_mpd_client_add_song_to_playlist \
	-Wl,--wrap=ncm_mpd_client_add_song_value \
	-Wl,--wrap=ncm_mpd_client_commit_command_list \
	-Wl,--wrap=ncm_status_state_player \
	-Wl,--wrap=ncm_status_state_current_song_position \
	-Wl,--wrap=native_playlist_screen_current_song \
	-Wl,--wrap=native_playlist_screen_now_playing_song \
	-Wl,--wrap=ncm_status_handle_server_error_value \
	-Wl,--wrap=ncm_statusbar_scoped_lock_init \
	-Wl,--wrap=ncm_statusbar_scoped_lock_destroy \
	-Wl,--wrap=ncm_statusbar_put \
	-Wl,--wrap=ncm_statusbar_print \
	-Wl,--wrap=ncm_statusbar_print_cstring \
	-Wl,--wrap=ncm_statusbar_format \
	-Wl,--wrap=nc_window_prompt \
	-Wl,--wrap=nc_window_prompt_result_destroy \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_display \
	-Wl,--wrap=nc_window_print_data \
	-Wl,--wrap=nc_menu_refresh
$(BUILD_DIR)/tests/c_playlist_management_dialogs_test: \
	LDFLAGS += $(PLAYLIST_MANAGEMENT_DIALOGS_TEST_WRAP_FLAGS)
SELECTED_ITEMS_ACTION_TEST_WRAP_FLAGS := \
	-Wl,--wrap=native_c_screens_current_type \
	-Wl,--wrap=native_c_screen_playlist \
	-Wl,--wrap=native_playlist_screen_selected_songs \
	-Wl,--wrap=native_c_screen_tag_editor \
	-Wl,--wrap=native_tag_editor_screen_selected_songs \
	-Wl,--wrap=native_c_screen_selected_items_adder_open
$(BUILD_DIR)/tests/c_selected_items_adder_action_test: \
	LDFLAGS += $(SELECTED_ITEMS_ACTION_TEST_WRAP_FLAGS)
VISUALIZER_DRAWING_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_width \
	-Wl,--wrap=nc_window_height \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_clear \
	-Wl,--wrap=nc_window_go_to_xy \
	-Wl,--wrap=nc_window_push_color \
	-Wl,--wrap=nc_window_apply_format \
	-Wl,--wrap=nc_window_print_data
$(BUILD_DIR)/tests/c_visualizer_drawing_test: \
	LDFLAGS += $(VISUALIZER_DRAWING_TEST_WRAP_FLAGS)
VISUALIZER_CALLBACKS_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_width \
	-Wl,--wrap=nc_window_height \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_clear \
	-Wl,--wrap=nc_window_refresh \
	-Wl,--wrap=nc_window_go_to_xy \
	-Wl,--wrap=nc_window_push_color \
	-Wl,--wrap=nc_window_apply_format \
	-Wl,--wrap=nc_window_print_data \
	-Wl,--wrap=nc_screen_switcher_get_resize_params \
	-Wl,--wrap=ui_state_main_start_y \
	-Wl,--wrap=ui_state_main_height \
	-Wl,--wrap=ncm_title_draw_header \
	-Wl,--wrap=ncm_status_state_player
$(BUILD_DIR)/tests/c_visualizer_callbacks_test: \
	LDFLAGS += $(VISUALIZER_CALLBACKS_TEST_WRAP_FLAGS)
VISUALIZER_BEHAVIOR_TEST_WRAP_FLAGS := \
	-Wl,--wrap=nc_window_init \
	-Wl,--wrap=nc_window_destroy \
	-Wl,--wrap=nc_window_width \
	-Wl,--wrap=nc_window_height \
	-Wl,--wrap=nc_window_resize \
	-Wl,--wrap=nc_window_move_to \
	-Wl,--wrap=nc_window_clear \
	-Wl,--wrap=nc_window_refresh \
	-Wl,--wrap=nc_window_go_to_xy \
	-Wl,--wrap=nc_window_push_color \
	-Wl,--wrap=nc_window_apply_format \
	-Wl,--wrap=nc_window_print_data \
	-Wl,--wrap=nc_screen_switcher_get_resize_params \
	-Wl,--wrap=ui_state_main_start_y \
	-Wl,--wrap=ui_state_main_height \
	-Wl,--wrap=ncm_title_draw_header \
	-Wl,--wrap=ncm_status_state_player \
	-Wl,--wrap=ncm_statusbar_message_delay_time \
	-Wl,--wrap=ncm_statusbar_format
$(BUILD_DIR)/tests/c_visualizer_behavior_test: \
	LDFLAGS += $(VISUALIZER_BEHAVIOR_TEST_WRAP_FLAGS)
VISUALIZER_ACTIONS_TEST_WRAP_FLAGS := \
	-Wl,--wrap=app_controller_current_screen \
	-Wl,--wrap=native_c_screen_visualizer_register \
	-Wl,--wrap=native_c_screen_visualizer_is_current \
	-Wl,--wrap=native_c_screens_switch_to_type \
	-Wl,--wrap=native_c_screen_visualizer \
	-Wl,--wrap=native_visualizer_screen_toggle_type
$(BUILD_DIR)/tests/c_visualizer_actions_test: \
	LDFLAGS += $(VISUALIZER_ACTIONS_TEST_WRAP_FLAGS)
STATUS_TEST_WRAP_FLAGS := \
	-Wl,--wrap=native_c_screen_visualizer \
	-Wl,--wrap=native_visualizer_screen_close_data_source \
	-Wl,--wrap=native_visualizer_screen_open_data_source \
	-Wl,--wrap=native_visualizer_screen_find_output_id
$(BUILD_DIR)/tests/c_status_test: \
	LDFLAGS += $(STATUS_TEST_WRAP_FLAGS)
DEPS := \
	$(CBASE_OBJS:.o=.d) \
	$(NCMPCPP_C_OBJS:.o=.d) \
	$(APP_C_OBJS:.o=.d) \
	$(APP_CXX_OBJS:.o=.d) \
	$(C_TEST_OBJS:.o=.d) \
	$(CXX_TEST_OBJS:.o=.d)

.PHONY: all check install clean help FORCE
.DELETE_ON_ERROR:
.SECONDARY: $(CBASE_OBJS) $(NCMPCPP_C_OBJS) $(APP_C_OBJS) $(APP_CXX_OBJS) $(C_TEST_OBJS) $(CXX_TEST_OBJS)

all: $(BINARY)

$(TOOLS_STAMP): FORCE
	@mkdir -p $(BUILD_DIR)
	@command -v $(PKG_CONFIG) >/dev/null 2>&1 || { printf 'missing command: %s\n' '$(PKG_CONFIG)' >&2; exit 1; }
	@$(PKG_CONFIG) --exists $(PKG_DEPS) || { printf 'missing pkg-config package(s): %s\n' "$(PKG_DEPS)" >&2; exit 1; }
	@test -n "$(CSTD)" || { printf '%s\n' 'C compiler does not support C23 or C2x' >&2; exit 1; }
	@printf 'int main() { return 0; }\n' | $(CXX) $(CXXSTD) -x c++ -c -o /dev/null - >/dev/null 2>&1 || { printf '%s\n' 'C++ compiler does not support C++20' >&2; exit 1; }
	@touch $@

$(CONFIG_H): | $(TOOLS_STAMP)
	@mkdir -p $(@D)
	@{ \
		printf '%s\n' '#ifndef NCMPCPP_CONFIG_H'; \
		printf '%s\n' '#define NCMPCPP_CONFIG_H'; \
		printf '\n'; \
		printf '%s\n' '#define ENABLE_OUTPUTS 1'; \
		printf '%s\n' '#define ENABLE_VISUALIZER 1'; \
		printf '%s\n' '#define HAVE_CURL_CURL_H 1'; \
		printf '%s\n' '#define HAVE_CURSES_H 1'; \
		printf '%s\n' '#define HAVE_FFTW3_H 1'; \
		printf '%s\n' '#define HAVE_LIBNCURSESW 1'; \
		printf '%s\n' '#define HAVE_LIBREADLINE 1'; \
		printf '%s\n' '#define HAVE_MPD_CLIENT_H 1'; \
		printf '%s\n' '#define HAVE_NETINET_IN_H 1'; \
		printf '%s\n' '#define HAVE_NETINET_TCP_H 1'; \
		printf '%s\n' '#define HAVE_READLINE_HISTORY 1'; \
		printf '%s\n' '#define HAVE_READLINE_HISTORY_H 1'; \
		printf '%s\n' '#define HAVE_READLINE_READLINE_H 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_FILE_NEW 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_FILE_SAVE 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_H 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_PROPERTY_GET 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_PROPERTY_SET 1'; \
		printf '%s\n' '#define HAVE_TAGLIB_PROPERTY_SET_APPEND 1'; \
		printf '%s\n' '#define PACKAGE "$(PACKAGE)"'; \
		printf '%s\n' '#define PACKAGE_NAME "$(PACKAGE)"'; \
		printf '%s\n' '#define PACKAGE_STRING "$(PACKAGE) $(VERSION)"'; \
		printf '%s\n' '#define PACKAGE_TARNAME "$(PACKAGE)"'; \
		printf '%s\n' '#define PACKAGE_VERSION "$(VERSION)"'; \
		printf '%s\n' '#define VERSION "$(VERSION)"'; \
		printf '\n'; \
		printf '%s\n' '#endif'; \
	} >$@

$(OBJ_DIR)/%.c.o: %.c $(CONFIG_H) | $(TOOLS_STAMP)
	@mkdir -p $(@D)
	@printf 'CC  %s\n' '$<'
	@$(CC) \
		$(COMMON_CPPFLAGS) \
		$(CSTD) \
		$(WARNINGS) \
		$(CFLAGS) \
		$(THREAD_FLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

$(OBJ_DIR)/%.cpp.o: %.cpp $(CONFIG_H) | $(TOOLS_STAMP)
	@mkdir -p $(@D)
	@printf 'CXX %s\n' '$<'
	@$(CXX) \
		$(COMMON_CPPFLAGS) \
		$(CXXSTD) \
		$(WARNINGS) \
		$(CXXFLAGS) \
		$(THREAD_FLAGS) \
		-MMD \
		-MP \
		-c $< \
		-o $@

$(CBASE_LIB): $(CBASE_OBJS)
	@printf 'AR  %s\n' '$@'
	@rm -f $@
	@$(AR) rcs $@ $^

$(NCMPCPP_C_LIB): $(NCMPCPP_C_OBJS)
	@printf 'AR  %s\n' '$@'
	@rm -f $@
	@$(AR) rcs $@ $^

$(NCMPCPP_APP_C_LIB): $(APP_C_OBJS)
	@printf 'AR  %s\n' '$@'
	@rm -f $@
	@$(AR) rcs $@ $^

$(BINARY): $(APP_C_OBJS) $(APP_CXX_OBJS) $(NCMPCPP_C_LIB) $(CBASE_LIB)
	@printf 'LD  %s\n' '$@'
	@$(CXX) $(LDFLAGS) -o $@ \
		$(APP_C_OBJS) \
		$(APP_CXX_OBJS) \
		$(NCMPCPP_C_LIB) \
		$(CBASE_LIB) \
		$(READLINE_LIBS) \
		$(PKG_LIBS) \
		$(LDLIBS) \
		$(THREAD_FLAGS)

$(C_TEST_BINS): $(BUILD_DIR)/tests/%: $(OBJ_DIR)/tests/%.c.o $(NCMPCPP_C_LIB) $(NCMPCPP_APP_C_LIB) $(CBASE_LIB)
	@mkdir -p $(@D)
	@printf 'LD  %s\n' '$@'
	@$(CC) $(LDFLAGS) -o $@ \
		$< \
		-Wl,--start-group \
		$(NCMPCPP_APP_C_LIB) \
		$(NCMPCPP_C_LIB) \
		$(CBASE_LIB) \
		-Wl,--end-group \
		$(READLINE_LIBS) \
		$(PKG_LIBS) \
		$(LDLIBS) \
		$(THREAD_FLAGS)

$(CXX_TEST_BINS): $(BUILD_DIR)/tests/%: $(OBJ_DIR)/tests/%.cpp.o $(CXX_TEST_SUPPORT_OBJS) $(NCMPCPP_C_LIB) $(NCMPCPP_APP_C_LIB) $(CBASE_LIB)
	@mkdir -p $(@D)
	@printf 'LD  %s\n' '$@'
	@$(CXX) $(LDFLAGS) -o $@ \
		$< \
		$(CXX_TEST_SUPPORT_OBJS) \
		-Wl,--start-group \
		$(NCMPCPP_APP_C_LIB) \
		$(NCMPCPP_C_LIB) \
		$(CBASE_LIB) \
		-Wl,--end-group \
		$(READLINE_LIBS) \
		$(PKG_LIBS) \
		$(LDLIBS) \
		$(THREAD_FLAGS)


check: $(TEST_BINS)
	@set -e; \
	for test in $(TEST_BINS); do \
		printf 'TEST %s\n' "$$test"; \
		"$$test"; \
	done

install: $(BINARY)
	install -d '$(DESTDIR)$(BINDIR)'
	install -m 755 '$(BINARY)' '$(DESTDIR)$(BINDIR)/ncmpcpp'
	install -d '$(DESTDIR)$(DOCDIR)'
	install -m 644 AUTHORS COPYING '$(DESTDIR)$(DOCDIR)'
	install -m 644 doc/bindings doc/config '$(DESTDIR)$(DOCDIR)'
	install -d '$(DESTDIR)$(MANDIR)/man1'
	install -m 644 doc/ncmpcpp.1 '$(DESTDIR)$(MANDIR)/man1/ncmpcpp.1'

clean:
	rm -rf '$(BUILD_DIR)'

help:
	@printf '%s\n' 'usage: make [all|check|install|clean|help]'
	@printf '%s\n' ''
	@printf '%s\n' 'Common variables:'
	@printf '%s\n' '  CC, CXX, AR        compiler and archive commands'
	@printf '%s\n' '  CFLAGS, CXXFLAGS   extra compiler flags'
	@printf '%s\n' '  CPPFLAGS, LDFLAGS  extra preprocessor and linker flags'
	@printf '%s\n' '  LDLIBS             extra libraries, default: -lm'
	@printf '%s\n' '  BUILD_DIR          build output directory, default: build'
	@printf '%s\n' '  PREFIX             install prefix, default: /usr/local'
	@printf '%s\n' '  BINDIR             binary install directory, default: PREFIX/bin'
	@printf '%s\n' '  DOCDIR             docs install directory, default: PREFIX/share/doc/ncmpcpp'
	@printf '%s\n' '  MANDIR             man install directory, default: PREFIX/share/man'

-include $(DEPS)
