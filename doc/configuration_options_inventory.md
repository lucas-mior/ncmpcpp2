# Configuration subsystem architecture

`src/configuration_options.def` is the authoritative schema for ncmpcpp2
configuration options. The schema currently expands to 134 effective options.
The `visualizer_type` entry has an FFTW-dependent default, so both source
branches exist in the `.def` file but only one is active in a build.

The configuration subsystem is intentionally split into four concerns:

1. the schema describes option syntax, storage, defaults, and type-specific
   metadata;
2. parser helpers convert one option value into `Configuration` state;
3. `configuration_validate()` checks relationships between already parsed
   fields;
4. runtime/application code consumes `Configuration` and applies environment,
   terminal, MPD, and command-line behavior.

This keeps configuration parsing independent from runtime side effects and
makes the schema the only place that ordinary options need to be declared.

## Files

- `src/configuration_options.def`: authoritative option schema.
- `src/configuration_options_pass.h`: include-based X-macro pass helper. It
  supplies no-op definitions for option kinds not handled by a pass, includes
  the schema, and undefines every pass macro afterward.
- `src/settings.h`: generated option IDs and generated `Configuration` fields.
- `src/settings.c`: type-specific parser helpers, generated `apply_*`
  functions, generated option descriptors, reading/default application,
  validation, and runtime application.
- `src/settings_types.c`: array support plus generated initialization and
  destruction for `Configuration`.
- `src/configuration.c`: startup integration, configuration-file loading,
  runtime application, environment overrides, and command-line overrides.
- `tests/c_settings_baseline_test.c`: schema-derived structural, default,
  numeric-range, lifecycle, and runtime-precedence coverage.

`configuration_options.def` and `configuration_options_pass.h` intentionally
have no include guards. Multiple inclusion is their purpose.

## Generated artifacts

The same schema generates all of the following:

- `enum SettingsOptionId`, including `SETTINGS_OPTION_COUNT`;
- option-backed fields in `Configuration`;
- intrinsic companion fields such as string lengths, cached display widths,
  optional-value presence bits, and derived column format state;
- compile-time field-type assertions;
- `apply_<option>()` functions;
- the `ncmpcpp_options` descriptor table;
- zero/empty initialization appropriate for each storage type;
- destruction for every owning storage type;
- schema-derived regression tests.

The descriptor table has inferred array length and a static assertion against
`SETTINGS_OPTION_COUNT`. Runtime tests also verify that each generated ID maps
to the expected name, default, and `apply_*` function.

## Supported option kinds

### Scalar and string-like types

`XX_BOOL(NAME, DEFAULT_VALUE)`

Stores a `bool` and parses the usual yes/no syntax.

`XX_INT_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)`

Stores an `int32`. Every integer setting uses this form, including effectively
unbounded values via `INT32_MIN` and `INT32_MAX`.

`XX_DOUBLE_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)`

Stores a `double`. Every double setting uses this form. `HUGE_VAL` is used for
an unbounded upper endpoint when needed.

`XX_STRING(NAME, DEFAULT_VALUE)`

Stores an owned `char *NAME` plus `int32 NAME_len`. The bytes are copied
verbatim.

`XX_PATH(NAME, DEFAULT_VALUE)`

Has the same storage shape as `XX_STRING`, but expands a leading `~`.

`XX_DIR(NAME, DEFAULT_VALUE)`

Has the same storage shape as `XX_PATH` and also ensures a trailing slash.

### Enum and choice types

`XX_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER)`

Stores `C_TYPE` and delegates string-to-enum conversion to `PARSER`.
Compatibility aliases belong in a small parser adapter when needed rather than
in the generic enum machinery.

`XX_OPTIONAL_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER, PRESENT_FIELD,
UNSET_VALUE)`

Stores the enum plus a generated presence flag. An empty value represents the
unset state.

`XX_NAMED_BOOL(NAME, DEFAULT_VALUE, TRUE_VALUE, FALSE_VALUE)`

Stores a `bool` whose external syntax is a pair of meaningful names instead of
yes/no.

`XX_UINT32_CHOICE(NAME, DEFAULT_VALUE, PARSER, UNSET_VALUE)`

Stores a `uint32` choice/bit-flag value parsed by a dedicated parser. This is
used where boolean or enum storage would misrepresent the runtime value.

### Color and formatting types

`XX_COLOR(NAME, DEFAULT_VALUE)`

Stores `NcColor`.

`XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE)`

Stores `NcFormattedColor`, including the lifecycle needed by its formatting
attributes.

`XX_BORDER(NAME, DEFAULT_VALUE)`

Stores `NcBorder` parsed from color syntax.

`XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS)`

Stores `NcmFormatAst`. `FLAGS` declares which format-language features are
valid for that option.

`XX_BUFFER(NAME, DEFAULT_VALUE, KEEP_EXISTING)`

Stores a rendered `NcBuffer`. `KEEP_EXISTING` preserves an already populated
buffer instead of replacing it.

`XX_BUFFER_WIDTH(NAME, DEFAULT_VALUE, KEEP_EXISTING)`

Stores the same buffer plus generated `int32 NAME_length` containing the
rendered display width.

`XX_LOOK(NAME, DEFAULT_VALUE, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)`

