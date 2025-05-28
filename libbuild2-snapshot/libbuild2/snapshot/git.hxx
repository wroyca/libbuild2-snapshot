#pragma once

#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>

#include <libbuild2/snapshot/export.hxx>

namespace build2
{
  namespace snapshot
  {
    class LIBBUILD2_SNAPSHOT_SYMEXPORT git_error : public std::runtime_error
    {
    public:
      explicit git_error (const string& msg) : runtime_error (msg) {}
    };

    class LIBBUILD2_SNAPSHOT_SYMEXPORT git_command_error : public git_error
    {
    public:
      git_command_error (const string& cmd, const string& msg)
        : git_error ("git command '" + cmd + "' failed: " + msg),
          command (cmd) {}

      string command;
    };

    class git_command_executor;
    class git_repository_state;
    class git_reference_manager;

    struct git_commit_info
    {
      string hash;
      string message;
      optional<string> branch;
    };

    struct git_reference_info
    {
      string name;
      string hash;
      bool is_branch;
    };

    class LIBBUILD2_SNAPSHOT_SYMEXPORT git_command_executor
    {
    public:
      git_command_executor () = default;

      string
      execute (const strings& args) const;

      bool
      try_execute (const strings& args) const noexcept;

      optional<string>
      execute_optional (const strings& args) const noexcept;

    private:
      // Build full command line for diagnostics.
      //
      string
      format_command (const strings& args) const;
    };

    class LIBBUILD2_SNAPSHOT_SYMEXPORT git_repository_state
    {
    public:
      explicit git_repository_state (const git_command_executor& exec)
        : executor_ (exec) {}

      // Repository validation
      //

      bool
      is_git_repository () const;

      void
      validate_repository () const;

      // Current state queries
      //

      optional<git_commit_info>
      current_head () const;

      bool
      has_uncommitted_changes () const;

      bool
      has_untracked_files () const;

      bool
      is_clean_working_tree () const;

      // Branch and reference queries
      //

      optional<string>
      current_branch () const;

      bool
      is_detached_head () const;

      vector<git_reference_info>
      list_references (const string& pattern = {}) const;

    private:
      const git_command_executor& executor_;

      // Parse git status porcelain output.
      //
      bool
      parse_status_clean (const string& output) const;
    };

    class LIBBUILD2_SNAPSHOT_SYMEXPORT git_reference_manager
    {
    public:
      explicit git_reference_manager (const git_command_executor& exec)
        : executor_ (exec) {}

      void
      update_reference (const string& ref_name, const string& commit_hash) const;

      void
      delete_reference (const string& ref_name) const;

      bool
      reference_exists (const string& ref_name) const;

      optional<string>
      resolve_reference (const string& ref_name) const;

      static string
      generate_timestamped_ref (const string& base_path);

      static string
      generate_branch_ref (const string& base_path,
                          const string& branch_name,
                          const string& timestamp);

    private:
      const git_command_executor& executor_;
    };

    class LIBBUILD2_SNAPSHOT_SYMEXPORT git_snapshot_manager
    {
    public:
      struct snapshot_config
      {
        string message = {};
        bool include_working_tree = true;
        bool include_untracked = true;
        string ref_prefix = "refs/build2/snapshot";
      };

      explicit git_snapshot_manager (const git_command_executor& exec)
        : executor_ (exec),
          state_ (exec),
          refs_ (exec) {}

      // Create complete snapshot of repository state.
      //

      void
      create_snapshot (const snapshot_config& config) const;

      void
      create_snapshot () const;

      // Access to subsystems.
      //

      const git_repository_state&
      state () const { return state_; }

      const git_reference_manager&
      references () const { return refs_; }

    private:
      const git_command_executor& executor_;
      git_repository_state state_;
      git_reference_manager refs_;

      // Individual snapshot operations.
      //

      string
      create_index_snapshot (const snapshot_config& config) const;

      optional<string>
      create_working_tree_snapshot (const snapshot_config& config) const;

      // Helper functions.
      //

      string
      create_commit_tree (const string& tree_hash,
                         const string& parent_hash,
                         const string& message) const;

      string
      generate_snapshot_message (const snapshot_config& config) const;

      void
      validate_snapshot_preconditions () const;
    };

    class LIBBUILD2_SNAPSHOT_SYMEXPORT git_repository
    {
    public:
      git_repository () : snapshot_manager_ (executor_) {}

      void
      snapshot (const string& message = {}) const;

      // Repository state queries.
      //

      bool
      is_clean () const;

      optional<string>
      current_branch () const;

      // Direct access to subsystems.
      //

      const git_command_executor&
      executor () const { return executor_; }

      const git_repository_state&
      state () const { return snapshot_manager_.state (); }

      const git_reference_manager&
      references () const { return snapshot_manager_.references (); }

    private:
      git_command_executor executor_;
      git_snapshot_manager snapshot_manager_;
    };
  }
}
