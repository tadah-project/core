// The commands of the blueprint ([TASKS.*]) and their command-line options:
// one command per run, no TASK key and no --task option, no option on a
// command that groups others (tadah data, tadah properties, tadah
// analysis), and every name in an option's cli_excludes and cli_needs an
// option of the same command.

#include "catch2/catch.hpp"

#include <tadah/core/blueprint_parser.h>

#include <string>
#include <vector>

using namespace tadah::core;
using Catch::Matchers::Contains;

namespace {

/* Every option of @p task: its own and those of its groups. */
std::vector<const BlueprintKey *> options_of(const BlueprintTask &task) {
  std::vector<const BlueprintKey *> out;
  for (const BlueprintKey &k : task.blueprintKeys) out.push_back(&k);
  for (const BlueprintKeyGroup &g : task.groups)
    for (const BlueprintKey &k : g.blueprintKeys) out.push_back(&k);
  return out;
}

/* Whether a blueprint command has commands of its own (DATA has
 * DATA.convert, ...). */
bool groups_commands(const std::string &name) {
  const std::string prefix = name + ".";
  for (const auto &kv : BlueprintParser::getTasksMapRef())
    if (kv.first.compare(0, prefix.size(), prefix) == 0) return true;
  return false;
}

/* An option "X.<name>" with the long flag --<name>. */
BlueprintKey option(const std::string &name) {
  BlueprintKey k;
  k.keyName = "X." + name;
  k.cli_long_flag = "--" + name;
  return k;
}

}  // namespace

/* Pins: the blueprint has no configuration key TASK, and no command, nor
 * tadah itself, has an option --task or -t: a run is one command, given on
 * the command line, and a configuration file holds the keys of that
 * command. [TADAH] has no keys table, so tadah alone takes only -h and
 * --version (Tadah.MLIP).
 * Before the fix: ALLOWED_KEYS.task (--task, -t, a file of TASK sections)
 * was a key, inherited by train, predict, hpo, the command groups data,
 * properties and analysis and every command of theirs, and by [TADAH.keys]:
 * every check fails. */
TEST_CASE("Blueprint: no key TASK, and no option --task or -t on a command "
          "or on tadah itself",
          "[BlueprintParser][commands]") {
  CHECK(BlueprintParser::getKeysMapRef().count("TASK") == 0);
  for (const auto &kv : BlueprintParser::getTasksMapRef()) {
    for (const BlueprintKey *k : options_of(kv.second)) {
      INFO("command " << kv.first << ", option " << k->keyName);
      CHECK(k->cli_long_flag != "--task");
      CHECK(k->cli_short_flag != "-t");
    }
  }
  const BlueprintTadah &top = BlueprintParser::getBlueprintTadahRef();
  CHECK(top.blueprintKeys.empty());
  CHECK(top.groups.empty());
  for (const std::string &example : top.examples) {
    INFO(example);
    CHECK(example.find("--task") == std::string::npos);
  }
}

/* Pins: a command that groups others (tadah data, tadah properties, tadah
 * analysis) has no option and no option group: it runs nothing itself, and
 * a command line names one of its commands. Its examples run one of them.
 * Before the fix: data and properties had --verbose and --task, analysis
 * --task, and the example of data was `tadah data --task tasks.tadah`. */
TEST_CASE("Blueprint: a command that groups others has no option",
          "[BlueprintParser][commands]") {
  std::size_t groups = 0;
  for (const auto &kv : BlueprintParser::getTasksMapRef()) {
    if (!groups_commands(kv.first)) continue;
    ++groups;
    INFO("command " << kv.first);
    CHECK(kv.second.blueprintKeys.empty());
    CHECK(kv.second.groups.empty());
    CHECK_FALSE(kv.second.examples.empty());
    for (const std::string &example : kv.second.examples) {
      INFO(example);
      CHECK(example.find("--task") == std::string::npos);
    }
  }
  CHECK(groups == 3);
}

/* Pins: every name in the cli_excludes and cli_needs of an option of a
 * command of the blueprint is the long flag, without "--", of another
 * option of the same command: the command line excludes or needs exactly
 * that option.
 * Before the fix: fails on the option --task of 21 commands, whose
 * cli_excludes was ["ALL"], a word the command line read as every other
 * option of the command and no option is named. */
TEST_CASE("Blueprint: the cli_excludes and cli_needs of each option name "
          "options of its command",
          "[BlueprintParser][commands]") {
  std::size_t names = 0;
  for (const auto &kv : BlueprintParser::getTasksMapRef()) {
    const auto options = options_of(kv.second);
    auto is_option = [&](const BlueprintKey *self, const std::string &n) {
      for (const BlueprintKey *o : options)
        if (o != self && o->cli_long_flag == "--" + n) return true;
      return false;
    };
    for (const BlueprintKey *k : options) {
      for (const auto *list : {&k->cli_excludes, &k->cli_needs})
        for (const std::string &n : *list) {
          INFO("command " << kv.first << ", option " << k->keyName
                          << ", name '" << n << "'");
          CHECK(is_option(k, n));
          ++names;
        }
    }
  }
  CHECK(names > 30);
}

/* Pins: loading the blueprint refuses a command with an option whose
 * cli_excludes or cli_needs names no other option of the command (no long
 * flag --<name> among its options and those of its groups): the message
 * names the command, the option, the field and the name. A name of an
 * option of a group of the command is accepted, and so is a command whose
 * names are all its options; every command of the blueprint passes.
 * Before the fix: the check of a command's options when the blueprint
 * loads (BlueprintParser::checkRetiredOptions, which checkCommandOptions
 * replaces) read only the field only_with of its retired options: every
 * command passed (no exception), and the command line threw
 * CLI::OptionNotFound for such a name when tadah started. */
TEST_CASE("Blueprint: an option whose cli_excludes or cli_needs names no "
          "option of its command is refused",
          "[BlueprintParser][commands]") {
  BlueprintTask task;
  task.taskName = "X";
  BlueprintKey a = option("alpha");
  a.cli_excludes = {"beta"};
  BlueprintKey b = option("beta");
  b.cli_needs = {"gamma"};
  BlueprintKeyGroup group;
  group.groupName = "input";
  group.blueprintKeys = {option("gamma")};
  task.blueprintKeys = {a, b};
  task.groups = {group};
  CHECK_NOTHROW(BlueprintParser::checkCommandOptions(task));

  for (const bool needs : {false, true}) {
    for (const std::string name : {"delta", "alpha", "ALL"}) {
      INFO((needs ? "cli_needs " : "cli_excludes ") << name);
      BlueprintTask bad = task;
      (needs ? bad.blueprintKeys[0].cli_needs
             : bad.blueprintKeys[0].cli_excludes) = {name};
      REQUIRE_THROWS_WITH(
          BlueprintParser::checkCommandOptions(bad),
          Contains("command X") && Contains("--alpha") &&
              Contains(needs ? "cli_needs" : "cli_excludes") &&
              Contains("'" + name + "'") &&
              Contains("names no other option of the command"));
    }
  }
  BlueprintTask in_group = task;
  in_group.groups[0].blueprintKeys[0].cli_excludes = {"epsilon"};
  REQUIRE_THROWS_WITH(BlueprintParser::checkCommandOptions(in_group),
                      Contains("--gamma") && Contains("'epsilon'"));

  for (const auto &kv : BlueprintParser::getTasksMapRef()) {
    INFO(kv.first);
    CHECK_NOTHROW(BlueprintParser::checkCommandOptions(kv.second));
  }
}
