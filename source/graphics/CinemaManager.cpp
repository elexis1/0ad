/* Copyright (C) 2017 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include <sstream>
#include <string>

#include "graphics/Camera.h"
#include "graphics/CinemaManager.h"
#include "graphics/GameView.h"
#include "gui/CGUI.h"
#include "gui/GUIutil.h"
#include "gui/GUIManager.h"
#include "gui/IGUIObject.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/GameSetup/Config.h"
#include "ps/Hotkey.h"
#include "simulation2/components/ICmpCinemaManager.h"
#include "simulation2/components/ICmpOverlayRenderer.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/components/ICmpTerritoryManager.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/system/ComponentManager.h"
#include "simulation2/Simulation2.h"
#include "renderer/Renderer.h"


CCinemaManager::CCinemaManager()
	: m_DrawPaths(false)
{
}

void CCinemaManager::Update(const float deltaRealTime) const
{
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(g_Game->GetSimulation2()->GetSimContext().GetSystemEntity());
	if (!cmpCinemaManager)
		return;

	UpdateSessionVisibility();
	UpdateSilhouettesVisibility();

	if (IsPlaying())
		cmpCinemaManager->PlayQueue(deltaRealTime, g_Game->GetView()->GetCamera());
}

void CCinemaManager::Render() const
{
	DrawBars();
	DrawPaths();
}

void CCinemaManager::UpdateSessionVisibility() const
{
	// TODO: Enabling/Disabling does not work if the session GUI page is not the top page.
	// This can happen in various situations, for example when the player wins/loses the game
	// while the cinematic is running (a message box is the top page in this case).

	IGUIObject *sn = g_GUI->FindObjectByName("sn");
	if (!sn)
		return;

	bool hidden = false;
	GUI<bool>::GetSetting(sn, "hidden", hidden);

	if (hidden != IsPlaying())
		sn->SetSetting("hidden", IsPlaying() ? L"true" : L"false");
}

void CCinemaManager::UpdateSilhouettesVisibility() const
{
	if (!CRenderer::IsInitialised())
		return;

	bool silhouettes = false;
	CFG_GET_VAL("silhouettes", silhouettes);
	g_Renderer.SetOptionBool(CRenderer::Option::OPT_SILHOUETTES, !IsEnabled() && silhouettes);
}

void CCinemaManager::DrawPaths() const
{
	if (!m_DrawPaths)
		return;

	CmpPtr<ICmpCinemaManager> cmpCinemaManager(g_Game->GetSimulation2()->GetSimContext().GetSystemEntity());
	if (!cmpCinemaManager)
		return;

	for (const std::pair<CStrW, CCinemaPath>& p : cmpCinemaManager->GetPaths())
		p.second.Draw();
}

void CCinemaManager::DrawBars() const
{
	if (!IsEnabled())
		return;

	int height = (float)g_xres / 2.39f;
	int shift = (g_yres - height) / 2;
	if (shift <= 0)
		return;

#if CONFIG2_GLES
	#warning TODO : implement bars for GLES
#else
	// Set up transform for GL bars
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	CMatrix3D transform;
	transform.SetOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);
	glLoadMatrixf(&transform._11);

	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBegin(GL_QUADS);
	glVertex2i(0, 0);
	glVertex2i(g_xres, 0);
	glVertex2i(g_xres, shift);
	glVertex2i(0, shift);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2i(0, g_yres - shift);
	glVertex2i(g_xres, g_yres - shift);
	glVertex2i(g_xres, g_yres);
	glVertex2i(0, g_yres);
	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	// Restore transform
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
#endif
}

InReaction cinema_manager_handler(const SDL_Event_* ev)
{
	// put any events that must be processed even if inactive here
	if (!g_Game || !g_Game->IsGameStarted())
		return IN_PASS;

	CCinemaManager* pCinemaManager = g_Game->GetView()->GetCinema();

	return pCinemaManager->HandleEvent(ev);
}

InReaction CCinemaManager::HandleEvent(const SDL_Event_* ev) const
{
	switch (ev->ev.type)
	{
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		// Prevent selection of units while the path is playing
		if (IsPlaying())
			return IN_HANDLED;
		break;
	}
	return IN_PASS;
}

bool CCinemaManager::IsEnabled() const
{
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(g_Game->GetSimulation2()->GetSimContext().GetSystemEntity());
	return cmpCinemaManager && cmpCinemaManager->IsEnabled();
}

bool CCinemaManager::IsPlaying() const
{
	return IsEnabled() && g_Game && !g_Game->m_Paused;
}
