//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013-2016 SuperTuxKart-Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef STUNCLIENT_H
#define STUNCLIENT_H

#include "scriptinterface/ScriptInterface.h"

typedef struct _ENetHost ENetHost;

namespace StunClient
{

struct StunEndpoint {
	u32 ip;
	u16 port;
};

void SendStunRequest(ENetHost* transactionHost, u32 targetIp, u16 targetPort);

/**
 * Used for hosting.
 */
JS::Value FindStunEndpoint(ScriptInterface& scriptInterface, int port);

/**
 * Used for joining.
 */
StunEndpoint FindStunEndpoint(ENetHost* transactionHost);

void SendHolePunchingMessages(ENetHost* enetClient, const char* serverAddress, u16 serverPort);

}

#endif // STUNCLIENT_H
