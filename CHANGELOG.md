# Changelog

All notable changes to **Tadah.CORE** are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning follows the umbrella project (Tadah.MLIP).

## [Unreleased]

Nothing yet.

## [1.3.0-beta.1] — TBD

First public beta of the Tadah! framework. CORE consolidates the blueprint, configuration, and registry layers and introduces the long-running-job infrastructure (checkpoint + signal handlers) used by HPO.

### Added
- `RCUTENV` blueprint key — envelope cutoff for the new `DM_mBlipEnv` descriptor.
- Load-time data-transform and filter blueprint keys: `LSCALE` / `NO_LSCALE`, `ESHIFT` / `NO_ESHIFT`, `ESHIFT_ATOM`, `ESHIFT_DBATOM`, `EWEIGHT_TEMP`, `EFILTER`, `FFILTER`, `WDBFILE` / `WDBFILE_AUTO`, `ZERO_COM_FORCE`. These are applied during dataset load before training and persisted into `pot.tadah` for prediction round-trip.
- Per-species intercept layout in the blueprint schema (`bias` key now documents the one-hot intercept column behaviour required when `NORM=true` with `MODEL=BF_Linear`).
- Checkpoint and signal-handler infrastructure for long HPO runs.
- Container-style `begin()` / `end()` iteration on `Config`.
- `TADAH_ENABLE_ASAN` CMake option for AddressSanitizer builds.
- Single-source-of-truth versioning: `project(... VERSION ...)` is now wired from `blueprint/blueprint_tadah.toml` via the `tadah_read_version_from_blueprint()` helper in `cmake-tools`.

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

## [1.2.0] and earlier (condensed)

The `v1.0.0` → `v1.1.1` releases were the project's "internal playground" era. CORE was bootstrapped as the central blueprint + config + registry layer for the Tadah! framework, with the blueprint TOML schema iterating ad hoc across releases (typo fixes, type corrections, and key additions for descriptor and cutoff parameters). The blueprint TOML version was rolled to `1.2.0` during this period but no `v1.2.0` git tag was ever pushed; consider any release before `1.3.0-beta.1` undocumented in the modern Keep-a-Changelog sense.
