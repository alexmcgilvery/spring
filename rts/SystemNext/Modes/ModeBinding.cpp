/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "ModeBinding.h"
namespace runtime {
ModeSelection ModeBinding::Select() { return {Resolve(), Generation()}; }
}
