/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Path.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/UI/GuiHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/Threading/ThreadPool.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"



CPathTexture::CPathTexture()
: CPboInfoTexture("path")
, isCleared(false)
//, updateFrame(0)
, updateProcess(0)
, lastSelectedPathType(0)
, forcedPathType(-1)
, forcedUnitDef(-1)
, lastUsage(spring_gettime())
{
	texSize = int2(mapDims.hmapx, mapDims.hmapy);
	texChannels = 4;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, texSize.x, texSize.y);

	infoTexPBO.Bind();
	infoTexPBO.New(texSize.x * texSize.y * texChannels, GL_STREAM_DRAW);
	infoTexPBO.Unbind();

	if (FBO::IsSupported()) {
		fbo.Bind();
		fbo.AttachTexture(texture);
		/*bool status =*/ fbo.CheckStatus("CPathTexture");
		FBO::Unbind();
	}

	if (!fbo.IsValid()) {
		throw opengl_error("");
	}
}


enum BuildSquareStatus {
	NOLOS          = 0,
	FREE           = 1,
	OBJECTBLOCKED  = 2,
	TERRAINBLOCKED = 3,
};


static const SColor buildColors[] = {
	SColor(  0,   0,   0), // nolos
	SColor(  0, 255,   0), // free
	SColor(  0,   0, 255), // objblocked
	SColor(254,   0,   0), // terrainblocked
};


static inline const SColor& GetBuildColor(const BuildSquareStatus& status) {
	RECOIL_DETAILED_TRACY_ZONE;
	return buildColors[status];
}


static SColor GetSpeedModColor(const float sm) {
	RECOIL_DETAILED_TRACY_ZONE;
	SColor col(255, 0, 0);

	if (sm > 0.0f) {
		col.r = 255 - std::min(sm * 255.0f, 255.0f);
		col.g = 255 - col.r;
	} else {
		col.b = 255;
	}

	return col;
}


const MoveDef* CPathTexture::GetSelectedMoveDef()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (forcedPathType >= 0)
		return moveDefHandler.GetMoveDefByPathType(forcedPathType);

	const auto& unitSet = selectedUnitsHandler.selectedUnits;

	if (unitSet.empty())
		return nullptr;

	const auto iter = unitSet.begin();
	const CUnit* unit = unitHandler.GetUnit(*iter);
	return unit->moveDef;
}


const UnitDef* CPathTexture::GetCurrentBuildCmdUnitDef()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (forcedUnitDef >= 0)
		return unitDefHandler->GetUnitDefByID(forcedUnitDef);

	if ((unsigned)guihandler->inCommand > guihandler->commands.size())
		return nullptr;

	if (guihandler->commands[guihandler->inCommand].type != CMDTYPE_ICON_BUILDING)
		return nullptr;

	return unitDefHandler->GetUnitDefByID(-guihandler->commands[guihandler->inCommand].id);
}


GLuint CPathTexture::GetTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	lastUsage = spring_gettime();
	return texture;
}


bool CPathTexture::ShowMoveDef(const int pathType)
{
	RECOIL_DETAILED_TRACY_ZONE;
	forcedUnitDef  = -1;
	forcedPathType = pathType;
	updateProcess = 0;
	return true; // TODO: unused
}


bool CPathTexture::ShowUnitDef(const int udefid)
{
	RECOIL_DETAILED_TRACY_ZONE;
	forcedUnitDef  = udefid;
	forcedPathType = -1;
	updateProcess = 0;
	return true; // TODO: unused
}


bool CPathTexture::IsUpdateNeeded()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// don't update when not rendered/used
	if ((spring_gettime() - lastUsage).toSecsi() > 2) {
		forcedUnitDef = forcedPathType = -1;
		return false;
	}

	// newly build cmd active?
	const UnitDef* ud = GetCurrentBuildCmdUnitDef();

	if (ud != nullptr) {
		const unsigned int buildDefID = -(ud->id + 1);

		if (buildDefID != lastSelectedPathType) {
			lastSelectedPathType = buildDefID;
			updateProcess = 0;
			return true;
		}
	} else {
		// newly unit/moveType active?
		const MoveDef* md = GetSelectedMoveDef();

		if (md != nullptr) {
			const unsigned int pathType = md->pathType + 1;

			if (pathType != lastSelectedPathType) {
				lastSelectedPathType = pathType;
				updateProcess = 0;
				return true;
			}
		}
	}

	// nothing selected nor any build cmd active -> don't update
	return (lastSelectedPathType != 0 || !isCleared);
}


