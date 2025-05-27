#include <libbuild2/snapshot/init.hxx>

#include <libbuild2/diagnostics.hxx>

#include <libbuild2/snapshot/rule.hxx>

using namespace std;

namespace build2
{
  namespace snapshot
  {
    bool
    init (scope&,
          scope& bs,
          const location& l,
          bool first,
          bool,
          module_init_extra&)
    {
      tracer trace ("snapshot::init");

      if (!first)
        fail (l) << "multiple snapshot module initializations";

      const auto& s (snapshot_rule::instance);

      // Register rules.
      //
      bs.insert_rule<exe> (perform_update_id, "snapshot", s);

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
