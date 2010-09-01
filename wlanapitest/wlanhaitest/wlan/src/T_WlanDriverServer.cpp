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



#include "t_wlandriverserver.h"
#include "t_wlanmgmtclientdata.h"
#include "t_wlanscaninfodata.h"
#include "t_rsocketservdata.h"
#include "t_ccommsdatabasedata.h"
#include "t_ccommsdbtableviewdata.h"
#include "t_rconnectiondata.h"
#include "t_rfiledata.h"
#include "t_rsocketdata.h"


/*@{*/
_LIT(KT_WlanMgmtClient, 		"WlanMgmtClient");
_LIT(KT_WlanScanInfo,			"WlanScanInfo");
_LIT(KT_WlanSocketServ,			"RSocketServ");
_LIT(KT_CCommsDB,				"CCommsDatabase");
_LIT(KT_CCommsDBTable,			"CCommsDbTableView");
_LIT(KT_RConnection,			"RConnection");
_LIT(KT_RSocket,				"RSocket");
_LIT(KT_RFile,				    "RFile");
/*@}*/


/**
 * 
 * Same code for Secure and non-secure variants
 * Called inside the MainL() function to create and start the
 * CTestServer derived server.
 * @return - Instance of the test server
 */
CT_WlanDriverServer* CT_WlanDriverServer::NewL()
	{
    CT_WlanDriverServer* server = new (ELeave) CT_WlanDriverServer();
    CleanupStack::PushL(server);
    server->ConstructL();
    CleanupStack::Pop(server);
    return server;
    }

/**
 * Secure variant
 * Much simpler, uses the new Rendezvous() call to sync with the client
 */
LOCAL_C void MainL()
	{
#if (defined __DATA_CAGING__)
    RProcess().DataCaging(RProcess::EDataCagingOn);
    RProcess().SecureApi(RProcess::ESecureApiOn);
#endif
    CActiveScheduler* sched = NULL;
    sched = new(ELeave) CActiveScheduler;
    CActiveScheduler::Install(sched);
    CT_WlanDriverServer* server = NULL;

    // Create the CTestServer derived server
    TRAPD(err, server = CT_WlanDriverServer::NewL());
    if(!err)
	    {
        // Sync with the client and enter the active scheduler
        RProcess::Rendezvous(KErrNone);
        sched->Start();
        }

    delete server;
    delete sched;
    }

/**
 * 
 * Secure variant only
 * Process entry point. Called by client using RProcess API
 * @return - Standard Epoc error code on process exit
 */
GLDEF_C TInt E32Main()
	{
    __UHEAP_MARK;
    CTrapCleanup* cleanup = CTrapCleanup::New();
    if(cleanup == NULL)
	    {
        return KErrNoMemory;
        }

#if (defined TRAP_IGNORE)
	TRAP_IGNORE(MainL());
#else
    TRAPD(err,MainL());
#endif

    delete cleanup;
    __UHEAP_MARKEND;
    return KErrNone;
    }
/*
 * Creates an instance of CDataWrapper that wraps a CT_WlanDriverData object 
 * @return wrapper	- a CDataWrapper instance that wraps the CT_WlanDriverData object
 */
CDataWrapper* CT_WlanDriverServer::CT_WlanDriverBlock::CreateDataL(const TDesC& aData)
	{
	CDataWrapper* wrapper = NULL;

	if( KT_WlanMgmtClient() == aData )
		{
		wrapper = CT_WlanMgmtClientData::NewL();
		}
	else if(KT_WlanScanInfo() == aData)
		{
		wrapper = CT_WlanScanInfoData::NewL();
		}
	else if(KT_WlanSocketServ() == aData)
		{
		wrapper = CT_RSocketServData::NewL();
		}
	else if(KT_CCommsDB() == aData)
		{
		wrapper = CT_CCommsDatabaseData::NewL();
		}
	else if(KT_CCommsDBTable() == aData)
		{
		wrapper = CT_CCommsDbTableViewData::NewL();
		}
	else if(KT_RConnection() == aData)
		{
		wrapper = CT_RConnectionData::NewL();
		}
	else if(KT_RSocket() == aData)
		{
		wrapper = CT_RSocketData::NewL();
		}
	else if(KT_RFile() == aData)
		{
		wrapper = CT_RFileData::NewL();
		}
	return wrapper;
	}
