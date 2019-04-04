/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_H_
#define BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace greaselion {

class GreaselionService : public KeyedService {
 public:
  GreaselionService() = default;

  virtual bool ScriptsFor(const GURL& primary_url,
                          std::vector<std::string>* scripts) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(GreaselionService);
};

}  // namespace greaselion

#endif  // BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_H_
