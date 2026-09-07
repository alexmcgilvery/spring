/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PreGameMode.h"

#include "IPreGameServices.h"

namespace runtime {
bool PreGameMode::HandlesSession() const
{
	return true;
}

/**
 * Keep connection setup in its existing sync/FPU domain.
 *
 * Awaiting async setup or retiring pregame does not turn the original update's
 * true result into false. Exit requests and error codes are existing lifecycle
 * effects. No backing state is read after connection service can retire it.
 */
SessionUpdate PreGameMode::UpdateSession(Session&)
{
	auto& services = ConnectionServices();
	services.EnterSyncedCode();
	services.CheckFloatingPointControl();

	ServiceConnection(services);

	services.LeaveSyncedCode();
	return SessionUpdate::FromContinuation(true);
}

/**
 * Gate transport and packet consumption on published setup-task completion.
 *
 * GAMEDATA can schedule work midway through a packet batch. Polling before
 * every read prevents subsequent player assignment from seeing partial setup.
 * A transition ends this invocation before the backing object is touched again.
 */
void PreGameMode::ServiceConnection(IPreGameServices& services)
{
	if (services.HasPendingSetupTask())
		return;

	services.UpdateConnectionTransport();

	if (services.HandleConnectionTimeout())
		return;

	while (!services.HasPendingSetupTask()) {
		if (services.ProcessNextConnectionPacket() != PreGamePacketResult::Continue)
			return;
	}
}
}
