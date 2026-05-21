[![pipeline](https://git.ecdf.ed.ac.uk/tadah/core/badges/develop/pipeline.svg)](https://git.ecdf.ed.ac.uk/tadah/core/-/commits/develop)
[![coverage](https://git.ecdf.ed.ac.uk/tadah/core/badges/develop/coverage.svg)](https://git.ecdf.ed.ac.uk/tadah/core/-/commits/develop)
[![license](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)
[![version](https://img.shields.io/badge/version-1.3.0--beta.1-blue.svg)](https://git.ecdf.ed.ac.uk/tadah/core/-/tags)

# CORE

Foundation library of the [Tadah!](https://tadah.readthedocs.io/) MLIP
toolchain. Provides the substrate every other Tadah! module is built
on:

- **Context** — the typed key/value store backing every configuration
  file, command-line flag, and persisted potential file.
- **Blueprint parser & validator** — drives the auto-generated CLI
  surface and the documented configuration grammar.
- **Periodic table & element metadata** — atomic numbers, masses, and
  symbol look-ups.
- **Checkpoint manager & signal handler** — resumable runs for any
  long-running engine.
- **Logger, math helpers, LAPACK shim, normaliser** — utilities every
  module re-uses.

## You don't need to use this directly

CORE is an internal module of the Tadah! MLIP toolchain. It is
fetched automatically (via CMake `FetchContent`) by every Tadah!
consumer (MODELS, MLIP, MD, HPO, Tadah.LAMMPS, Tadah.MLIP). End users
install the umbrella project
[Tadah!MLIP](https://git.ecdf.ed.ac.uk/tadah/tadah.mlip) — see the
[installation guide](https://tadah.readthedocs.io/en/latest/installation.html).

## Contributing

Build instructions, dependencies, layout, and the per-module dev
workflow live in [CONTRIBUTING.md](CONTRIBUTING.md). The full
multi-repo contributor guide is on the umbrella project.

## License

GPL-3.0; see [LICENSE](LICENSE).

## Reporting issues

Bugs and feature requests:
[CORE issues](https://git.ecdf.ed.ac.uk/tadah/core/-/issues).
For end-user questions please use the umbrella project
[Tadah!MLIP](https://git.ecdf.ed.ac.uk/tadah/tadah.mlip).
