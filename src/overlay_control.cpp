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

#include "overlay_control.h"

#include "diagnostics.h"

#include "Core.h"
#include "PluginManager.h"

#include <atomic>
#include <fstream>
#include <mutex>

namespace dwf {
namespace {

std::mutex g_overlay_mutex;
bool g_overlay_disabled_by_dwf = false;

constexpr const char* kDisableOverlayFlagFile = "dfcapture_disable_overlay.txt";
std::atomic<bool> g_overlay_disable_requested(false);
std::mutex g_overlay_request_mutex;
std::string g_overlay_disable_reason;

} // namespace

bool disable_overlay_for_stream(DFHack::color_ostream& out, std::string* note) {
    std::lock_guard<std::mutex> lock(g_overlay_mutex);

    auto& core = DFHack::Core::getInstance();
    auto* plugins = core.getPluginManager();
    DFHack::Plugin* overlay = plugins ? plugins->getPluginByName("overlay") : nullptr;
    if (!overlay || !overlay->can_set_enabled() || !overlay->is_enabled()) {
        if (note) note->clear();
        return true;
    }

    auto rc = core.runCommand(out, "disable overlay");
    if (rc != DFHack::CR_OK || overlay->is_enabled()) {
        if (note) *note = "could not disable DFHack overlay plugin";
        return false;
    }

    g_overlay_disabled_by_dwf = true;
    diagnostics_log("DIAG: disabled DFHack overlay while dwf stream is running; "
                    "offscreen viewscreen rendering would otherwise invoke overlay Lua.");
    if (note) {
        *note = "DFHack overlay was disabled while dwf is streaming; "
                "it will be restored when the stream stops.";
    }
    return true;
}

void restore_overlay_after_stream(DFHack::color_ostream* out) {
    std::lock_guard<std::mutex> lock(g_overlay_mutex);
    if (!g_overlay_disabled_by_dwf)
        return;

    auto& core = DFHack::Core::getInstance();
    if (!core.isValid()) {
        g_overlay_disabled_by_dwf = false;
        diagnostics_log("DIAG: skipped DFHack overlay restore during DF shutdown.");
        return;
    }
    auto* plugins = core.getPluginManager();
    DFHack::Plugin* overlay = plugins ? plugins->getPluginByName("overlay") : nullptr;
    if (overlay && overlay->can_set_enabled() && !overlay->is_enabled()) {
        DFHack::color_ostream& con = out ? *out : core.getConsole();
        auto rc = core.runCommand(con, "enable overlay");
        if (rc != DFHack::CR_OK || !overlay->is_enabled()) {
            diagnostics_log("WARN: dwf could not restore DFHack overlay after stream stop.");
            return;
        }
    }

    g_overlay_disabled_by_dwf = false;
    diagnostics_log("DIAG: restored DFHack overlay after dwf stream stop.");
}

bool overlay_keep_mode_requested() {
#ifdef _WIN32
    std::ifstream flag(kDisableOverlayFlagFile);
    return !flag.good();
#else
    // The Linux capture path renders the host viewscreen directly and still needs the
    // overlay plugin disabled.
    return false;
#endif
}

bool overlay_plugin_enabled() {
    auto* plugins = DFHack::Core::getInstance().getPluginManager();
    DFHack::Plugin* overlay = plugins ? plugins->getPluginByName("overlay") : nullptr;
    return overlay && overlay->is_enabled();
}

void request_overlay_disable(const std::string& reason) {
    {
        std::lock_guard<std::mutex> lock(g_overlay_request_mutex);
        if (g_overlay_disable_reason.empty())
            g_overlay_disable_reason = reason;
    }
    g_overlay_disable_requested.store(true);
}

void service_overlay_requests(DFHack::color_ostream& out) {
    if (!g_overlay_disable_requested.exchange(false))
        return;
    std::string reason;
    {
        std::lock_guard<std::mutex> lock(g_overlay_request_mutex);
        reason.swap(g_overlay_disable_reason);
    }
    std::string note;
    if (disable_overlay_for_stream(out, &note)) {
        diagnostics_log("DIAG: keep-overlay mode fell back to disabling DFHack overlay: " + reason);
        // color_ostream::print is fmt-style; stream the text so the reason is never parsed.
        out << "dwf: DFHack overlay disabled for this stream (" << reason << ")." << std::endl;
    } else {
        diagnostics_log("WARN: keep-overlay fallback could not disable DFHack overlay: " + note);
    }
}

} // namespace dwf
