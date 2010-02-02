/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:   Implementation of the WlanDeepPsModePowerModeMgr class
*
*/

/*
* %version: 3 %
*/

#include "config.h"
#include "umacdeeppsmodepowermodemgr.h"
#include "UmacContextImpl.h"


// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
WlanDeepPsModePowerModeMgr::WlanDeepPsModePowerModeMgr() 
    {
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
WlanDeepPsModePowerModeMgr::~WlanDeepPsModePowerModeMgr() 
    {
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TPowerMgmtModeChange WlanDeepPsModePowerModeMgr::OnFrameTx( 
    WlanContextImpl& /*aCtxImpl*/, 
    WHA::TQueueId /*aQueueId*/,
    TUint16 aEtherType,
    TBool /*aIgnoreThisFrame*/ )
    {
    TPowerMgmtModeChange powerMgmtModeChange( ENoChange );

    if ( aEtherType == KEapolType ||
         aEtherType == KWaiType )
        {
        powerMgmtModeChange = EToActive;

        OsTracePrint( KPwrStateTransition, (TUint8*)
            ("UMAC: WlanDeepPsModePowerModeMgr::OnFrameTx: EAPOL or WAI frame; change to Active") );            
        }
    else
        {
        powerMgmtModeChange = EToLightPs; 

        OsTracePrint( KPwrStateTransition, (TUint8*)
            ("UMAC: WlanDeepPsModePowerModeMgr::OnFrameTx: change to Light PS") );            
        }
    
    return powerMgmtModeChange;    
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TPowerMgmtModeChange WlanDeepPsModePowerModeMgr::OnFrameRx( 
    WlanContextImpl& /*aCtxImpl*/,
    WHA::TQueueId /*aAccessCategory*/,
    TUint16 aEtherType,
    TBool /*aIgnoreThisFrame*/,
    TUint /*aPayloadLength*/,
    TDaType aDaType ) 
    {
    TPowerMgmtModeChange powerMgmtModeChange( ENoChange );

    if ( aEtherType == KEapolType ||
         aEtherType == KWaiType )
        {
        powerMgmtModeChange = EToActive;

        OsTracePrint( KPwrStateTransition, (TUint8*)
            ("UMAC: WlanDeepPsModePowerModeMgr::OnFrameRx: EAPOL or WAI frame; change to Active") );            
        }
    else if ( aDaType == EBroadcastAddress ) 
        {
        // no action needed
        
        OsTracePrint( KPwrStateTransition, (TUint8*)
            ("UMAC: WlanDeepPsModePowerModeMgr::OnFrameRx: bcast frame; no state change") );            
        }
    else
        {
        powerMgmtModeChange = EToLightPs;        

        OsTracePrint( KPwrStateTransition, (TUint8*)
            ("UMAC: WlanDeepPsModePowerModeMgr::OnFrameRx: change to Light PS") );            
        }
    
    return powerMgmtModeChange;    
    }
