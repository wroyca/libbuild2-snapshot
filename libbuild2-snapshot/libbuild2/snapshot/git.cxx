#include <libbuild2/snapshot/git.hxx>

#include <libbuild2/diagnostics.hxx>

#include <libbutl/process.hxx>
#include <libbutl/timestamp.hxx>
#include <libbutl/fdstream.hxx>

using namespace std;
using namespace butl;

namespace build2
{
  namespace snapshot
  {
    static inline string
    trim (const string& s)
    {
      size_t start = s.find_first_not_of (" \t\n\r");
      if (start == string::npos)
        return {};

      size_t end = s.find_last_not_of (" \t\n\r");
      return s.substr (start, end - start + 1);
    }

    // git_command_executor
    //

    string git_command_executor::
    execute (const strings& args) const
    {
      tracer trace ("git_command_executor::execute");

      l5 ([&] { trace << "executing: " << format_command (args); });

      optional<string> result = execute_optional (args);

      if (!result)
        throw git_command_error (format_command (args), "command failed");

      return std::move (*result);
    }

    bool git_command_executor::
    try_execute (const strings& args) const noexcept
    {
      tracer trace ("git_command_executor::try_execute");

      l5 ([&] { trace << "trying: " << format_command (args); });

      optional<string> result = execute_optional (args);

      if (!result)
      {
        l5 ([&] { trace << "command failed: " << format_command (args); });
        return false;
      }

      return true;
    }

    optional<string> git_command_executor::
    execute_optional (const strings& args) const noexcept
    {
      tracer trace ("git_command_executor::execute_optional");

      try
      {
        path git_path ("git");
        process_path pp (run_search (git_path, true /* init */));

        cstrings cmd_args;
        cmd_args.push_back (pp.recall_string ());
        for (const string& arg : args)
          cmd_args.push_back (arg.c_str ());
        cmd_args.push_back (nullptr);

        process pr (pp,
                    cmd_args,
                    0 /* stdin */,
                   -1 /* stdout */,
                    2 /* stderr */);

        string output;
        ifdstream is (std::move (pr.in_ofd),
                      fdstream_mode::skip,
                      ifdstream::badbit);

        for (string line; getline (is, line);)
        {
          if (!output.empty ())
            output += '\n';
          output += line;
        }

        is.close ();

        if (pr.wait ())
        {
          l5 ([&] { trace << "command succeeded, output length: "
                          << output.size (); });
          return output;
        }
        else
        {
          l5 ([&] { trace << "command failed with non-zero exit"; });
          return nullopt;
        }
      }
      catch (const process_error& e)
      {
        l5 ([&] { trace << "process error: " << e.what (); });
        return nullopt;
      }
    }

    string git_command_executor::
    format_command (const strings& args) const
    {
      string cmd = "git";
      for (const string& arg : args)
        cmd += " " + arg;
      return cmd;
    }

    // git_repository_state
    //

    bool git_repository_state::
    is_git_repository () const
    {
      return executor_.try_execute ({"rev-parse", "--git-dir"});
    }

    void git_repository_state::
    validate_repository () const
    {
      tracer trace ("git_repository_state::validate_repository");

      if (!is_git_repository ())
        fail << "not a git repository";

      l5 ([&] { trace << "repository validation passed"; });
    }

    optional<git_commit_info> git_repository_state::
    current_head () const
    {
      tracer trace ("git_repository_state::current_head");

      optional<string> hash_output =
        executor_.execute_optional ({"rev-parse", "HEAD"});
      if (!hash_output)
      {
        l5 ([&] { trace << "no HEAD found"; });
        return nullopt;
      }

      git_commit_info info;
      info.hash = trim (*hash_output);
      info.branch = current_branch ();

      // Get commit message.
      //
      optional<string> msg_output = executor_.execute_optional (
        {"log", "-1", "--format=%s", info.hash});
      if (msg_output)
        info.message = trim (*msg_output);

      l5 ([&] { trace << "HEAD: " << info.hash << " on branch: "
                      << (info.branch ? *info.branch : "detached"); });

      return info;
    }

    bool git_repository_state::
    has_uncommitted_changes () const
    {
      optional<string> status = executor_.execute_optional (
        {"status", "--porcelain"});
      return status && !status->empty ();
    }

    bool git_repository_state::
    has_untracked_files () const
    {
      optional<string> status = executor_.execute_optional (
        {"status", "--porcelain", "--untracked-files=normal"});

      if (!status)
        return false;

      // Look for lines starting with '??' which indicate untracked files.
      //
      istringstream iss (*status);
      for (string line; getline (iss, line);)
      {
        if (line.size () >= 2 && line.substr (0, 2) == "??")
          return true;
      }

      return false;
    }

    bool git_repository_state::
    is_clean_working_tree () const
    {
      return !has_uncommitted_changes ();
    }

