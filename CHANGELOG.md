# Changelog

All notable changes to **Tadah.CORE** are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning follows the umbrella project (Tadah.MLIP).

## [Unreleased]

### Added
- `EMBF0` key (`ZERO` | `FREE`, default `ZERO`): the DM_REAM embedding reference (see Tadah.MODELS). STRING key with an explicit blueprint default, so the DM_REAM constructor materialises it into the model Context and every DM_REAM potential is written with an explicit `EMBF0` line. `refit_constrain_f0` description: effective under `EMBF0 FREE` only.
- `number_format.h`: lossless number text. `format_roundtrip()` gives the shortest text that reads back as the identical double (`std::to_chars`); `format_sig()` / `format_sig_chars()` give locale-independent `%.{N}g` (a finite value never renders to text that overflows); `reformat_double_token()` re-renders one token; `to_double()` / `parse_number()` parse like `std::stod`, except that an underflowing literal is a value (the subnormal or ±0) rather than an error; `resolve_pot_digits()` / `pot_digits_warning()` implement the potential-file precision rule below.
- `to_string_lossless()` (`utils.h`) for scalars, strings and ranges. `to_string(x, n)` is for human-facing text only.
- `Context::has_stored()` / `Context::stored_value()` look a key up without inserting its blueprint default.
- `Context::write_potential(os, sig_digits)`, the potential-file writer: the `operator<<` layout, never a NUMERIC line, and every non-integer number of a key with a DOUBLE position rendered with `sig_digits` significant digits; integers, strings, `LIN`/`GEOM`, bools, index patterns and inf/nan are written verbatim.

### Changed
- **`Context` stores values losslessly.** `add()`, `addInternal()` and the derived `RCUT2BMAX`/`RCUTMBMAX`/`RCUTMAX` keys used 18 fixed decimals: values below 5e-19 became 0 and small values lost digits on every add. `get<double>()` now returns the bit-identical value. The `Context` layout changed: rebuild LAMMPS (ML-TADAH embeds CORE) together with `tadah`.
- **`NUMERIC` defaults to 18 (was 12), and in potential and dataset files it means significant digits (`%.{N}g`).** 18 reads back bit-identically. An explicit NUMERIC below 17 is honoured with a WARNING that names the fix (delete the NUMERIC line), and NUMERIC is never written into a potential. Loaded values are never limited by NUMERIC. Other outputs (predictions, analysis tables) keep their meaning but are wider by default.
- Floating-point `std::to_chars` is required (libstdc++ >= 11, libc++ >= 14).

### Fixed
- `is_number()`, `Checkpoint::get_dbl()` and the blueprint validator's long-line fast path accept subnormal values, which the lossless writers produce (`1e-310`). `is_number()` rejected them, so the HPO WARM reader shifted every later dimension of the key; a checkpointed subnormal read back as missing; and the validator fell back to its slow regex path. Overflow is still rejected.
- A potential written with a low explicit NUMERIC (1–5, 10, 11, 15 or 16) rendered the `DBL_MAX` `NSTDEV` sentinel of a constant descriptor column as text that overflows (`1.8e+308`), and the file could not be loaded again. Such a value is now truncated toward zero (`1.79e+308`).

## [1.5.0] — 2026-09-10

### Changed
- Framework version bumped to `1.5.0` (`blueprint/blueprint_tadah.toml`), the single source every module's CMake `project(VERSION)` reads. The release is HPO-led; see the Tadah.HPO changelog for the three breaking changes it carries.

## [1.4.0] — 2026-07-14

### Added
- `EAMFAST` blueprint key (BOOL, explicit `default_value = ["true"]`): toggles the collapsed-EAM fast path in `pair_style tadah` for linear EAM-form models (see Tadah.LAMMPS changelog). `EAMFAST false` restores the bitwise generic kernel.

## [1.3.0-beta.1] — 2026-05-20

First public beta of the Tadah! framework. CORE consolidates the blueprint, configuration, and registry layers and introduces the long-running-job infrastructure (checkpoint + signal handlers) used by HPO.