Stores a `StrBuilder` containing a constrained UTF-8 glyph/look string.
`PAD_TO_MAX` preserves the progress-bar compatibility behavior where a shorter
accepted value is NUL-padded to the maximum length.

### Collection and structured types

`XX_RATIO(NAME, DEFAULT_VALUE, EXPECTED_LEN)`

Stores `NcmInt32Array` parsed from a colon-separated integer ratio with an
exact element count.

`XX_FORMATTED_COLOR_LIST(NAME, DEFAULT_VALUE)`

Stores `NcmFormattedColorArray` parsed from the common comma-separated list
syntax.

`XX_LYRICS_FETCHERS(NAME, DEFAULT_VALUE)`

Stores `NcmLyricsFetcherRegistry` using the same list traversal with
fetcher-specific validation.

`XX_SCREEN_LIST(NAME, DEFAULT_VALUE, PREVIOUS_FIELD)`

Stores `ScreenTypeArray` and owns the generated `PREVIOUS_FIELD` companion for
the special `previous` mode.

`XX_COLUMNS(NAME, DEFAULT_VALUE, FORMAT_FIELD)`

Owns both the parsed `ColumnArray` and the derived `NcmFormatAst` used by column
mode. The columns grammar is sufficiently domain-specific that it remains its
own semantic configuration type.

## Companion and derived fields

A physical `Configuration` member is not necessarily an independent option.
Companion state belongs to the macro entry that owns it. Current examples are:

- `*_len` fields for `XX_STRING`, `XX_PATH`, and `XX_DIR`;
- `*_length` cached widths from `XX_BUFFER_WIDTH`;
- `has_startup_slave_screen_type` from `XX_OPTIONAL_ENUM`;
- `screen_switcher_previous` from `XX_SCREEN_LIST`;
- `song_columns_mode_format` from `XX_COLUMNS`.

Do not add such fields as separate schema entries.

## Reading, defaults, and validation

`configuration_read()` performs the following sequence:

1. clear the target `Configuration`;
2. parse explicit values from each existing configuration file;
3. reject duplicate option names across all files participating in that read;
4. apply descriptor defaults to options not explicitly set;
5. call `configuration_validate()` for cross-field constraints.

Duplicate tracking is local to one read. The static descriptor table is
immutable and does not retain per-read state.

Scalar range validation belongs in `XX_INT_RANGE` or `XX_DOUBLE_RANGE`.
Relationships between fields belong in `configuration_validate()`. For
example, the visualizer maximum frequency is parsed independently and the
maximum-greater-than-minimum requirement is checked only after all explicit
values and defaults are present.

Configuration fields store the configuration-facing representation when
practical. Runtime conversions use helpers such as
`configuration_locked_screen_width_fraction()` and
`configuration_search_engine_default_mode()` instead of silently changing the
stored value while parsing.

## Runtime application and precedence

Option parsers do not mutate unrelated runtime systems. In particular, parsing
MPD settings does not modify `global_mpd`, and parsing `enable_window_title`
does not inspect `TERM`.

The production startup path is:

1. schema defaults and configuration files populate `Config`;
2. `configuration_apply_runtime()` applies MPD host, port, password, timeout,
   and window-title configuration to their owning runtime systems;
3. `MPD_HOST` and `MPD_PORT` environment variables override configuration;
4. command-line host/port options override both configuration and environment.

Terminal support for window titles is owned by `title.c`; unsupported terminals
do not rewrite `Config.enable_window_title`.

## Adding an option

For an option that matches an existing semantic type, add exactly one entry to
`src/configuration_options.def`. For example:

```c
XX_BOOL(show_example, "yes")
XX_INT_RANGE(example_count, "5", 0, 100)
XX_ENUM(example_mode, enum ExampleMode, "normal", example_mode_parse)
```

Do not separately edit `Configuration`, initialization/destruction,
`ncmpcpp_options`, the generated option count, or an `apply_*` table entry.
Those are all generated.

Then add explicit behavioral tests only when the new option has behavior not
already covered by its generic type. The schema-derived tests automatically
cover descriptor identity, default parsing, lifecycle, and numeric bounds for
supported generic numeric types.

When no existing macro fits, first decide whether the behavior is a reusable
semantic configuration type. Add a new `XX_*` type only when it captures a
real storage/parsing/lifecycle concept. Update every required generation pass
and schema-derived test pass for that type. Do not generalize unique grammars
merely to avoid a small dedicated parser.

## Testing invariants

`tests/c_settings_baseline_test.c` verifies:

- every generated option ID maps to the expected descriptor name, default, and
  `apply_*` function;
- every descriptor slot is populated and option names are unique;
- every declared default parses independently into a fresh configuration;
- a complete default-only read succeeds;
- generated integer and double range boundaries behave as declared and failed
  parses preserve the previous value;
- owning generated types survive cleanup and repeated destruction;
- optional enums, named booleans, regex choices, colors, formats, buffers,
  looks, lists, columns, and compatibility aliases retain their semantics;
- duplicate options are rejected within one read but state does not leak into
  later reads;
- parser/runtime separation is preserved;
- runtime precedence remains configuration, environment, then command line.

The schema is the contract. Adding an existing-type option should make missing
field, descriptor, lifecycle, or numeric-test bookkeeping impossible by
construction rather than by convention.
