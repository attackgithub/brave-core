/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/local_data_files_service.h"

#include <algorithm>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/base_local_data_files_observer.h"

namespace brave_shields {

std::string LocalDataFilesService::g_local_data_files_component_id_(
    kLocalDataFilesComponentId);
std::string
    LocalDataFilesService::g_local_data_files_component_base64_public_key_(
        kLocalDataFilesComponentBase64PublicKey);

LocalDataFilesService::LocalDataFilesService()
    : initialized_(false),
      observers_already_called_(false),
      weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

LocalDataFilesService::~LocalDataFilesService() {}

bool LocalDataFilesService::Start() {
  if (initialized_)
    return true;
  // Ensure that all services that observe the local data files service
  // are created before calling Start().
  g_brave_browser_process->autoplay_whitelist_service();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  g_brave_browser_process->extension_whitelist_service();
#endif
  g_brave_browser_process->referrer_whitelist_service();
  g_brave_browser_process->greaselion_download_service();
  g_brave_browser_process->tracking_protection_service();

  Register(kLocalDataFilesComponentName, g_local_data_files_component_id_,
           g_local_data_files_component_base64_public_key_);
  initialized_ = true;
  return true;
}

void LocalDataFilesService::AddObserver(BaseLocalDataFilesObserver* observer) {
  DCHECK(!observers_already_called_);
  observers_.push_back(observer);
}

void LocalDataFilesService::OnComponentReady(const std::string& component_id,
                                             const base::FilePath& install_dir,
                                             const std::string& manifest) {
  observers_already_called_ = true;
  for (BaseLocalDataFilesObserver* observer : observers_)
    observer->OnComponentReady(component_id, install_dir, manifest);
}

// static
void LocalDataFilesService::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_local_data_files_component_id_ = component_id;
  g_local_data_files_component_base64_public_key_ = component_base64_public_key;
}

///////////////////////////////////////////////////////////////////////////////

// The brave shields factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<LocalDataFilesService> LocalDataFilesServiceFactory() {
  return std::make_unique<LocalDataFilesService>();
}

}  // namespace brave_shields
