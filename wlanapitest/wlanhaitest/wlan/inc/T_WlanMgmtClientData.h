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



#ifndef T_WLANMGMTCLIENTDATA_H_
#define T_WLANMGMTCLIENTDATA_H_

//User Includes
#include "datawrapperbase.h"

//forward 
class CWlanMgmtClient;

class CT_WlanMgmtClientData: public CDataWrapperBase
	{
public:
	static CT_WlanMgmtClientData* NewL();
	~CT_WlanMgmtClientData();
	
public:
	virtual TAny* GetObject();	
	virtual TBool DoCommandL(const TTEFFunction& aCommand, const TTEFSectionName& aSection, const TInt aAsyncErrorIndex);	
	
protected:
	CT_WlanMgmtClientData();
	void ConstructL();
	
private:
	void DoCmdNewL();	
	void DoCmdGetScanResults(const TTEFSectionName& aSection);	
	void DoCmdDestructor();
	void DestroyData();
	
private:
	/**
	 * Wrapped object
	 */
	CWlanMgmtClient* iData;

};


#endif /*T_WLANMGMTCLIENTDATA_H_*/