    optional<string> git_repository_state::
    current_branch () const
    {
      optional<string> branch_ref = executor_.execute_optional (
        {"symbolic-ref", "-q", "HEAD"});

      if (!branch_ref)
        return nullopt;

      string ref = trim (*branch_ref);

      // Remove refs/heads/ prefix if present.
      //
      const string prefix = "refs/heads/";
      if (ref.substr (0, prefix.size ()) == prefix)
        return ref.substr (prefix.size ());

      return ref;
    }

    bool git_repository_state::
    is_detached_head () const
    {
      return !current_branch ();
    }

    vector<git_reference_info> git_repository_state::
    list_references (const string& pattern) const
    {
      tracer trace ("git_repository_state::list_references");

      strings args = {"for-each-ref", "--format=%(refname) %(objectname)"};
      if (!pattern.empty ())
        args.push_back (pattern);

      optional<string> output = executor_.execute_optional (args);
      if (!output)
        return {};

      vector<git_reference_info> refs;
      istringstream iss (*output);

      for (string line; getline (iss, line);)
      {
        if (line.empty ())
          continue;

        size_t space_pos = line.find (' ');
        if (space_pos == string::npos)
          continue;

        git_reference_info ref_info;
        ref_info.name = line.substr (0, space_pos);
        ref_info.hash = line.substr (space_pos + 1);
        ref_info.is_branch = ref_info.name.find ("refs/heads/") == 0;

        refs.push_back (std::move (ref_info));
      }

      l5 ([&] { trace << "found " << refs.size () << " references"; });
      return refs;
    }

    bool git_repository_state::
    parse_status_clean (const string& output) const
    {
      return output.empty ();
    }

    // git_reference_manager
    //

    void git_reference_manager::
    update_reference (const string& ref_name, const string& commit_hash) const
    {
      tracer trace ("git_reference_manager::update_reference");

      l5 ([&] { trace << "updating ref: " << ref_name
                                          << " -> "
                                          << commit_hash; });

      string result = executor_.execute ({"update-ref", ref_name, commit_hash});

      l5 ([&] { trace << "reference updated successfully"; });
    }

    void git_reference_manager::
    delete_reference (const string& ref_name) const
    {
      tracer trace ("git_reference_manager::delete_reference");

      l5 ([&] { trace << "deleting ref: " << ref_name; });

      if (!reference_exists (ref_name))
      {
        l5 ([&] { trace << "reference does not exist, skipping"; });
        return;
      }

      executor_.execute ({"update-ref", "-d", ref_name});

      l5 ([&] { trace << "reference deleted successfully"; });
    }

    bool git_reference_manager::
    reference_exists (const string& ref_name) const
    {
      return executor_.try_execute (
        {"show-ref", "--verify", "--quiet", ref_name});
    }

    optional<string> git_reference_manager::
    resolve_reference (const string& ref_name) const
    {
      optional<string> result = executor_.execute_optional (
        {"rev-parse", "--verify", ref_name});

      if (result)
        return trim (*result);

      return nullopt;
    }

    string git_reference_manager::
    generate_timestamped_ref (const string& base_path)
    {
      auto ts = timestamp::clock::now ();
      string timestamp = to_string (ts, "%Y%m%d-%H%M%S", true, true);
      return base_path + "/" + timestamp;
    }

    string git_reference_manager::
    generate_branch_ref (const string& base_path,
                        const string& branch_name,
                        const string& timestamp)
    {
      return base_path + "/" + branch_name + "/" + timestamp;
    }

    // git_snapshot_manager
    //

    void git_snapshot_manager::
    create_snapshot (const snapshot_config& config) const
    {
      tracer trace ("git_snapshot_manager::create_snapshot");

      l5 ([&] { trace << "creating snapshot with message: '"
                      << config.message << "'"; });

      validate_snapshot_preconditions ();

      string index_ref = create_index_snapshot (config);

      l5 ([&] { trace << "index snapshot created: " << index_ref; });

      // Create working tree snapshot if needed (i.e. if there are uncommitted
      // changes)
      //
      if (config.include_working_tree)
      {
        optional<string> wtree_ref = create_working_tree_snapshot (config);
        if (wtree_ref)
        {
          l5 ([&] { trace << "working tree snapshot created: "
                          << *wtree_ref; });
        }
        else
        {
          l5 ([&] { trace << "no working tree changes to snapshot"; });
        }
      }

      l1 ([&] { trace << "snapshot created successfully"; });
    }

    void git_snapshot_manager::
    create_snapshot () const
    {
      snapshot_config config;
      create_snapshot (config);
    }

