[![pipeline](https://git.ecdf.ed.ac.uk/tadah/core/badges/develop/pipeline.svg)](https://git.ecdf.ed.ac.uk/tadah/core/-/commits/develop)
[![coverage](https://git.ecdf.ed.ac.uk/tadah/core/badges/develop/coverage.svg)](https://git.ecdf.ed.ac.uk/tadah/core/-/commits/develop)
[![license](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)
[![version](https://img.shields.io/badge/version-1.3.0--beta.1-blue.svg)](https://git.ecdf.ed.ac.uk/tadah/core/-/tags)

# Tadah!CORE

Foundation library for the [Tadah!](https://tadah.readthedocs.io/) MLIP
toolchain. Provides the substrate every other Tadah! module is built
on:

- **Context** — the typed key/value store backing every configuration
  file, command-line flag, and persisted potential file.
- **Blueprint parser & validator** — drives the auto-generated CLI
  surface and the documented configuration grammar.
- **Periodic table & element metadata** — atomic numbers, masses, and
  symbol look-ups.
- **Checkpoint manager & signal handler** — resumable runs (used by
  `tadah hpo`'s `BASINS` strategy and any long-running engine).
- **Logger, math helpers, LAPACK shim, normaliser** — utilities every
  module re-uses.

CORE has no scientific dependencies of its own; it is intentionally a
small substrate library.

## When you use this directly

You should never need to clone or build CORE in isolation: it is
fetched automatically by every Tadah! consumer (MODELS, MLIP, MD, HPO,
Tadah.LAMMPS, Tadah.MLIP) via CMake's `FetchContent`. The only reason
to interact with CORE directly is to develop on it — see the
[installation docs](https://tadah.readthedocs.io/en/latest/installation.html)
for the multi-repo Develop build.

## Build & test in isolation

CORE depends only on the [cmake-tools](https://git.ecdf.ed.ac.uk/tadah/cmake-tools)
helper repo (also fetched automatically) and a C++17 compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build/tests --output-on-failure
```

In-source builds are blocked deliberately; always pick a separate
build directory.

## Layout

| Path | Purpose |
|---|---|
| `include/tadah/core/` | Public headers (`context.h`, `checkpoint.h`, `logger.h`, …). |
| `src/`                | Implementation translation units. |
| `blueprint/`          | `blueprint_tadah.toml` — the single source of truth for config keys, CLI flags, and version. The umbrella docs render it via `read_toml.py`. |
| `tests/`              | Unit tests. |
| `external/`           | Third-party headers vendored at fixed versions. |

## Versioning

The Tadah! version is read from `blueprint/blueprint_tadah.toml`
(`[TADAH] version = "..."`). All Tadah! consumers — including the
documentation build — pull their version from this file, so a single
edit propagates project-wide.

## License

GPL-3.0; see [LICENSE](LICENSE).

## Reporting issues

Bugs and feature requests:
[Tadah!CORE issues on GitLab](https://git.ecdf.ed.ac.uk/tadah/core/-/issues).
For user-facing questions, please use the umbrella project
([Tadah!MLIP](https://git.ecdf.ed.ac.uk/tadah/tadah.mlip)) instead.
