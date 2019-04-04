/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/greaselion/browser/greaselion_service_impl.h"

#include <memory>

#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace greaselion {

GreaselionServiceImpl::GreaselionServiceImpl(
    Profile* profile,
    GreaselionDownloadService* download_service)
    : profile_(profile), download_service_(download_service) {}

GreaselionServiceImpl::~GreaselionServiceImpl() = default;

bool GreaselionServiceImpl::ScriptsFor(const GURL& primary_url,
                                       std::vector<std::string>* scripts) {
  auto rewards_enabled = profile_->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled);
  auto twitter_tips_enabled = false;  // TODO(mpilgrim): how?
  bool profile_on_the_record = !profile_->IsOffTheRecord();
  bool any = false;
  std::vector<std::unique_ptr<GreaselionRule>>* rules =
      download_service_->rules();
  scripts->clear();
  for (const auto& rule : *rules) {
    if (rule->Matches(primary_url, rewards_enabled, twitter_tips_enabled,
                      profile_on_the_record)) {
      rule->Populate(scripts);
      any = true;
    }
  }
  return any;
}

}  // namespace greaselion
