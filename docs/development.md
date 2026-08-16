# Development

## Requirements

- A C compiler (`cc`, `gcc` or `clang`)
- `make`
- `git`, for the test-runner submodule
- `doxygen` and `graphviz`, only for `make docs`

## Getting the source

The test suite lives in a submodule, so clone recursively:

```sh
git clone --recurse-submodules https://github.com/jonruttan/microminesweeper.git
```

If you already cloned without it:

```sh
make submodule
```

Without the submodule, `make test` stops with a message telling you to run
exactly that.

## Targets

| Target | Effect |
| --- | --- |
| `all` | Build `minesweeper` (default) |
| `check` | Build, then run the suite |
| `test` | Run the suite |
| `sanitize` | Run the suite under ASan and UBSan |
| `docs` | Build the doxygen reference into `build/doxygen/html` |
| `verify-runner` | Check the submodule matches `RUNNER_VERSION` |
| `submodule` | Check out the test-runner submodule |
| `run` | Play; honours `LEVEL` |
| `clean` | Remove build output |

The Makefile documents itself in a literate header comment, in the same style
as `test-runner.sh`.

## Testing

```sh
make test
```

Tests live in `tests/` and must be named `*.spec.c` for `test-runner.sh` to
discover them. The suite includes `minesweeper.c` directly rather than linking
against it:

```c
#include "test-runner.h"
#include "../minesweeper.c"
```

The runner compiles with `-DTESTS`, which excludes `main()` and the level table
so the test binary can supply its own entry point.

### Writing a test

A test is a function returning `char *` — `NULL` on success, or the message of
the failing assertion:

```c
static char *test_xytoi(void)
{
	_it_should("return 255 for (f, f)", 255 == xytoi(MASK, MASK));

	return NULL;
}
```

Register it in `run_tests()`:

```c
_run_test(test_xytoi);
```

Useful macros, all from `test-runner.h`:

| Macro | Effect |
| --- | --- |
| `_it_should(msg, expr)` | Assert; returns `msg` on failure |
| `_run_test(fn)` | Run a test |
| `_xrun_test(fn)` | Skip a test, counted as skipped |
| `_xit_should(msg, expr)` | Skip an assertion |
| `_mark_incomplete()` | Flag a test as unfinished |

A test that runs no assertions is reported as **empty**, which is a warning
rather than a pass. That is deliberate — an empty test is worse than no test,
because it looks like coverage.

### Test conventions

- Call `init()` at the top of a test rather than relying on state from an
  earlier one. Globals persist across tests, and `init()` resets `board`,
  `score` and `stack_p`.
- Verify against something independent where you can. `count_mines()` in the
  spec walks the board directly instead of going through `box()`, so `box()` is
  checked against a second implementation rather than against itself.
- Watch the width of your own counters. A full board is 256 cells, which does
  not fit the `uint8` the rest of the program uses — `count_cells()` returns
  `int` for exactly that reason.

### Sanitizers

```sh
make sanitize
```

Runs the suite under AddressSanitizer and UndefinedBehaviorSanitizer. Worth
doing after anything that touches indexing: the code deliberately relies on
`uint8` wraparound, and the line between that and a genuine out-of-bounds
access is thin.

## The test-runner pin

The submodule is pinned to a tag, and the Makefile declares which one:

```make
RUNNER_VERSION := 1.6.1
```

`make verify-runner` compares that against `TEST_RUNNER_VERSION` in the
submodule header and fails with both values if they differ. `make test` depends
on it, and CI runs it as its own step, so the two cannot drift apart unnoticed.

### Bumping it

```sh
git -C test-runner fetch --tags
git -C test-runner checkout v1.7.0
```

Then update `RUNNER_VERSION` in the Makefile to match, and:

```sh
make test
git add test-runner Makefile
```

Doing only one half fails the build with a message naming both versions. That
is the point — a submodule bump that silently changes test behaviour is the
failure mode this guards against.

Check the runner's own changelog before bumping. The v1.6.1 upgrade changed
spec discovery from `*-test.c` to `*.spec.c` and started propagating the exit
status, both of which needed changes here.

## Continuous integration

`.github/workflows/ci.yml` runs on push to `main`, on pull requests, and on
demand.

| Job | What it does |
| --- | --- |
| `test` | `verify-runner`, `make check`, `make sanitize` across ubuntu and macos on gcc and clang |
| `docs` | `make docs`, uploading the HTML as an artifact |

The suite's exit status only became meaningful in test-runner v1.6.1; before
that a failing test still exited 0 and CI would have passed regardless.

The `docs` job builds with `WARN_AS_ERROR = FAIL_ON_WARNINGS`, so an
undocumented parameter or a broken `@ref` fails the build in the same way a
failing test does.

## Documentation

```sh
make docs
```

Output lands in `build/doxygen/html`. The prose under `docs/` is part of the
generated reference, and `README.md` is its front page, so the reference and
the hand-written docs are one artifact rather than two.

Publishing to GitHub Pages is not set up. To enable it, turn on Pages for the
repository with **GitHub Actions** as the source, then add a deploy step to the
`docs` job.

## Code style

`.editorconfig` is authoritative: tabs for `*.c`, `*.h`, `*.sh` and the
Makefile; two spaces elsewhere; LF endings; final newline.

Beyond that, follow what is there:

- Declarations at the top of a block.
- Braces on every `if` body, even one-liners.
- A blank line before `return`.
- Comments explain *why*. The code is short enough that what it does is
  legible; what is not legible is which constraint forced it.

Doc comments are doxygen-style `/** ... */`. Every function gets `@brief`,
`@param` for each parameter, and `@return` unless it returns `void` — the docs
build fails otherwise.

## Commit messages

See `test-runner/docs/commit_guidelines.md`, which this project follows:
`<type>(<scope>): <subject>`, imperative mood, subject under 50 characters,
body wrapped at 72.

```
fix(minesweeper): stop mark draining the score
```

Types in use here: `feat`, `fix`, `refactor`, `style`, `docs`, `test`, `build`,
`ci`, `chore`.
