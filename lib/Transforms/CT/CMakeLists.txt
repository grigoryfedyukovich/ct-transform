add_llvm_loadable_module( LLVMCT
  Print.cpp
  CTite.cpp
  CTloop.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