### Added
- `RCUTENV` blueprint key — envelope cutoff for the new `DM_mBlipEnv` descriptor.
- Load-time data-transform and filter blueprint keys: `LSCALE` / `NO_LSCALE`, `ESHIFT` / `NO_ESHIFT`, `ESHIFT_ATOM`, `ESHIFT_DBATOM`, `EWEIGHT_TEMP`, `EFILTER`, `FFILTER`, `WDBFILE` / `WDBFILE_AUTO`, `ZERO_COM_FORCE`. These are applied during dataset load before training and persisted into `pot.tadah` for prediction round-trip.
- Per-species intercept layout in the blueprint schema (`bias` key now documents the one-hot intercept column behaviour required when `NORM=true` with `MODEL=BF_Linear`).
- Checkpoint and signal-handler infrastructure for long HPO runs.
- Container-style `begin()` / `end()` iteration on `Config`.
- `TADAH_ENABLE_ASAN` CMake option for AddressSanitizer builds.
- Single-source-of-truth versioning: `project(... VERSION ...)` is now wired from `blueprint/blueprint_tadah.toml` via the `tadah_read_version_from_blueprint()` helper in `cmake-tools`.
- **Bound-validation primitives**: blueprint adds `AUDIT` (off / warn / error; default off), `AUDIT_SAMPLE` (NN-sweep cap, default 256; `0` = full DB), `HEALTH_LOG` (per-iter monitor verbosity), and the analysis subcommands `TASKS.analysis.{dataset_stats, suggest_bounds, check_bounds}`. `Registry` exposes `view()` and `names()` so the audit can enumerate registered descriptors without instantiating them.

### Changed
- C++ standard upgraded from C++14 to **C++17**. A C++17 compiler is now required.
- Blueprint key casing: `atom` → `ATOM` (consistency with the rest of the schema; case-sensitive parsers will need to update).
- `TYPEMB` value-type regex generalised to accept the additional spherical-harmonics-order parameter; existing 2-UINT `TYPEMB DM_<name> <mu> <nu> <El1> <El2>` lines still parse, and the new 3-UINT example form `TYPEMB DM_<name> <l> <mu> <nu> <El1> <El2>` is documented.
- `WATOM` blueprint type/handling corrected; bundled `DM_*` examples updated to match.
- `THRESHOLD` lost its top-level default (was `1e-6`); the per-task `[TASKS.data.balance.keys.threshold]` default tightened from `["1e-1", "2e-2", "3e-3"]` to `["1e-4", "1e-5", "1e-6"]`. **This changes `tadah data balance` results when the user does not pass `THRESHOLD` explicitly.**
- `Context` output formatting improvements.

### Fixed
- `Registry` static-initialisation order across translation units — descriptors / cutoffs / models registered in different `.cpp` files no longer race the factory map at first use.

### Internal
- Substantive README rewrite (Session B).
- Community / governance pointer files (CONTRIBUTING / CITATION / SECURITY / CoC).
- CI/CD: lint stage (codespell) and tag-gated release-tarball stage added; `.gitignore` extended.
- Test build routed through `cmake-tools/cmake/TadahTesting.cmake` helpers (`tadah_declare_testing` / `tadah_add_tests`); replaces the hand-rolled `BUILD_TESTING` setup. Fixes a silent test-suite-disable on a bare reconfigure where the previous code captured `BUILD_TESTING` before the cache entry existed and FORCE-restored an empty value.

## [1.2.0] and earlier (condensed)

The `v1.0.0` → `v1.1.1` releases were the project's "internal playground" era. CORE was bootstrapped as the central blueprint + config + registry layer for the Tadah! framework, with the blueprint TOML schema iterating ad hoc across releases (typo fixes, type corrections, and key additions for descriptor and cutoff parameters). The blueprint TOML version was rolled to `1.2.0` during this period but no `v1.2.0` git tag was ever pushed; consider any release before `1.3.0-beta.1` undocumented in the modern Keep-a-Changelog sense.
