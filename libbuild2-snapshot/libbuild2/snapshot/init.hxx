#pragma once


#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>
#include <libbuild2/module.hxx>

#include <libbuild2/snapshot/export.hxx>

namespace build2
{
  namespace snapshot
  {
    extern "C"
    {
      LIBBUILD2_SNAPSHOT_SYMEXPORT const module_functions*
      build2_snapshot_load ();
    }
  }
}
