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



#ifndef T_RSOCKETSERVDATA_H_
#define T_RSOCKETSERVDATA_H_

//User Includes
#include "datawrapperbase.h"

//Epoc includes
#include <es_sock.h> // RSocketServ, RConnection

class CT_RSocketServData: public CDataWrapperBase
	{
public:
	static CT_RSocketServData* NewL();
	virtual ~CT_RSocketServData();

public:	
	virtual TAny* GetObject();
	virtual TBool DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);		
	void SetIapID(TUint32 );
	TUint32 GetIapID(){return iIapID;}

protected:
	CT_RSocketServData();
	void ConstructL();

private:
	void DoCmdSetOutgoingIap(const TTEFSectionName& aSection);
	void DoCmdConnect();
	void DoCmdClose();
	void Close();
	
private:
	/**
	 * Wrapped object
	 */
	RSocketServ*	iSocketServ;
	/**
	 * Flag to review RSocketServ is in Connected state
	 */
	TBool			iSocketServConnected;
	/**
	 * Store de ID of the IAP
	 */
	TUint32			iIapID;
	
	};
#endif /*T_RSOCKETSERVDATA_H_*/
