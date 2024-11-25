# setup latest, supported LCG release via cvmfs

if test -n "$BASH_SOURCE"; then
  this_script=$BASH_SOURCE
elif test -n "$ZSH_VERSION"; then
  setopt function_argzero
  this_script=$0
else
  echo "Unsupported shell. Please use bash or zsh." 1>&2
  return
fi

dir="$( cd "$( dirname "${this_script}" )" && pwd )"

# Clear any existing LCG environment
unset CMAKE_PREFIX_PATH
unset BOOST_ROOT
unset BOOST_INCLUDEDIR
unset BOOST_LIBRARYDIR

# Remove any LCG 106 paths from PATH and LD_LIBRARY_PATH
PATH=$(echo $PATH | tr ':' '\n' | grep -v "LCG_106" | tr '\n' ':' | sed 's/:$//')
LD_LIBRARY_PATH=$(echo $LD_LIBRARY_PATH | tr ':' '\n' | grep -v "LCG_106" | tr '\n' ':' | sed 's/:$//')

# Setup LCG environment
source /cvmfs/sft.cern.ch/lcg/views/LCG_105/x86_64-el9-gcc13-opt/setup.sh

# Explicitly set Boost environment variables with correct hash
export BOOST_ROOT="/cvmfs/sft.cern.ch/lcg/releases/Boost/1.82.0-fbfc9/x86_64-el9-gcc13-opt"
export BOOST_INCLUDEDIR="${BOOST_ROOT}/include"
export BOOST_LIBRARYDIR="${BOOST_ROOT}/lib"

# Ensure LCG 106 is not in the path
export CMAKE_PREFIX_PATH="/cvmfs/sft.cern.ch/lcg/views/LCG_105/x86_64-el9-gcc13-opt"

# Source the original script if you still need anything from it
source $dir/setup_cvmfs_lcg105.sh
