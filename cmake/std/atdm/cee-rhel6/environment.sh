################################################################################
#
# Set up env on a CEE RHEL6 system for ATMD builds of Trilinos
#
# This source script gets the settings from the JOB_NAME var.
#
################################################################################

# NOTE: The custom_builds.sh script in this directory sets the exact compilers
# used so no need for dealing with different varients of compilers here.

if [[ "$ATDM_CONFIG_COMPILER" == "DEFAULT" ]] ; then
  # No supported compiler was selected in custom_bulids.sh so abort!
  return
fi

if [[ "$ATDM_CONFIG_KOKKOS_ARCH" == "DEFAULT" ]] ; then
  # Use no KOKKOS_ARCH so it will not appear in the tweaks file
  unset ATDM_CONFIG_KOKKOS_ARCH
else
  echo
  echo "***"
  echo "*** ERROR: Specifying KOKKOS_ARCH is not supported on CEE RHEL6 builds"
  echo "*** remove '$ATDM_CONFIG_KOKKOS_ARCH' from JOB_NAME=$JOB_NAME"
  echo "***"
  return
fi

echo "Using CEE RHEL6 compiler stack $ATDM_CONFIG_COMPILER to build $ATDM_CONFIG_BUILD_TYPE code with Kokkos node type $ATDM_CONFIG_NODE_TYPE"

export ATDM_CONFIG_ENABLE_SPARC_SETTINGS=ON
export ATDM_CONFIG_USE_NINJA=ON

# Get ATDM_CONFIG_NUM_CORES_ON_MACHINE for this machine
source $ATDM_SCRIPT_DIR/utils/get_num_cores_on_machine.sh

if [ "$ATDM_CONFIG_NUM_CORES_ON_MACHINE" -gt "16" ] ; then
  export ATDM_CONFIG_MAX_NUM_CORES_TO_USE=16
  # NOTE: We get links crashing if we try to use to many processes.  ToDo: We
  # should limit the number of processes that ninja uses to link instead of
  # reducing the overrall parallel build level like this.
else
  export ATDM_CONFIG_MAX_NUM_CORES_TO_USE=$ATDM_CONFIG_NUM_CORES_ON_MACHINE
fi

export ATDM_CONFIG_BUILD_COUNT=$ATDM_CONFIG_MAX_NUM_CORES_TO_USE
# NOTE: Use as many build processes and there are cores by default.

#
# Save the original PATH and LD_LIBRARY_PATH if not already saved
#

if [[ "${ATDM_PATH_ORIG}" == "" ]] ; then
  export ATDM_PATH_ORIG=$PATH
  export ATDM_LD_LIBRARY_PATH_ORIG=${LD_LIBRARY_PATH}
fi

#
# Purge as much as you can
#

module purge

unset BOOST_ROOT
unset HDF5_ROOT
unset CGNS_ROOT
unset PNETCDF_ROOT
unset NETCDF_ROOT
unset LIBHIO_ROOT
unset METIS_ROOT
unset PARMETIS_ROOT
unset SUPERLUDIST_ROOT
unset SGM_ROOT
unset EUCLID_ROOT
unset SUPERLUDIST_ROOT

export PATH=$ATDM_PATH_ORIG
export LD_LIBRARY_PATH=$ATDM_LD_LIBRARY_PATH_ORIG

#
# Set up basic number of processes to use for building and running tests
#

if [[ "$ATDM_CONFIG_NODE_TYPE" == "OPENMP" ]] ; then
  export ATDM_CONFIG_CTEST_PARALLEL_LEVEL=$(($ATDM_CONFIG_MAX_NUM_CORES_TO_USE/2))
  export OMP_NUM_THREADS=2
else
  export ATDM_CONFIG_CTEST_PARALLEL_LEVEL=$(($ATDM_CONFIG_MAX_NUM_CORES_TO_USE/2))
fi
# NOTE: Above, we use 1/2 as many executors as


#
# Set up the env based on the chosen compiler
#

