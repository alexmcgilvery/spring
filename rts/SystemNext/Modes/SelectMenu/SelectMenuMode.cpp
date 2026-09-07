/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SelectMenuMode.h"

namespace runtime {
/**
 * Record input/activation blocks which remain backed by the existing GUI.
 *
 * This is a documentation boundary, not a second event dispatcher. Existing
 * callbacks retain their implementation until their lifetime fixes are settled.
 * No callable placeholder is supplied for an unimplemented action.
 *
 * ActivateSelection:
 *   SelectMenu construction attaches its background GUI tree; mounts MenuArchive
 *   in the selection VFS; chooses a background with guRNG; constructs selection
 *   widgets from LastSelected* settings; opens connection UI for non-host setup.
 *   SelectionWidget::UpdateAvailableScripts also mounts selected game/map content
 *   temporarily to enumerate Lua AI choices. None of this belongs in frame draw.
 *
 * SelectLocalGame:
 *   SelectMenu::Single first prompts for missing game, map, then script. Once
 *   complete, its pregame guard suppresses duplicate starts. Sandbox selection
 *   becomes an empty script, pregame is constructed, setup-script execution is
 *   queued, and GUI removal is requested. Keep setup generation after construction
 *   unless its exception and side-effect ordering is deliberately changed.
 *
 * SelectReplay / SelectSave:
 *   SelectionWidget closes its list before invoking the selected-file callback.
 *   SelectMenu checks pregame, mutates shared ClientSetup, constructs pregame,
 *   queues LoadDemoFile/LoadSaveFile, then requests GUI removal. Replay additionally
 *   appends " (spec)" to the player name. Keep the existing demos/ and Saves/
 *   prefixes and asynchronous argument-copy behavior at the existing boundary.
 *
 * ConnectToServer:
 *   SelectMenu::DirectConnect saves the address, changes shared setup to client,
 *   constructs pregame (which starts client connection), then requests removal.
 *
 * EditSettings:
 *   Keep natural-sort listing, LastSelectedSetting persistence, dialog replacement,
 *   " = " parsing, ConfigHandler mutation, and focus restoration in existing
 *   SelectMenu/SettingsWindow callbacks. ConfigHandler::Update remains host work.
 *
 * ExitSelection:
 *   Quit sets globalQuit and queues menu removal. Escape invokes Quit but returns
 *   unhandled from HandleEventSelf; Return invokes Single and returns handled.
 *   Preserve that distinction and the GUI input handler's foreground ordering.
 */

//FIXME [SELECT-MENU-04] SelectMenu::DirectConnect has no pregame-null guard,
// unlike Single and the replay/save callbacks. A repeated callback can construct
// another CPreGame while clientNet already exists (its constructor asserts null).
// Test duplicate connection submission and define a separately reviewed remedy;
// do not silently normalize all start actions while extracting this mode.

//FIXME [SELECT-MENU-05] SelectMenu windows and SelectionWidget store callbacks
// capturing their owners. CPreGame construction immediately changes activeController,
// but Gui::RmElement only schedules deletion, so old callbacks can remain dispatchable.
// A binding must distinguish mode departure from GUI retirement and never access a
// backing menu after Gui::Clean. Validate a fake pregame replacement, another input
// event in the same batch, destructor-enqueued removals, and exception paths where
// construction/AsyncExecute succeeds partially before removal is requested.

//FIXME [SELECT-MENU-06] SelectionWidget::AddAIScriptsFromArchive changes VFS
// mappings under manual GrabLock/FreeLock, and SelectMenu construction changes the
// VFS name while loading its background. Exceptions can interrupt these sequences.
// Background loading, AI enumeration and widget construction must stay in activation/
// input work, not visual preparation or a worker. Audit exception recovery and lock/
// mapping restoration before extracting activation; do not add a second RNG draw.

//FIXME [SELECT-MENU-07] Gui::~Gui only calls Clean, which deletes queued removals
// rather than every remaining root element. SpringApp::Kill does not directly delete
// SelectMenu. Direct OS exit can bypass SelectMenu::Quit and its removal request.
// Establish a real quit/error shutdown fixture and explicit root ownership before
// claiming teardown is covered by mode departure. Do not add double ownership to
// the mode as a workaround for GUI shutdown behavior.

/**
 * Leave authoritative advancement to the mode selected by a start action.
 * A local-game, replay, save or connection selection creates pregame; it does
 * not make the selection menu itself responsible for servicing that session.
 */
bool SelectMenuMode::HandlesSession() const
{
	return false;
}
}
