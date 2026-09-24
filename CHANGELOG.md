# Changelog

All notable changes to **Tadah.CORE** are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning follows the umbrella project (Tadah.MLIP).

## [Unreleased]

## [1.5.1] — 2026-09-23

### Changed
- Framework version bumped to `1.5.1` (`blueprint/blueprint_tadah.toml`).
- The blueprint is a CMake configure dependency: editing it (e.g. a version bump) re-runs the configure step, so the embedded blueprint and `tadah --version` can no longer go stale.
- The `0.0.0` fallback for a missing version reader is gone: an out-of-date cmake-tools is a configure error.
- Tests take Catch2 and its test discovery from cmake-tools (`tadah_setup_tests()`, `include(Catch)`). The vendored `tests/catch2/catch.hpp`, `tests/Catch.cmake` and `tests/CatchAddTests.cmake` are removed, and so is the test-local `Catch2` target, whose include directory did not exist.
- cmake-tools is fetched at `v1-dev` instead of `main`.
- The blueprint text of the `tadah refit` `OPTIMIZER` block (the `OPTIMIZER` key on the configuration-keys page and the stage-pipeline paragraph of `tadah refit --help`) lists the keys each stage reads and says that `ALGO` is required and any other key is an error, as `tadah refit` now enforces. It said the block took the stopping criteria of the `tadah hpo` `OPTIMIZER` block, `FTOL_ABS` and `XTOL_ABS` included.

### Added
- Strict index parsing for indexed configuration keys.
- Deprecated configuration keys: the blueprint field `deprecated` holds the reason and the replacement. `CoreUserConfigParser` accepts a deprecated key, checks its value against the key's format, writes one notice per key and parse to standard error (`WARNING: Deprecated key MBLOCK is ignored: …`; a `CONFIG` include chain is one parse) and does not store it. The notice bypasses the `Logger`, so neither the log level nor a `VERBOSE` line suppresses it, and `pair_style tadah` in LAMMPS (which leaves the `Logger` at `ERROR`) shows it too. `BlueprintParser::buildKey` rejects a deprecated key with a `default_value` (`Context::contains()` would insert it, and it would reappear in every written potential), a CLI flag or examples (the configuration-keys page and the invalid-value error would show how to use it; `examples = []` overrides those of an inherited `TYPES` table), and `findClosestKey` never suggests one.

### Deprecated
- `MPIWPCKG`, `MBLOCK` and `NBLOCK`, the keys of the legacy MPI training build removed in 1.5.1. Configuration and potential files that contain them still load, with a warning on standard error, and the values are ignored. The keys lost their default values, so a Context never holds them and a newly written potential never contains them.

### Removed
- The `TADAH_BUILD_MPI` branch of `Logger::log` (an `MPI_Comm_rank` prefix). The flag was defined for MLIP and HPO but not for CORE and MODELS, so the two sides compiled different `Logger::log` bodies (an ODR violation).

### Fixed
- **199 of the 249 unit tests never ran under CTest.** The test name `histogram fixed-bin counts samples in [low, high)` has an unbalanced `[`, and the vendored Catch discovery script split the test listing through a CMake list, so that name and the 198 listed after it became one CTest entry. It matched no test case and still passed. With cmake-tools' discovery every test case is registered exactly once and an entry that matches nothing fails (`--warn NoTests`); all 249 run and pass. The explicit `core_pc_dataset_indices` entry, a workaround for this defect, is removed (its two cases are discovered).
- A `CoreUserConfigParser::parse` that threw (an invalid key or value, a missing `CONFIG` file) left the parser's recursion depth raised, so the next parse on that thread was treated as nested and did not reset its `CONFIG` chain. An RAII scope restores the depth on every exit.

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
