/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

//FIXME [SELECT-FLOW-01] Bind operations to the mode only after GUI retirement
// and input routing are resolved. This disabled candidate owns no GUI/controller
// state; compilation of the block-out does not type-check this draft.
#if 0
#include "ISelectMenuServices.h"
namespace runtime {
class LegacySelectMenuServices final : public ISelectMenuServices {
public:
	void WaitForMenuFrame() override;
	void AdvanceDrawCounter() override;
	void ClearMenuScreen() override;
	void DrawMenuInterface() override;
};
}
#endif
