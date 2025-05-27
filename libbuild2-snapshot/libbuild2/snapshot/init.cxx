#include <libbuild2/snapshot/init.hxx>

#include <libbuild2/diagnostics.hxx>

using namespace std;

namespace build2
{
  namespace snapshot
  {
    bool
    init (scope&,
          scope&,
          const location& l,
          bool,
          bool,
          module_init_extra&)
    {
      return true;
    }

    static const module_functions mod_functions[] =
    {
      {"snapshot", nullptr, init},
      {nullptr,    nullptr, nullptr}
    };

    const module_functions*
    build2_snapshot_load ()
    {
      return mod_functions;
    }
  }
}