if [ "$ATDM_CONFIG_COMPILER" == "CLANG-5.0.1-OPENMPI-1.10.2" ]; then

  module load sparc-dev/clang-5.0.1_openmpi-1.10.2
  export OMPI_CXX=`which clang++`
  export OMPI_CC=`which clang`
  export OMPI_FC=`which gfortran`
  export MPICC=`which mpicc`
  export MPICXX=`which mpicxx`
  export MPIF90=`which mpif90`

elif [[ "$ATDM_CONFIG_COMPILER" == "GNU-7.2.0-OPENMPI-1.10.2" ]] ; then

  module load sparc-dev/gcc-7.2.0_openmpi-1.10.2
  export OMPI_CXX=`which g++`
  export OMPI_CC=`which gcc`
  export OMPI_FC=`which gfortran`
  export MPICC=`which mpicc`
  export MPICXX=`which mpicxx`
  export MPIF90=`which mpif90`
  export ATDM_CONFIG_MPI_EXEC=mpirun
  export ATDM_CONFIG_MPI_EXEC_NUMPROCS_FLAG=-np
  export ATDM_CONFIG_MPI_POST_FLAGS="-bind-to;core"
  # NOTE: Above is waht What SPARC uses?

elif [[ "$ATDM_CONFIG_COMPILER" == "GNU-4.9.3-OPENMPI-1.10.2" ]] ; then

  module load tools/intel-advixe-2016.1
  module load tools/intel-amplxe-2016.1
  module load tools/intel-inspxe-2016.1
  SPARC_TPL_ROOT=/projects/sparc/tpls/cee-rhel6
  SPARC_TPL_BUILD_NAME=cee-cpu_gcc-4.9.3
  SPARC_MPI_TPL_BUILD_NAME=cee-cpu_gcc-4.9.3_openmpi-1.10.2
  export CBLAS_ROOT=/sierra/sntools/SDK/compilers/intel/composer_xe_2017.1.132/compilers_and_libraries/linux
  export GNU_ROOT=/sierra/sntools/SDK/compilers/gcc/4.9.3-RHEL6
  export MPI_ROOT=/sierra/sntools/SDK/mpi/openmpi/1.10.2-gcc-4.9.3-RHEL6
  export PATH=${MPI_ROOT}/bin:${GNU_ROOT}/bin:$PATH
  export LD_LIBRARY_PATH=${GNU_ROOT}/lib64:${GNU_ROOT}/lib:${LD_LIBRARY_PATH}
  export LD_LIBRARY_PATH=$MPI_ROOT}/lib:/sierra/sntools/SDK/hwloc/lib:${LD_LIBRARY_PATH}
  export CC=`which gcc`
  export CXX=`which g++`
  export FC=`which gfortran`
  export OMPI_CC=`which gcc`
  export OMPI_CXX=`which g++`
  export OMPI_FC=`which gfortran`
  export MPICC=`which mpicc`
  export MPICXX=`which mpicxx`
  export MPIF90=`which mpif90`
  export ATDM_CONFIG_MPI_PRE_FLAGS="--bind-to;none"
  # Still uses old TPL for SuperLUDist
  export SUPERLUDIST_ROOT=${SPARC_TPL_ROOT}/superlu_dist-4.2/${SPARC_MPI_TPL_BUILD_NAME}
  export ATDM_CONFIG_SUPERLUDIST_INCLUDE_DIRS=${SUPERLUDIST_ROOT}/SRC
  export ATDM_CONFIG_SUPERLUDIST_LIBS=${SUPERLUDIST_ROOT}/lib/libsuperlu_dist_4.2.a

