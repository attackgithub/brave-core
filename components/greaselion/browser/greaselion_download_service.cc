/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/greaselion/browser/greaselion_download_service.h"

#include <memory>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/local_data_files_service.h"

namespace greaselion {

const char kGreaselionConfigFile[] = "Greaselion.json";
const char kGreaselionConfigFileVersion[] = "1";
// Greaselion.json keys
const char kPreconditions[] = "preconditions";
const char kURLs[] = "urls";
const char kScripts[] = "scripts";
// precondition keys
const char kRewards[] = "rewards-enabled";
const char kTwitterTips[] = "twitter-tips-enabled";
const char kOnTheRecord[] = "profile-on-the-record";

GreaselionPreconditionValue GreaselionRule::ParsePrecondition(
    base::DictionaryValue* root,
    const char* key) {
  base::Value* node = nullptr;
  GreaselionPreconditionValue value = kAny;
  if (root) {
    node = root->FindKeyOfType(key, base::Value::Type::BOOLEAN);
    if (node)
      value = node->GetBool() ? kMustBeTrue : kMustBeFalse;
  }
  return value;
}

GreaselionRule::GreaselionRule(base::DictionaryValue* preconditions_value,
                               base::ListValue* urls_value,
                               base::ListValue* scripts_value,
                               const base::FilePath& root_dir)
    : weak_factory_(this) {
  std::vector<std::string> patterns;
  preconditions_.rewards_enabled =
      ParsePrecondition(preconditions_value, kRewards);
  preconditions_.twitter_tips_enabled =
      ParsePrecondition(preconditions_value, kTwitterTips);
  preconditions_.profile_on_the_record =
      ParsePrecondition(preconditions_value, kOnTheRecord);
  // Can't use URLPatternSet::Populate here because it does not expose
  // any way to set the ParseOptions, which we need to do to support
  // eTLD wildcarding.
  for (const auto& urls_it : urls_value->GetList()) {
    URLPattern pattern;
    pattern.SetValidSchemes(URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS);
    if (pattern.Parse(urls_it.GetString(),
                      URLPattern::ALLOW_WILDCARD_FOR_EFFECTIVE_TLD) !=
        URLPattern::ParseResult::kSuccess) {
      LOG(ERROR) << "Malformed pattern in Greaselion configuration";
      urls_.ClearPatterns();
      return;
    }
    urls_.AddPattern(pattern);
  }
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  for (const auto& scripts_it : scripts_value->GetList()) {
    base::FilePath script_path =
        root_dir.AppendASCII(kGreaselionConfigFileVersion)
            .AppendASCII(scripts_it.GetString());
    if (script_path.ReferencesParent()) {
      LOG(ERROR) << "Malformed filename in Greaselion configuration";
    } else {
      // Read script file on task runner to avoid file I/O on main thread.
      auto script_contents = std::make_unique<std::string>();
      std::string* buffer = script_contents.get();
      base::PostTaskAndReplyWithResult(
          task_runner.get(), FROM_HERE,
          base::BindOnce(&base::ReadFileToString, script_path, buffer),
          base::BindOnce(&GreaselionRule::AddScriptAfterLoad,
                         weak_factory_.GetWeakPtr(),
                         std::move(script_contents)));
    }
  }
}

GreaselionRule::~GreaselionRule() = default;

void GreaselionRule::AddScriptAfterLoad(std::unique_ptr<std::string> contents,
                                        bool did_load) {
  if (!did_load || !contents) {
    LOG(ERROR) << "Could not load Greaselion script";
    return;
  }
  scripts_.push_back(*contents);
}

bool GreaselionRule::PreconditionFulfilled(
    GreaselionPreconditionValue precondition,
    bool value) const {
  switch (precondition) {
    case kMustBeTrue:
      return value;
    case kMustBeFalse:
      return !value;
    default:
      return true;
  }
}

bool GreaselionRule::Matches(const GURL& url,
                             bool rewards_enabled,
                             bool twitter_tips_enabled,
                             bool profile_on_the_record) const {
  if (!PreconditionFulfilled(preconditions_.rewards_enabled, rewards_enabled))
    return false;
  if (!PreconditionFulfilled(preconditions_.twitter_tips_enabled,
                             twitter_tips_enabled))
    return false;
  if (!PreconditionFulfilled(preconditions_.profile_on_the_record,
                             profile_on_the_record))
    return false;
  return urls_.MatchesURL(url);
}

void GreaselionRule::Populate(std::vector<std::string>* scripts) const {
  scripts->insert(scripts->end(), scripts_.begin(), scripts_.end());
}

GreaselionDownloadService::GreaselionDownloadService()
    : task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

GreaselionDownloadService::~GreaselionDownloadService() {}

void GreaselionDownloadService::OnDATFileDataReady() {
  rules_.clear();
  if (file_contents_.empty()) {
    LOG(ERROR) << "Could not obtain Greaselion configuration";
    return;
  }
  base::Optional<base::Value> root = base::JSONReader::Read(file_contents_);
  file_contents_.clear();
  if (!root) {
    LOG(ERROR) << "Failed to parse Greaselion configuration";
    return;
  }
  base::ListValue* root_list = nullptr;
  root->GetAsList(&root_list);
  for (base::Value& rule_it : root_list->GetList()) {
    base::DictionaryValue* rule_dict = nullptr;
    rule_it.GetAsDictionary(&rule_dict);
    base::DictionaryValue* preconditions_value = nullptr;
    rule_dict->GetDictionary(kPreconditions, &preconditions_value);
    base::ListValue* urls_value = nullptr;
    rule_dict->GetList(kURLs, &urls_value);
    base::ListValue* scripts_value = nullptr;
    rule_dict->GetList(kScripts, &scripts_value);
    std::unique_ptr<GreaselionRule> rule = std::make_unique<GreaselionRule>(
        preconditions_value, urls_value, scripts_value, install_dir_);
    rules_.push_back(std::move(rule));
  }
}

void GreaselionDownloadService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  install_dir_ = install_dir;
  base::FilePath dat_file_path =
      install_dir.AppendASCII(kGreaselionConfigFileVersion)
          .AppendASCII(kGreaselionConfigFile);
  GetTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&brave_shields::GetDATFileAsString, dat_file_path,
                 &file_contents_),
      base::Bind(&GreaselionDownloadService::OnDATFileDataReady,
                 weak_factory_.GetWeakPtr()));
}

scoped_refptr<base::SequencedTaskRunner>
GreaselionDownloadService::GetTaskRunner() {
  return task_runner_;
}

///////////////////////////////////////////////////////////////////////////////

// The factory
std::unique_ptr<GreaselionDownloadService> GreaselionDownloadServiceFactory() {
  std::unique_ptr<GreaselionDownloadService> service =
      std::make_unique<GreaselionDownloadService>();
  g_brave_browser_process->local_data_files_service()->AddObserver(
      service.get());
  return service;
}

}  // namespace greaselion
