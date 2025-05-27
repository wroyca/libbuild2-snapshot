# libbuild2-snapshot-tests - A C++ library

The `libbuild2-snapshot-tests` C++ library provides <SUMMARY-OF-FUNCTIONALITY>.


## Usage

To start using `libbuild2-snapshot-tests` in your project, add the following `depends`
value to your `manifest`, adjusting the version constraint as appropriate:

```
depends: libbuild2-snapshot-tests ^<VERSION>
```

Then import the library in your `buildfile`:

```
import libs = libbuild2-snapshot-tests%lib{<TARGET>}
```


## Importable targets

This package provides the following importable targets:

```
lib{<TARGET>}
```

<DESCRIPTION-OF-IMPORTABLE-TARGETS>


## Configuration variables

This package provides the following configuration variables:

```
[bool] config.libbuild2_snapshot_tests.<VARIABLE> ?= false
```

<DESCRIPTION-OF-CONFIG-VARIABLES>
