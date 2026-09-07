/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {

enum class PreGamePacketResult {
	NoPacket,
	Continue,
	Stop,
};

/**
 * Extraction seams for connection establishment and its connection screen.
 *
 * This is an abstract research contract, not a binding to a live controller.
 * Implementations must reuse the corresponding operations in CPreGame without
 * forwarding its complete Update or Draw methods. Service lifetime must exceed
 * an invocation even when connection processing retires the backing controller.
 */
class IPreGameServices {
public:
	virtual ~IPreGameServices() = default;

	/** Keep the existing explicit sync/FPU scope around connection processing. */
	virtual void EnterSyncedCode() = 0;
	virtual void CheckFloatingPointControl() = 0;
	virtual void LeaveSyncedCode() = 0;

	/**
	 * Poll without waiting; consume a ready future to publish worker completion
	 * and propagate its exception before transport or another packet is read.
	 * Reuse CPreGame::HasPendingAsyncTask, including its get-before-reset order.
	 */
	virtual bool HasPendingSetupTask() = 0;
	virtual void UpdateConnectionTransport() = 0;

	/**
	 * Reuse CheckTimeout(0, true) and its existing menu/exit-code/globalQuit path.
	 * Return true after either timeout outcome; the caller must do no further
	 * controller work. False means packet processing may begin.
	 */
	virtual bool HandleConnectionTimeout() = 0;

	/**
	 * Read one packet with the current gs->frameNum, preserving stream order.
	 * NoPacket ends draining. Continue permits another future poll then read.
	 * Stop means a terminal transition occurred; do not inspect retired state.
	 *
	 * Extract the switch in CPreGame::UpdateClientNet once. Preserve its local
	 * catches for malformed rejection/player packets, warnings for unknown
	 * packets, and content_error propagation for invalid setup/player numbers.
	 * GAMEDATA starts the existing owning-packet async task; the next poll must
	 * run before SETPLAYERNUM can observe its writes. SETPLAYERNUM sends client
	 * data and checksum messages before starting loading and retiring pregame.
	 */
	virtual PreGamePacketResult ProcessNextConnectionPacket() = 0;

	/** The headless path skips all drawing operations but still allows present. */
	virtual bool HasConnectionScreen() const = 0;
	virtual void ClearConnectionScreen() = 0;
	virtual void BeginConnectionText() = 0;
	virtual void DrawConnectionStatus() = 0;
	virtual void DrawConnectionIdentity() = 0;
	virtual void DrawArchiveChecksumProgress() = 0;
	virtual void DrawAbortInstructionsAndCredits() = 0;
	virtual void EndConnectionText() = 0;
};

//FIXME PRE-001: CPreGame::AsyncExecute and the GAMEDATA branch capture raw this.
// CPreGame::~CPreGame cleans GUI before member destruction, and modFileName is
// destroyed before pendingTask (reverse declaration order). A future's final
// wait cannot protect members that were already destroyed. LoadSaveFile can
// also delete pregame from its own worker on a bad-save/menu transition. Before
// implementing this service, establish external task ownership and retirement
// ordering; validate cancellation and worker-requested retirement without
// self-wait, races, or freeing setup/VFS dependencies still used by the task.

//FIXME PRE-002: UpdateClientNet timeout, rejection and SETPLAYERNUM paths can
// delete pregame inside an operation. Stop is a proposed boundary, not proof
// the legacy functions can safely supply it. Bind with no cached CPreGame
// reference across a transition, and return facts computed before retirement.
// Test menu and loading replacements (including nested loading callbacks), and
// preserve globalQuit/exitCode effects even though CPreGame::Update returns true.

//FIXME PRE-003: CPreGame::Update and GameDataReceived explicitly leave synced
// code only on normal completion. Their exceptions currently bypass LEAVE.
// Introducing an RAII scope would change that behavior. Keep the explicit
// boundaries until SYNCDEBUG/SYNCCHECK exception tests establish the intended
// outer-handler cleanup; do not silently promise exception-safe restoration.

//FIXME PRE-004: UpdateClientNet reads inbuf[0] in its zero-length warning and
// reads packet->data[1] for SETPLAYERNUM without a local length check. Audit
// protocol ingress validation before extraction. Preserve accepted valid input
// ordering, and separately test malformed packets rather than copying a possible
// out-of-bounds read or silently changing rejection policy in the new seam.

//FIXME PRE-005: Draw reads clientSetup, connection state and archive hashing
// progress while setup work can run asynchronously. These are live reads, not
// a render snapshot. GetNumFilesHashed is already atomic; audit the other
// accessors and object lifetime before moving visual execution. Copying values
// alone does not make concurrent reads race-free.

/**
 * Startup work is separate from per-iteration connection service.
 *
 * Current initiation paths (SpringApp and SelectMenu) call AsyncExecute with:
 * - LoadSetupScript: parse setup, select RNG seed, optionally generate blank
 *   map, mount map archives, run start-position Lua, calculate checksums, then
 *   construct the server and add the local client.
 * - LoadDemoFile: apply DemoFromDemo, read the first game-data packet and setup,
 *   rewrite the replay script, construct a replay server and add a local client.
 * - LoadSaveFile: construct the save handler and read startup information; start
 *   the server with its script, or report incompatible save and retire/quit.
 * A joining client starts transport directly in the CPreGame constructor.
 *
 * A received GAMEDATA task has its own ordered blocks:
 * enter synced code -> decode game data -> seed synced RNG -> reset/load setup
 * -> load global/player/team state -> validate player/team references -> reset
 * hashing progress -> mount map, mutators and game -> compare checksums and
 * prepare messages -> decide demo recording -> attach recorder -> leave synced.
 * These paths share CPreGame-owned setup state; they are not alternate modes.
 * No new startup methods are executable until task and ownership seams below
 * are resolved. Existing startup functions are reuse candidates, not no-op
 * implementations of this research contract.
 */

//FIXME PRE-006: The saveFileHandler raw pointer crosses LoadSaveFile ->
// SETPLAYERNUM -> CLoadScreen -> CGame, whose destructor deletes it before VFS
// teardown. CPreGame's destructor does not delete it. Document exactly when
// ownership transfers and how cancellation or construction failure disposes of
// it before binding startup operations; test failure before and after loading
// creation without double-delete, leak, or early VFS dependency destruction.

//FIXME PRE-007: AsyncExecute initializes streflop on its worker, whereas the
// GAMEDATA async call enters GameDataReceived directly, which enters synced code
// but does not repeat that initialization. Do not unify these worker entry
// points by assumption. Record thread/FPU state and seeded setup checksums for
// host, join, demo and save before choosing a shared worker execution scope.

}
