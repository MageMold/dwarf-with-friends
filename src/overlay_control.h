// dwf - multiplayer Dwarf Fortress in the browser, as a DFHack plugin
// Copyright (C) 2026 Gabriel Rios
// Copyright (C) 2026 Jake Taplin
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, version 3 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Runs on DFHack (Zlib); descends from DFPlex (Zlib) and webfort (ISC).
// Full license: see LICENSE. Third-party credits: see NOTICE.
//
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "ColorText.h"

#include <string>

namespace dwf {

bool disable_overlay_for_stream(DFHack::color_ostream& out, std::string* note = nullptr);
void restore_overlay_after_stream(DFHack::color_ostream* out = nullptr);

// Keep-overlay mode (Windows): the DFHack overlay plugin stays enabled while streaming and dwf
// renders remote cameras into private viewport buffers instead of re-rendering the host
// viewscreen. Create <DF>/dfcapture_disable_overlay.txt to force the legacy behaviour.
bool overlay_keep_mode_requested();

// True while the DFHack overlay plugin is enabled. Cheap; safe from any thread.
bool overlay_plugin_enabled();

// Render-thread code that cannot proceed without a viewscreen render calls this; the actual
// `disable overlay` runs later on DF's core thread from service_overlay_requests().
void request_overlay_disable(const std::string& reason);
void service_overlay_requests(DFHack::color_ostream& out);

} // namespace dwf