void CPathTexture::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const MoveDef* md = GetSelectedMoveDef();
	const UnitDef* ud = GetCurrentBuildCmdUnitDef();

	// just clear
	if (ud == nullptr && md == nullptr) {
		isCleared = true;
		updateProcess = 0;
		fbo.Bind();
		glViewport(0, 0, texSize.x, texSize.y);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		globalRendering->LoadViewport();
		FBO::Unbind();
		return;
	}

	// spread update across time
	constexpr int TEX_SIZE_TO_UPDATE_EACH_FRAME = 128*128;
	if (updateProcess >= texSize.y) updateProcess = 0;

	int start = updateProcess;
	const int updateLines = std::max(TEX_SIZE_TO_UPDATE_EACH_FRAME / texSize.x, ThreadPool::GetNumThreads());
	updateProcess += updateLines;
	updateProcess = std::min(updateProcess, texSize.y);

	// map PBO
	infoTexPBO.Bind();
	const size_t offset = start * texSize.x;
	SColor* infoTexMem = reinterpret_cast<SColor*>(infoTexPBO.MapBuffer(offset * sizeof(SColor), (updateProcess - start) * texSize.x * sizeof(SColor)));

	//FIXME make global func
	const bool losFullView = ((gu->spectating && gu->spectatingFullView) || losHandler->GetGlobalLOS(gu->myAllyTeam));

	if (ud != nullptr) {
		for_mt(start, updateProcess, [&](const int y) {
			int currentThread = ThreadPool::GetThreadNum();
			for (int x = 0; x < texSize.x; ++x) {
				const float3 pos = float3(x << 1, 0.0f, y << 1) * SQUARE_SIZE;
				const int idx = y * texSize.x + x;

				BuildSquareStatus status = FREE;
				BuildInfo bi(ud, pos, guihandler->buildFacing);
				bi.pos = CGameHelper::Pos2BuildPos(bi, false);

				CFeature* f = nullptr;

				if (CGameHelper::TestUnitBuildSquare(
						bi, f, gu->myAllyTeam, false, nullptr, nullptr, nullptr, nullptr, currentThread
					)) {
					if (f != nullptr) {
						status = OBJECTBLOCKED;
					}
				} else {
					status = TERRAINBLOCKED;
				}

				infoTexMem[idx - offset] = GetBuildColor(status);
			}
		});
	} else if (md != nullptr) {
		for_mt(start, updateProcess, [&](const int y) {
			int thread = ThreadPool::GetThreadNum();
			for (int x = 0; x < texSize.x; ++x) {
				const int2 sq = int2(x << 1, y << 1);
				const int idx = y * texSize.x + x;

				float scale = 1.0f;

				if (losFullView || losHandler->InLos(SquareToFloat3(sq), gu->myAllyTeam)) {
					if (CMoveMath::IsBlocked(*md, sq.x,     sq.y    , nullptr, thread) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
					if (CMoveMath::IsBlocked(*md, sq.x + 1, sq.y    , nullptr, thread) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
					if (CMoveMath::IsBlocked(*md, sq.x,     sq.y + 1, nullptr, thread) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
					if (CMoveMath::IsBlocked(*md, sq.x + 1, sq.y + 1, nullptr, thread) & CMoveMath::BLOCK_STRUCTURE) { scale -= 0.25f; }
				}

				// NOTE: raw speedmods are not necessarily clamped to [0, 1]
				const float sm = CMoveMath::GetPosSpeedMod(*md, sq.x, sq.y);
				infoTexMem[idx - offset] = GetSpeedModColor(sm * scale);
			}
		});
	}

	infoTexPBO.UnmapBuffer();
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, start, texSize.x, updateProcess - start, GL_RGBA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr(offset * sizeof(SColor)));
	infoTexPBO.Unbind();

	isCleared = false;
}