elif [ "$ATDM_CONFIG_COMPILER" == "INTEL-18.0.2-MPICH2-3.2" ]; then

  module load sparc-dev/intel-18.0.2_mpich2-3.2
  export OMPI_CXX=`which icpc`
  export OMPI_CC=`which icc`
  export OMPI_FC=`which ifort`
  export MPICC=`which mpicc`
  export MPICXX=`which mpicxx`
  export MPIF90=`which mpif90`
  export ATDM_CONFIG_MPI_EXEC=mpirun
  export ATDM_CONFIG_MPI_EXEC_NUMPROCS_FLAG=-np
  export ATDM_CONFIG_MPI_POST_FLAGS="-bind-to;core" # Critical to perforamnce!
  export ATDM_CONFIG_OPENMP_FORTRAN_FLAGS=-fopenmp
  export ATDM_CONFIG_OPENMP_FORTRAN_LIB_NAMES=gomp
  export ATDM_CONFIG_OPENMP_GOMP_LIBRARY=-lgomp

elif [ "$ATDM_CONFIG_COMPILER" == "INTEL-17.0.1-INTELMPI-5.1.2" ]; then

  module load sparc-dev/intel-17.0.1_intelmpi-5.1.2
  export OMPI_CXX=`which icpc`
  export OMPI_CC=`which icc`
  export OMPI_FC=`which ifort`
  export MPICC=`which mpicc`
  export MPICXX=`which mpicxx`
  export MPIF90=`which mpif90`
  export ATDM_CONFIG_MPI_EXEC=mpirun
  export ATDM_CONFIG_MPI_EXEC_NUMPROCS_FLAG=-np
  export ATDM_CONFIG_OPENMP_FORTRAN_FLAGS=-fopenmp
  export ATDM_CONFIG_OPENMP_FORTRAN_LIB_NAMES=gomp
  export ATDM_CONFIG_OPENMP_GOMP_LIBRARY=-lgomp

else

  echo
  echo "***"
  echo "*** ERROR: COMPILER=$ATDM_CONFIG_COMPILER is not supported on this system!"
  echo "***"
  return

fi

# ToDo: Update above to only load the compiler and MPI moudles and then
# directly set <TPL_NAME>_ROOT to point to the right TPLs.  This is needed to
# avoid having people depend on the SPARC modules.

# Use updated Ninja and CMake
module load atdm-env
module load atdm-cmake/3.11.1
module load atdm-ninja_fortran/1.7.2

#
# Set up TPL ROOT env vars
#

export BOOST_ROOT=${SPARC_TPL_ROOT}/boost-1.59.0/${SPARC_TPL_BUILD_NAME}
export HDF5_ROOT=${SPARC_TPL_ROOT}/hdf5-1.8.16/${SPARC_MPI_TPL_BUILD_NAME}
export CGNS_ROOT=${SPARC_TPL_ROOT}/CGNS/${SPARC_MPI_TPL_BUILD_NAME}_hdf5-1.8.16
export PNETCDF_ROOT=${SPARC_TPL_ROOT}/pnetcdf-1.6.1/${SPARC_MPI_TPL_BUILD_NAME}
export NETCDF_ROOT=${SPARC_TPL_ROOT}/netcdf-4.4.0/${SPARC_MPI_TPL_BUILD_NAME}_hdf5-1.8.16
export METIS_ROOT=${SPARC_TPL_ROOT}/parmetis-4.0.3/${SPARC_MPI_TPL_BUILD_NAME}
export PARMETIS_ROOT=${SPARC_TPL_ROOT}/parmetis-4.0.3/${SPARC_MPI_TPL_BUILD_NAME}
export SUPERLUDIST_ROOT=${SPARC_TPL_ROOT}/superlu_dist-4.2/${SPARC_MPI_TPL_BUILD_NAME}

# ToDo: Can we remove these below for Trilinos?  Are these only needed for SPARC?
export LIBHIO_ROOT=${SPARC_TPL_ROOT}/libhio-1.4.1.2/cee_gcc-4.9.3_openmpi-1.10.2
export SGM_ROOT=${SPARC_TPL_ROOT}/sgm/${SPARC_MPI_TPL_BUILD_NAME}
export EUCLID_ROOT=${SPARC_TPL_ROOT}/euclid/${SPARC_MPI_TPL_BUILD_NAME}

