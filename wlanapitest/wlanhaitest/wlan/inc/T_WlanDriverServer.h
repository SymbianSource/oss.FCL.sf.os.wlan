/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/



#ifndef T_WLAN_DRIVER_SERVER_H
#define T_WLAN_DRIVER_SERVER_H

//Epoc includes
#include <testserver2.h>

/**
 * This wrapper class extends the test server and creates test server for Wlan driver
 */
class CT_WlanDriverServer : public CTestServer2
	{
private:
	class CT_WlanDriverBlock : public CTestBlockController
		{
	public:
		inline CT_WlanDriverBlock();
		inline ~CT_WlanDriverBlock();

		CDataWrapper* CreateDataL( const TDesC& aData );
		};

public:
	static CT_WlanDriverServer* NewL();
	inline CTestBlockController* CreateTestBlock();
	};

#include "t_wlandriverserver.inl"

#endif // T_WLAN_DRIVER_SERVER_H
