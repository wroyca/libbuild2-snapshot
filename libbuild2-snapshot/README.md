# libbuild2-snapshot - A C++ library

The `libbuild2-snapshot` C++ library provides <SUMMARY-OF-FUNCTIONALITY>.


## Usage

To start using `libbuild2-snapshot` in your project, add the following `depends`
value to your `manifest`, adjusting the version constraint as appropriate:

```
depends: libbuild2-snapshot ^<VERSION>
```

Then import the library in your `buildfile`:

```
import libs = libbuild2-snapshot%lib{<TARGET>}
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
[bool] config.libbuild2_snapshot.<VARIABLE> ?= false
```

<DESCRIPTION-OF-CONFIG-VARIABLES>
