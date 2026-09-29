# Changelog

All notable changes to **Tadah.CORE** are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning follows the umbrella project (Tadah.MLIP).

## [Unreleased]

## [1.6.0] — 2026-09-25

### Added
- Strict index parsing for indexed configuration keys.
- Blueprint field `allowed_values` (`BlueprintKey::allowed_values`, `isAllowedValue()`): the only values of a single-string key, matched without regard to case. `BlueprintParser::buildKey` requires a non-empty list of non-empty strings on a `STRING` key with `value_N = [1]`, and the key's default and examples among them. `CoreUserConfigParser` rejects another value at the line that sets it, naming the file (`Invalid value for key EMBF0 on line 3 of pot.tadah: 'ZEROO' (allowed: ZERO, FREE)`; lines that come from no file give the line alone), and `validateWithBlueprint` (the CLI and task path) refuses it with an `Allowed values` log line; `validateSchemaOnly()` is the type check alone. `EMBF0` is the first key that uses it (`value_format` `ZERO|FREE`). Test: `test_blueprint_allowed_values.cpp`.
- Deprecated configuration keys: the blueprint field `deprecated` holds the reason and the replacement. `CoreUserConfigParser` accepts a deprecated key, checks its value against the key's format, writes one notice per key and process to standard error (`WARNING: Deprecated key MBLOCK is ignored: …`, through `report_ignored_key_once`, however many lines, `CONFIG` files or parses read it) and does not store it. The notice bypasses the `Logger`, so neither the log level nor a `VERBOSE` line suppresses it, and `pair_style tadah` in LAMMPS (which leaves the `Logger` at `ERROR`) shows it too. `BlueprintParser::buildKey` rejects a deprecated key with a `default_value` (`Context::contains()` would insert it, and it would reappear in every written potential), a CLI flag or examples (the configuration-keys page and the invalid-value error would show how to use it; `examples = []` overrides those of an inherited `TYPES` table), and `findClosestKey` never suggests one.
- `EMBF0` key (`ZERO` | `FREE`, default `ZERO`): the DM_REAM embedding reference (see Tadah.MODELS). It takes `ZERO` or `FREE` only, in any case (`allowed_values`, below); any other value is an error at its line. STRING key with an explicit blueprint default, so the DM_REAM constructor materialises it into the model Context and every DM_REAM potential is written with an explicit `EMBF0` line. `refit_constrain_f0` description: effective under `EMBF0 FREE` only.
- **BREAKING — `RHOFORM` key (`SIGNED` | `NONNEG` | `MONOTONE`): the DM_REAM density form (see Tadah.MODELS), mandatory and without a default.** A STRING key deliberately given no `default_value`: every DM_REAM potential and configuration must declare it, and nothing supplies it implicitly. Readers go through `Context::has_stored()` / `stored_value()` only (a `Context::get` on the absent key would log a spurious ERROR before throwing). Its `allowed_values` are `SIGNED`, `NONNEG` and `MONOTONE`, in any case (`value_format` `SIGNED|NONNEG|MONOTONE`): another value is an error at its line, naming the file, as for `EMBF0` (`Invalid value for key RHOFORM on line 24 of pot.tadah: 'SQUARE' (allowed: SIGNED, NONNEG, MONOTONE)`), also in a potential without a DM_REAM block. The `AMPGRIDMB` description says what the amplitudes multiply under each form, and the `refit` task text lists `RHOFORM` as a required refit key. Test: `test_blueprint_allowed_values.cpp`. **Migration:** add `RHOFORM SIGNED` to a DM_REAM potential or configuration written before tadah 1.6.0 to use it unchanged. Binaries older than the key refuse a potential that carries it (`Invalid key: RHOFORM`).
- `number_format.h`: lossless number text. `format_roundtrip()` gives the shortest text that reads back as the identical double (`std::to_chars`); `format_sig()` / `format_sig_chars()` give locale-independent `%.{N}g` (a finite value never renders to text that overflows); `reformat_double_token()` re-renders one token; `to_double()` / `parse_number()` parse like `std::stod`, except that an underflowing literal is a value (the subnormal or ±0) rather than an error; `resolve_pot_digits()` / `pot_digits_warning()` implement the potential-file precision rule below.
- `to_string_lossless()` (`utils.h`) for scalars, strings and ranges. `to_string(x, n)` is for human-facing text only.
- `Context::has_stored()` / `Context::stored_value()` look a key up without inserting its blueprint default.
- `IUserConfigParser::parse(lines, first_line, source)` (implemented by `CoreUserConfigParser`): parses lines taken out of a larger source, such as the `TASK` section of a task file, where `lines[i]` is line `first_line + i` of `source`, so that an error that names a line names the line of the file, and the file (`Invalid value for key EMBF0 on line 6 of run.task: 'frei' (allowed: ZERO, FREE)`). MLIP's task-file parser uses it. A class that implements `IUserConfigParser` must implement it too (pure virtual). Test: `test_blueprint_allowed_values.cpp`.
- `Context::write_potential(os, sig_digits)`, the potential-file writer: the `operator<<` layout, never a NUMERIC line, and every non-integer number of a key with a DOUBLE position rendered with `sig_digits` significant digits; integers, strings, `LIN`/`GEOM`, bools, index patterns and inf/nan are written verbatim.
- `key_notice.h`: the notice of a key that is accepted but has no effect. `report_ignored_key(subject, reason)` writes `WARNING: <subject> is ignored: <reason>` to standard error in one insertion (not a `Logger` message, so neither the log level nor `VERBOSE` hides it, in LAMMPS either); `report_ignored_key_once(key, subject, reason)` writes it at most once per key and process (thread-safe), and `ignored_key_reported(key)` says whether it has. The deprecated-key notice of `CoreUserConfigParser` goes through `report_ignored_key_once` (the text is unchanged; once per key and process, as for every ignored key, by the owner's decision of 2026-09-29: parsing a potential with `MBLOCK 64` and `EMBF0 FREE` twice in one process prints each notice once); MODELS reports `EMBF0` and `RHOFORM` without a DM_REAM block through `report_ignored_key_once`, and the `EMBF0` and `RHOFORM` descriptions say so (they said a WARNING). Test: `test_key_notice.cpp`.
- `parse_decimal_normal()` (`number_format.h`): the number grammar of the hpotarget file, `parse_decimal_finite()` whose value is also zero or a normal double (a subnormal such as `1e-310`, and a nonzero literal that underflows to 0 such as `1e-400`, are refused; the tests are on the bits). HPO's `parse_number` reads hpotarget numbers with the same grammar, and MLIP's `tadah refit --retarget` reads its `--pct` and `--set` numbers with it, so every number it writes into an hpotarget is one `tadah hpo` reads back. Test: `test_number_format.cpp`.
- `RETARGET_RHOFORM` key, the `--rhoform SIGNED|NONNEG|MONOTONE` option of `tadah refit --retarget` (`allowed_values`; needs `--retarget`): the density form of the potential an hpotarget searches around, for a file written without the `RHOFORM` header line (see Tadah.MLIP). The `refit` help and the `retarget` key describe it, the `RHOFORM`/`EMBF0` rules of the `--edit` and `--retarget` configurations, and the checks on `--pct` and `--set` (the hpotarget number grammar, an `OPTIM` line on each `--pct KEY`, finite `--set` bounds within the density form's rule). The `analysis check_bounds` help says how each corner is built and that a DM_REAM seed must declare `RHOFORM`. Test: `test_blueprint_allowed_values.cpp`.

### Changed
- Framework version bumped to `1.6.0` (`blueprint/blueprint_tadah.toml`), the single source every module's CMake `project(VERSION)` reads. The release carries exact number handling in potential and dataset files, a non-zero exit when the best potential cannot be saved, the DM_REAM zero-referenced embedding (`EMBF0 ZERO`) as the new default (**BREAKING**: add `EMBF0 FREE` to a 1.5.0 DM_REAM potential to reproduce its energies), and the mandatory DM_REAM density form `RHOFORM SIGNED | NONNEG | MONOTONE`, with no default (**BREAKING**: add `RHOFORM SIGNED` to a DM_REAM potential or configuration written before tadah 1.6.0, bit-identical); see the module changelogs.
- The blueprint is a CMake configure dependency: editing it (e.g. a version bump) re-runs the configure step, so the embedded blueprint and `tadah --version` can no longer go stale.
- The `0.0.0` fallback for a missing version reader is gone: an out-of-date cmake-tools is a configure error.
- Tests take Catch2 and its test discovery from cmake-tools (`tadah_setup_tests()`, `include(Catch)`). The vendored `tests/catch2/catch.hpp`, `tests/Catch.cmake` and `tests/CatchAddTests.cmake` are removed, and so is the test-local `Catch2` target, whose include directory did not exist.
- cmake-tools is fetched at `v1-dev` instead of `main`.
- The `EAMFAST` description says that each collapse table is certified when the potential is loaded (relative tolerance 1e-9 for the value and the derivative, the error of each table segment bounded from 31 check points), that the fast path evaluates the analytic function below the certified range, that a single failing segment (a basis knot, a switching-cutoff junction) ends that range, and what the LAMMPS log line reports. It claimed about 1e-9 relative accuracy at every distance and "typically 5-10x faster force calls"; it now gives measured speed-ups (1.6x to 4.5x in 250-atom MD runs, growing with the cost of the descriptors).
- The blueprint text of the `tadah refit` `OPTIMIZER` block (the `OPTIMIZER` key on the configuration-keys page and the stage-pipeline paragraph of `tadah refit --help`) lists the keys each stage reads and says that `ALGO` is required and any other key is an error, as `tadah refit` now enforces. It said the block took the stopping criteria of the `tadah hpo` `OPTIMIZER` block, `FTOL_ABS` and `XTOL_ABS` included.
- The same text says that `LIB` is required wherever the block runs an optimiser, naming the library of its `ALGO` (`NLOPT`, `DLIB` or `CERES`), that `LIB` with `ALGO NONE` is an error, and that `MAXEVAL` and `MAXITER` are integers from 1 to 2147483647, as `tadah refit` now enforces. It said `LIB` was needed only where the `ALGO` did not name the library.
- The same text says that `LIB` with `ALGO UNIFORM` is an error, as with `NONE`, and so is a stopping criterion in a block with `ALGO NONE` or `UNIFORM` unless another stage reads it, and that `MAXEVAL`/`MAXITER` counts the objective evaluations of the NLopt and dlib algorithms exactly and the iterations of Ceres' `LM` and `DOGLEG`, as `tadah refit` now does. It said `UNIFORM` did not check `LIB`.
- `Mat6R3C::row()` and `Matrix3d::row()`/`col()` return const copies. Assigning to one (`m.row(n) = v`, `m.row(n) += v`, `m.row(n)[k] = x`) compiled and changed the temporary copy instead of the matrix; it is now a compile error. `Mat6R3C::set_row()` writes a row. The MLIP `DIMER` descriptors were computed from a zero displacement matrix this way (fixed in MLIP 1.6.0). Test: compile-time checks in `test_maths.cpp`.
- The `DIMER` key's description says that a `DIMER` potential is evaluated with every atom as Z=1 (`tadah train`, `tadah predict` and `pair_style tadah`): every pair gets the weight of the Z=1 (H-H) pair, which is 1 unless `WATOM` sets one for H, not the `WATOM` weight of the potential's element. Its `TYPE2B`/`TYPEMB` descriptors must therefore be declared for `* *` element pairs, and a potential naming specific element pairs is refused when it is loaded (MODELS, MLIP and Tadah.LAMMPS 1.6.0).
- The README badges and links and the pointers to the umbrella's community files (`CITATION.md`, `CONTRIBUTING.md`, …) point to the `v1-dev` line: GitLab `v1-dev`, `https://tadah.readthedocs.io/v1-dev/`. They pointed to `develop`.
- **`Context` stores values losslessly.** `add()`, `addInternal()` and the derived `RCUT2BMAX`/`RCUTMBMAX`/`RCUTMAX` keys used 18 fixed decimals: values below 5e-19 became 0 and small values lost digits on every add. `get<double>()` now returns the bit-identical value. The `Context` layout changed: rebuild LAMMPS (ML-TADAH embeds CORE) together with `tadah`.
- **`NUMERIC` defaults to 18 (was 12), and in potential and dataset files it means significant digits (`%.{N}g`).** 18 reads back bit-identically. An explicit NUMERIC below 17 is honoured with a WARNING that names the fix (delete the NUMERIC line), and NUMERIC is never written into a potential. Loaded values are never limited by NUMERIC. Other outputs (predictions, analysis tables) keep their meaning but are wider by default.
- Floating-point `std::to_chars` is required (libstdc++ >= 11, libc++ >= 14).

### Deprecated
- `MPIWPCKG`, `MBLOCK` and `NBLOCK`, the keys of the legacy MPI training build removed in 1.6.0. Configuration and potential files that contain them still load, with a warning on standard error, and the values are ignored. The keys lost their default values, so a Context never holds them and a newly written potential never contains them.

### Removed
- The `TADAH_BUILD_MPI` branch of `Logger::log` (an `MPI_Comm_rank` prefix). The flag was defined for MLIP and HPO but not for CORE and MODELS, so the two sides compiled different `Logger::log` bodies (an ODR violation).

### Fixed
- **199 of the 249 unit tests never ran under CTest.** The test name `histogram fixed-bin counts samples in [low, high)` has an unbalanced `[`, and the vendored Catch discovery script split the test listing through a CMake list, so that name and the 198 listed after it became one CTest entry. It matched no test case and still passed. With cmake-tools' discovery every test case is registered exactly once and an entry that matches nothing fails (`--warn NoTests`); all 249 run and pass. The explicit `core_pc_dataset_indices` entry, a workaround for this defect, is removed (its two cases are discovered).
- A `CoreUserConfigParser::parse` that threw (an invalid key or value, a missing `CONFIG` file) left the parser's recursion depth raised, so the next parse on that thread was treated as nested and did not reset its `CONFIG` chain. An RAII scope restores the depth on every exit.
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
