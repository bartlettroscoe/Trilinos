# Building Trilinos on ATDM Machines

This directory contains a set of configuration files and scripts for setting up the env and configuring, building, and testing Trilinos for ATDM on a number of ATDM-related platforms.

## Local development workflow

With:

* `<TrilinosBaseDir>`: abs path to the local Trilinos git repo
* `<build_dir>`: abs path to the build directory

the basic workflow for configuring, building, and testing Trilinos is given below.

### 1) Set up the env

First set the `JOB_NAME` env var such as:

```
export JOB_NAME=Drekar_opt_gnu_openmp
```

This is a free-form field and certain keywords are searched for in this name to specify what gets built with the components:

* Compiler: `gnu`, `intel`, or `cuda`
* Optimized or debug: `opt` or `debug`
* Backend: nothing '' (serial), `openmp`, or `cuda`

(Note you only need to specify `cuda` once and these keywords can be listed in any order in the `JOB_NAME`.)

Then source the script:

```
source <TrilinosBaseDir>/cmake/std/atdm/environment.sh
```

The above script will get the machine name from `hostname` and then load the right env for that machine. The loaded env vars will have the prefix `ATDM_` (i.e. so as to not pollute the global env namespace).

### 2) Configure Trilinos

Once the env is loaded, then can configure Trilinos with:

```
cd <build_dir>/
cmake \
  -DTrilinos_OPTIONS_FILE:STRING=cmake/std/atdm/ATDMConfig.cmake \
  [other cache vars] \
  <TrilinosBaseDir>
```

selecting any packages or tests that one wants to enable.  Including the file `ATDMConfig.cmake` picks up the right compilers, MPI, TPL locations, and disables any TPLs or packages not needed by any of the ATDM customers configuring Trilinos.  To make configuration easier for local development, one use the configure script:

```
cd <build_dir>/
ln -s <TrilinosBaseDir>/cmake/std/atdm/do-configure .
./do-configure [other cache vars]
```

### 3) Build enabled Trilinos packages

```
cd <build_dir>/
make -j16
```

(NOTE: One may reserve and then move onto the compute nodes on this cluster and then build in parallel from there to avoid overloading the host nodes.)


### 4) Running Trilinos exectuables and tests

On some ATDM machines, one needs to move onto the compute nodes in order to run executables and tests (especially on machines that have a special backend like `cuda`).

To do this, run:

```
cd <build_dir>/
source <TrilinosBaseDir>/cmake/atdm/get_on_compute_nodes.sh [number_of_nodes]
ctest -j16
```

## Setup for automated builds of Trilinos

Setting up for automated builds of Trilinos for submitting to the Trilinos CDash site involves the key files:

* `<TrilinosBaseDir>/cmake/std/atdm/environment.sh`
* `<TrilinosBaseDir>/cmake/std/atdm/ATDMConfig.cmake`

The file `environment.sh` must be sourced in the other shell script that calls the `ctest -S <script>` command.  The file `ATDMConfig.cmake` is passing as part of the configure options for the automated build for the platform.  That last piece of information to set up an automated build is to specify what packages should be enabled and (additional) packages to be disabled.  ToDo: Specify how that is done!
