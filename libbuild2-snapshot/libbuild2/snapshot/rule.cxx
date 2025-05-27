#pragma once

#include <libbuild2/snapshot/rule.hxx>

#include <libbuild2/target.hxx>
#include <libbuild2/algorithm.hxx>
#include <libbuild2/diagnostics.hxx>

#include <libbuild2/snapshot/export.hxx>

namespace build2
{
  namespace snapshot
  {
    snapshot_rule::
    snapshot_rule () {}

    const snapshot_rule snapshot_rule::instance = snapshot_rule {};

    namespace
    {
      target_state
      perform_update (action a, const target& t)
      {
        tracer trace ("snapshot_rule::perform_update");

        l5 ([&]
        {
          trace << "for target: " << t.name << " with action: " << a;
        });

        target_state ts = straight_execute_prerequisites (a, t);

        return ts;
      }
    }

    bool snapshot_rule::
    match (action a, target& t) const
    {
      tracer trace ("snapshot_rule::match");

      l5 ([&]
      {
        trace << "for target: " << t.name << " with action: " << a;
      });

      if (a != perform_update_id)
      {
        l5 ([&]
        {
          trace << "action is not perform_update, not matching";
        });

        return false;
      }

      return true;
    }

    recipe snapshot_rule::
    apply (action a, target& t) const
    {
      tracer trace ("snapshot_rule::apply");

      l5 ([&]
      {
        trace << "for target: " << t.name << " with action: " << a;
      });

      switch (a)
      {
      case perform_update_id:
        return &perform_update;
      default:
        assert (false);
        return default_recipe;
      }
    }
  }
}
