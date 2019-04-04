/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_DOWNLOAD_SERVICE_H_
#define BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_DOWNLOAD_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/brave_shields/browser/base_local_data_files_observer.h"
#include "extensions/common/url_pattern_set.h"
#include "url/gurl.h"

class GreaselionServiceTest;

namespace greaselion {

extern const char kGreaselionConfigFile[];
extern const char kGreaselionConfigFileVersion[];

enum GreaselionPreconditionValue { kMustBeFalse, kMustBeTrue, kAny };

struct GreaselionPreconditions {
  GreaselionPreconditionValue rewards_enabled;
  GreaselionPreconditionValue twitter_tips_enabled;
  GreaselionPreconditionValue profile_on_the_record;
};

class GreaselionRule {
 public:
  GreaselionRule(base::DictionaryValue* preconditions_value,
                 base::ListValue* urls_value,
                 base::ListValue* scripts_value,
                 const base::FilePath& root_dir);
  ~GreaselionRule();

  bool Matches(const GURL& url,
               bool rewards_enabled,
               bool twitter_tips_enabled,
               bool profile_on_the_record) const;
  void Populate(std::vector<std::string>* scripts) const;

 private:
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();
  void AddScriptAfterLoad(std::unique_ptr<std::string> contents, bool did_load);
  GreaselionPreconditionValue ParsePrecondition(base::DictionaryValue* root,
                                                const char* key);
  bool PreconditionFulfilled(GreaselionPreconditionValue precondition,
                             bool value) const;

  extensions::URLPatternSet urls_;
  std::vector<std::string> scripts_;
  GreaselionPreconditions preconditions_;
  base::WeakPtrFactory<GreaselionRule> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(GreaselionRule);
};

// The Greaselion download service is in charge
// of loading and parsing the Greaselion configuration file
// and the scripts that the configuration file references
class GreaselionDownloadService
    : public brave_shields::BaseLocalDataFilesObserver {
 public:
  GreaselionDownloadService();
  ~GreaselionDownloadService() override;

  std::vector<std::unique_ptr<GreaselionRule>>* rules() { return &rules_; }

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();

  // implementation of BaseLocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::GreaselionServiceTest;

  void OnDATFileDataReady();
  void LoadOnTaskRunner();

  std::string file_contents_;
  std::vector<std::unique_ptr<GreaselionRule>> rules_;
  base::FilePath install_dir_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<GreaselionDownloadService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(GreaselionDownloadService);
};

// Creates the GreaselionDownloadService
std::unique_ptr<GreaselionDownloadService> GreaselionDownloadServiceFactory();

}  // namespace greaselion

#endif  // BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_DOWNLOAD_SERVICE_H_