#
# Set paths to executbles and libraries
#

export LD_LIBRARY_PATH=/usr/netpub/lld/lib:${LD_LIBRARY_PATH}
export LD_LIBRARY_PATH=${CBLAS_ROOT}/mkl/lib/intel64:${LD_LIBRARY_PATH}

export PATH=${BOOST_ROOT}/bin:${PATH}
export PATH=${HDF5_ROOT}/bin:${PATH}
export PATH=${CGNS_ROOT}/bin:${PATH}
export PATH=${PNETCDF_ROOT}/bin:${PATH}
export PATH=${NETCDF_ROOT}/bin:${PATH}

# ToDo: Can we remove this below for Trilinos?  Are these only needed by SPARC?
export PATH=${LIBHIO_ROOT}/bin:${PATH}

#
# Set TPL include dirs and libs
#

export ATDM_CONFIG_USE_HWLOC=OFF

export ATDM_CONFIG_BINUTILS_LIBS="/usr/lib64/libbfd.so;/usr/lib64/libiberty.a"
# NOTE: Above, we have to explicitly set the libs to use libbdf.so instead of
# libbdf.a because the former works and the latter does not and TriBITS is set
# up to only find static libs by default!

# BLAS and LAPACK

#export ATDM_CONFIG_BLAS_LIBS="-L${CBLAS_ROOT}/mkl/lib/intel64;-L${CBLAS_ROOT}/lib/intel64;-lmkl_intel_lp64;-lmkl_intel_thread;-lmkl_core;-liomp5"
#export ATDM_CONFIG_LAPACK_LIBS="-L${CBLAS_ROOT}/mkl/lib/intel64"

# NOTE: The above does not work.  For some reason, the library 'iomp5' can't
# be found at runtime.  Instead, you have to explicitly list out the library
# files in order as shown below.  Very sad.

atdm_config_add_libs_to_var ATDM_CONFIG_BLAS_LIBS ${CBLAS_ROOT}/mkl/lib/intel64 .so \
  mkl_intel_lp64 mkl_intel_thread mkl_core

atdm_config_add_libs_to_var ATDM_CONFIG_BLAS_LIBS ${CBLAS_ROOT}/lib/intel64 .so \
  iomp5

export ATDM_CONFIG_LAPACK_LIBS=${ATDM_CONFIG_BLAS_LIBS}

# HDF5
export ATDM_CONFIG_HDF5_LIBS="-L${HDF5_ROOT}/lib;${HDF5_ROOT}/lib/libhdf5_hl.a;${HDF5_ROOT}/lib/libhdf5.a;-lz;-ldl"

# Netcdf
export ATDM_CONFIG_NETCDF_LIBS="-L${BOOST_ROOT}/lib;-L${NETCDF_ROOT}/lib;-L${NETCDF_ROOT}/lib;-L${SEMS_PNETCDF_ROOT}/lib;-L${HDF5_ROOT}/lib;${BOOST_ROOT}/lib/libboost_program_options.a;${BOOST_ROOT}/lib/libboost_system.a;${NETCDF_ROOT}/lib/libnetcdf.a;${PNETCDF_ROOT}/lib/libpnetcdf.a;${HDF5_ROOT}/lib/libhdf5_hl.a;${HDF5_ROOT}/lib/libhdf5.a;-lz;-ldl;-lcurl"

# SuperLUDist
if [[ "${ATDM_CONFIG_SUPERLUDIST_INCLUDE_DIRS}" == "" ]] ; then
  # Set the default which is correct for all of the new TPL builds
  export ATDM_CONFIG_SUPERLUDIST_INCLUDE_DIRS=${SUPERLUDIST_ROOT}/include
  export ATDM_CONFIG_SUPERLUDIST_LIBS=${SUPERLUDIST_ROOT}/lib/libsuperlu_dist.a
fi

# Finished!
export ATDM_CONFIG_COMPLETED_ENV_SETUP=TRUE