    string git_snapshot_manager::
    create_index_snapshot (const snapshot_config& config) const
    {
      tracer trace ("git_snapshot_manager::create_index_snapshot");

      optional<git_commit_info> head = state_.current_head ();
      if (!head)
        fail << "cannot create snapshot without HEAD commit";

      string tree_hash = trim (executor_.execute ({"write-tree"}));
      string message = generate_snapshot_message (config);
      string commit_hash = create_commit_tree (tree_hash, head->hash, message);

      l5 ([&] { trace << "tree hash: " << tree_hash; });
      l5 ([&] { trace << "commit hash: " << commit_hash; });

      // Generate a unique reference name under the configured snapshot index.
      //
      // If the HEAD is on a branch, generate a ref in the form:
      //   <prefix>/index/<branch-name>/<timestamp>
      //
      // Otherwise (detached HEAD), generate a timestamp-only ref:
      //   <prefix>/index/<timestamp>
      //
      // The timestamp is in the format: YYYYMMDD-HHMMSS (UTC).
      //
      string timestamp = to_string (timestamp::clock::now (),
                                   "%Y%m%d-%H%M%S", true, true);
      string ref_name;

      if (head->branch)
      {
        ref_name = refs_.generate_branch_ref (config.ref_prefix + "/index",
                                             *head->branch, timestamp);
      }
      else
      {
        ref_name =
          refs_.generate_timestamped_ref (config.ref_prefix + "/index");
      }

      // fast-forwards the ref <ref_name> to <commit_hash>
      //
      refs_.update_reference (ref_name, commit_hash);

      return ref_name;
    }

    optional<string> git_snapshot_manager::
    create_working_tree_snapshot (const snapshot_config& config) const
    {
      tracer trace ("git_snapshot_manager::create_working_tree_snapshot");

      if (state_.is_clean_working_tree () &&
          (!config.include_untracked || !state_.has_untracked_files ()))
      {
        l5 ([&] { trace << "working tree is clean, no snapshot needed"; });
        return nullopt;
      }

      // The timestamp is in the format: YYYYMMDD-HHMMSS (UTC).
      //
      string timestamp = to_string (timestamp::clock::now (),
                                   "%Y%m%d-%H%M%S", true, true);
      string stash_message = "snapshot " + timestamp;

      strings stash_args = {"stash", "push", "-m", stash_message};
      if (config.include_untracked)
        stash_args.insert (stash_args.begin () + 2, "--include-untracked");

      executor_.execute (stash_args);

      string stash_hash = trim (executor_.execute ({"rev-parse", "stash@{0}"}));
      l5 ([&] { trace << "stash hash: " << stash_hash; });

      // Generate our own permanent, timestamped reference under the working
      // tree namespace.
      //
      // This creates a Git ref in the form:
      //   <prefix>/wtree/<timestamp>
      //
      string ref_name =
        refs_.generate_timestamped_ref (config.ref_prefix + "/wtree");
      refs_.update_reference (ref_name, stash_hash);

      // Restore the working tree state from the stash.
      //
      // After capturing the working tree state in a permanent reference, we
      // restore the original working tree state so that we can continue working
      // normally.
      //
      executor_.execute ({"stash", "apply", "stash@{0}"});
      l5 ([&] { trace << "working tree restored from stash"; });

      // Remove the transient stash entry.
      //
      // After promoting the stash to a permanent reference, we drop the
      // original stash entry (`stash@{0}`) to avoid duplication and maintain a
      // clean stash stack. This operation is non-fatal; if it fails (e.g., due
      // to race conditions or prior deletion), we proceed regardless.
      //
      optional<string> stash_drop =
        executor_.execute_optional ({"stash", "drop", "stash@{0}"});
      if (!stash_drop || stash_drop->empty())
        l5 ([&] { trace << "failed to drop stash, continuing with snapshot"; });

      return ref_name;
    }

    string git_snapshot_manager::
    create_commit_tree (const string& tree_hash,
                       const string& parent_hash,
                       const string& message) const
    {
      tracer trace ("git_snapshot_manager::create_commit_tree");

      string commit_hash = trim (executor_.execute (
        {"commit-tree", tree_hash, "-p", parent_hash, "-m", message}));

      l5 ([&] { trace << "created commit: " << commit_hash; });
      return commit_hash;
    }

    string git_snapshot_manager::
    generate_snapshot_message (const snapshot_config& config) const
    {
      if (!config.message.empty ())
        return config.message;

      string timestamp = to_string (timestamp::clock::now (),
                                   "%Y%m%d-%H%M%S", true, true);
      return "build2 snapshot " + timestamp;
    }

    void git_snapshot_manager::
    validate_snapshot_preconditions () const
    {
      tracer trace ("git_snapshot_manager::validate_snapshot_preconditions");

      state_.validate_repository ();

      if (!state_.current_head ())
        fail << "cannot create snapshot: no HEAD commit found";

      l5 ([&] { trace << "snapshot preconditions validated"; });
    }

    // git_repository
    //

    void git_repository::
    snapshot (const string& message) const
    {
      tracer trace ("git_repository::snapshot");

      git_snapshot_manager::snapshot_config config;
      config.message = message;

      snapshot_manager_.create_snapshot (config);
    }

    bool git_repository::
    is_clean () const
    {
      return snapshot_manager_.state ().is_clean_working_tree ();
    }

    optional<string> git_repository::
    current_branch () const
    {
      return snapshot_manager_.state ().current_branch ();
    }
  }
}
