#pragma once

#include "core/rules.hpp"

#include <obs.h>

namespace ff {

obs_data_t *rulesToData(const Rules &rules);
Rules rulesFromData(obs_data_t *data);

} // namespace ff
