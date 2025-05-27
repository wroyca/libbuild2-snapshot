# libbuild2-snapshot - <SUMMARY>

`libbuild2-snapshot` is a <SUMMARY-OF-FUNCTIONALITY>.

This file contains setup instructions and other details that are more
appropriate for development rather than consumption. If you want to use
`libbuild2-snapshot` in your `build2`-based project, then instead see the
accompanying package [`README.md`](<PACKAGE>/README.md) file.

The development setup for `libbuild2-snapshot` uses the standard `bdep`-based
workflow:

```
git clone .../libbuild2-snapshot.git
cd libbuild2-snapshot

bdep init --empty

bdep config create @module ../libbuild2-snapshot-build/module/ --type build2 cc config.config.load=~build2
bdep config create @target ../libbuild2-snapshot-build/target/ cc config.cxx=g++

bdep init @module -d libbuild2-snapshot/
bdep init @target -d libbuild2-snapshot-tests/
```

Once this is done, we can develop using bdep or the build system as usual:

```
bdep test                          # run tests
b libbuild2-snapshot/              # update the module directly
b test: libbuild2-snapshot-tests/  # run only external tests
```

We can also CI our module, manage releases, and publish it to the package
repository:

```
bdep ci @module  # submits only the module (which pulls in the tests)

bdep release     # releases both the module and the tests

bdep publish     # submits both the module and the tests
```
