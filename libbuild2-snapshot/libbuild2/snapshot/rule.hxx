#pragma once

#include <libbuild2/rule.hxx>

#include <libbuild2/snapshot/export.hxx>

namespace build2
{
  namespace snapshot
  {
    class LIBBUILD2_SNAPSHOT_SYMEXPORT snapshot_rule : public simple_rule
    {
    public:
      snapshot_rule ();
      static const snapshot_rule instance;

      virtual bool
      match (action, target&) const override;

      virtual recipe
      apply (action, target&) const override;
    };
  }
}
