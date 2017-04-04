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

#include "simulation2/system/Component.h"
#include "ICmpCinemaManager.h"

#include "graphics/GameView.h"
#include "graphics/CinemaManager.h"
#include "gui/CGUI.h"
#include "gui/GUIManager.h"
#include "gui/IGUIObject.h"
#include "ps/ConfigDB.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "simulation2/components/ICmpOverlayRenderer.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/components/ICmpTerritoryManager.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/Simulation2.h"
#include "renderer/Renderer.h"


class CCmpCinemaManager : public ICmpCinemaManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update);
	}

	DEFAULT_COMPONENT_ALLOCATOR(CinemaManager)

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_Enabled = false;
		m_Paused = false;
		m_MapRevealed = false;
		m_ElapsedTime = fixed::Zero();
		m_TotalTime = fixed::Zero();
		m_CurrentPathElapsedTime = fixed::Zero();
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		serialize.Bool("MapRevealed", m_MapRevealed);
		serialize.NumberU32_Unbounded("NumberOfPaths", m_Paths.size());
		for (const std::pair<CStrW, CCinemaPath>& it : m_Paths)
		{
			const CCinemaPath& path = it.second;
			const CCinemaData* data = path.GetData();

			// TODO: maybe implement String_Unbounded
			serialize.String("PathName", data->m_Name, 1, 2048);
			serialize.String("PathOrientation", data->m_Orientation, 1, 2048);
			serialize.String("PathMode", data->m_Mode, 1, 2048);
			serialize.String("PathStyle", data->m_Style, 1, 2048);
			serialize.NumberFixed_Unbounded("PathTimescale", data->m_Timescale);
			serialize.Bool("LookAtTarget", data->m_LookAtTarget);

			serialize.NumberU32("NumberOfNodes", path.GetAllNodes().size(), 1, MAX_SPLINE_NODES);
			const std::vector<SplineData>& nodes = path.GetAllNodes();
			for (size_t i = 0; i < nodes.size(); ++i)
			{
				if (i > 0)
					serialize.NumberFixed_Unbounded("NodeDeltaTime", nodes[i - 1].Distance);
				else
					serialize.NumberFixed_Unbounded("NodeDeltaTime", fixed::Zero());
				serialize.NumberFixed_Unbounded("PositionX", nodes[i].Position.X);
				serialize.NumberFixed_Unbounded("PositionY", nodes[i].Position.Y);
				serialize.NumberFixed_Unbounded("PositionZ", nodes[i].Position.Z);

				serialize.NumberFixed_Unbounded("RotationX", nodes[i].Rotation.X);
				serialize.NumberFixed_Unbounded("RotationY", nodes[i].Rotation.Y);
				serialize.NumberFixed_Unbounded("RotationZ", nodes[i].Rotation.Z);
			}

			if (!data->m_LookAtTarget)
				continue;

			const std::vector<SplineData>& targetNodes = path.GetTargetSpline().GetAllNodes();
			serialize.NumberU32("NumberOfTargetNodes", targetNodes.size(), 1, MAX_SPLINE_NODES);
			for (size_t i = 0; i < targetNodes.size(); ++i)
			{
				if (i > 0)
					serialize.NumberFixed_Unbounded("NodeDeltaTime", targetNodes[i - 1].Distance);
				else
					serialize.NumberFixed_Unbounded("NodeDeltaTime", fixed::Zero());
				serialize.NumberFixed_Unbounded("PositionX", targetNodes[i].Position.X);
				serialize.NumberFixed_Unbounded("PositionY", targetNodes[i].Position.Y);
				serialize.NumberFixed_Unbounded("PositionZ", targetNodes[i].Position.Z);
			}
		}
	}

	virtual void Deserialize(const CParamNode& UNUSED(paramNode), IDeserializer& deserialize)
	{
		deserialize.Bool("MapRevealed", m_MapRevealed);
		uint32_t numberOfPaths = 0;
		deserialize.NumberU32_Unbounded("NumberOfPaths", numberOfPaths);
		for (uint32_t i = 0; i < numberOfPaths; ++i)
		{
			CCinemaData data;

			deserialize.String("PathName", data.m_Name, 1, 2048);
			deserialize.String("PathOrientation", data.m_Orientation, 1, 2048);
			deserialize.String("PathMode", data.m_Mode, 1, 2048);
			deserialize.String("PathStyle", data.m_Style, 1, 2048);
			deserialize.NumberFixed_Unbounded("PathTimescale", data.m_Timescale);
			deserialize.Bool("LookAtTarget", data.m_LookAtTarget);

			TNSpline pathSpline, targetSpline;
			uint32_t numberOfNodes = 0;
			deserialize.NumberU32("NumberOfNodes", numberOfNodes, 1, MAX_SPLINE_NODES);
			for (uint32_t j = 0; j < numberOfNodes; ++j)
			{
				SplineData node;
				deserialize.NumberFixed_Unbounded("NodeDeltaTime", node.Distance);

				deserialize.NumberFixed_Unbounded("PositionX", node.Position.X);
				deserialize.NumberFixed_Unbounded("PositionY", node.Position.Y);
				deserialize.NumberFixed_Unbounded("PositionZ", node.Position.Z);

				deserialize.NumberFixed_Unbounded("RotationX", node.Rotation.X);
				deserialize.NumberFixed_Unbounded("RotationY", node.Rotation.Y);
				deserialize.NumberFixed_Unbounded("RotationZ", node.Rotation.Z);

				pathSpline.AddNode(node.Position, node.Rotation, node.Distance);
			}

			if (data.m_LookAtTarget)
			{
				uint32_t numberOfTargetNodes = 0;
				deserialize.NumberU32("NumberOfTargetNodes", numberOfTargetNodes, 1, MAX_SPLINE_NODES);
				for (uint32_t j = 0; j < numberOfTargetNodes; ++j)
				{
					SplineData node;
					deserialize.NumberFixed_Unbounded("NodeDeltaTime", node.Distance);

					deserialize.NumberFixed_Unbounded("PositionX", node.Position.X);
					deserialize.NumberFixed_Unbounded("PositionY", node.Position.Y);
					deserialize.NumberFixed_Unbounded("PositionZ", node.Position.Z);

					targetSpline.AddNode(node.Position, CFixedVector3D(), node.Distance);
				}
			}

			// Construct cinema path with data gathered
			m_Paths[data.m_Name] = CCinemaPath(data, pathSpline, targetSpline);
		}
		SetEnabled(m_Enabled);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Update:
		{
			const CMessageUpdate &msgData = static_cast<const CMessageUpdate&>(msg);
			if (!m_Enabled)
				break;

			m_ElapsedTime += msgData.turnLength;
			m_CurrentPathElapsedTime += msgData.turnLength;
			if (m_CurrentPathElapsedTime >= m_PathQueue.front().GetDuration())
			{
				CMessageCinemaPathEnded msgCinemaPathEnded(m_PathQueue.front().GetName());
				m_PathQueue.pop_front();
				GetSimContext().GetComponentManager().PostMessage(SYSTEM_ENTITY, msgCinemaPathEnded);
				m_CurrentPathElapsedTime = fixed::Zero();
				if (!m_PathQueue.empty())
					m_PathQueue.front().Reset();
			}
			if (m_ElapsedTime >= m_TotalTime)
			{
				m_CurrentPathElapsedTime = fixed::Zero();
				m_ElapsedTime = fixed::Zero();
				m_TotalTime = fixed::Zero();
				SetEnabled(false);
				GetSimContext().GetComponentManager().PostMessage(SYSTEM_ENTITY, CMessageCinemaQueueEnded());
			}
			break;
		}
		default:
			break;
		}
	}

	virtual void AddPath(const CStrW& name, const CCinemaPath& path)
	{
		if (m_Paths.find(name) != m_Paths.end())
		{
			LOGWARNING("Path with name '%s' already exists", name.ToUTF8());
			return;
		}
		m_Paths[name] = path;
	}

	virtual void AddCinemaPathToQueue(const CStrW& name)
	{
		if (!HasPath(name))
		{
			LOGWARNING("Path with name '%s' doesn't exist", name.ToUTF8());
			return;
		}
		m_PathQueue.push_back(m_Paths[name]);

		if (m_PathQueue.size() == 1)
			m_PathQueue.front().Reset();
		m_TotalTime += m_Paths[name].GetDuration();
	}

	virtual void Play()
	{
		SetEnabled(true);
	}

	virtual void Stop()
	{
		SetEnabled(false);
	}

	virtual bool HasPath(const CStrW& name) const
	{
		return m_Paths.find(name) != m_Paths.end();
	}

	virtual void ClearQueue()
	{
		m_PathQueue.clear();
	}

	virtual const std::map<CStrW, CCinemaPath>& GetPaths() const
	{
		return m_Paths;
	}

	virtual void SetPaths(const std::map<CStrW, CCinemaPath>& newPaths)
	{
		m_Paths = newPaths;
	}

	virtual const std::list<CCinemaPath>& GetQueue() const
	{
		return m_PathQueue;
	}

	virtual bool IsEnabled() const
	{
		return m_Enabled;
	}

	virtual bool IsPaused() const
	{
		return m_Paused;
	}

	virtual void SetEnabled(bool enabled)
	{
		if (m_PathQueue.empty() && enabled)
			enabled = false;

		if (m_Enabled == enabled)
			return;

		// Overlay visibility
		if (CRenderer::IsInitialised())
		{
			bool silhouettes = false;
			CFG_GET_VAL("silhouettes", silhouettes);
			g_Renderer.SetOptionBool(CRenderer::Option::OPT_SILHOUETTES, !enabled && silhouettes);
		}

		CmpPtr<ICmpRangeManager> cmpRangeManager(GetSimContext().GetSystemEntity());
		CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(GetSimContext().GetSystemEntity());
		if (cmpRangeManager)
		{
			if (enabled)
				m_MapRevealed = cmpRangeManager->GetLosRevealAll(-1);
			// TODO: improve m_MapRevealed state and without fade in
			cmpRangeManager->SetLosRevealAll(-1, enabled);
		}
		if (cmpTerritoryManager)
			cmpTerritoryManager->SetVisibility(!enabled);
		ICmpSelectable::SetOverrideVisibility(!enabled);
		ICmpOverlayRenderer::SetOverrideVisibility(!enabled);

		m_Enabled = enabled;
	}

	virtual void PlayQueue(const float deltaRealTime, CCamera* camera)
	{
		if (m_PathQueue.empty())
			return;
		m_PathQueue.front().Play(deltaRealTime, camera);
	}

private:
	bool m_Enabled;
	bool m_Paused;
	std::map<CStrW, CCinemaPath> m_Paths;
	std::list<CCinemaPath> m_PathQueue;

	// States before playing
	bool m_MapRevealed;

	fixed m_ElapsedTime, m_TotalTime, m_CurrentPathElapsedTime;
};

REGISTER_COMPONENT_TYPE(CinemaManager)
