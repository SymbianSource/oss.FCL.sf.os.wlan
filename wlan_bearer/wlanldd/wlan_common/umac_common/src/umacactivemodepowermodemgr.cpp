/*
* Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:   Implementation of the WlanActiveModePowerModeMgr class
*
*/

/*
* %version: 13 %
*/

#include "config.h"
#include "umacactivemodepowermodemgr.h"
#include "UmacContextImpl.h"


// Default Rx/Tx frame count threshold for considering change to Light PS mode.
// This value is used if another value hasn't been provided
const TUint KDefaultToLightPsFrameThreshold = 1;


// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
WlanActiveModePowerModeMgr::WlanActiveModePowerModeMgr() : 
    iToLightPsFrameCount( 0 ),
    iToLightPsFrameThreshold( KDefaultToLightPsFrameThreshold )
    {
    }   
    
// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
WlanActiveModePowerModeMgr::~WlanActiveModePowerModeMgr() 
    {
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TPowerMgmtModeChange WlanActiveModePowerModeMgr::OnFrameTx( 
    WlanContextImpl& /*aCtxImpl*/, 
    WHA::TQueueId /*aQueueId*/,
    TUint16 aEtherType,
    TBool aIgnoreThisFrame )
    {
    if ( aEtherType == KEapolType ||
         aEtherType == KWaiType )
        {
        OsTracePrint( KPwrStateTransition, (TUint8*)
            ("UMAC: WlanActiveModePowerModeMgr::OnFrameTx: count this frame; eapol or WAI") );

        ++iToLightPsFrameCount; 
        }
    else
        {
        if ( aIgnoreThisFrame )
            { 
            // for these frames we don't increment the Tx counter
    
            OsTracePrint( KPwrStateTransition, (TUint8*)
                ("UMAC: WlanActiveModePowerModeMgr::OnFrameTx: do not count this frame") );
            }
        else
            {
            OsTracePrint( KPwrStateTransition, (TUint8*)
                ("UMAC: WlanActiveModePowerModeMgr::OnFrameTx: count this frame") );
    
            ++iToLightPsFrameCount;
            }
        }

    return ENoChange;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TPowerMgmtModeChange WlanActiveModePowerModeMgr::OnFrameRx(
    WlanContextImpl& aCtxImpl,
    WHA::TQueueId aAccessCategory,
    TUint16 aEtherType,
    TBool aIgnoreThisFrame,
    TUint aPayloadLength,
    TDaType aDaType )
    { 
    if ( CountThisFrame( 
            aCtxImpl, 
            aAccessCategory,
            aEtherType,
            aIgnoreThisFrame, 
            aPayloadLength, 
            iUapsdRxFrameLengthThreshold,
            aDaType ) ) 
        {
        ++iToLightPsFrameCount; 
        }
        
    return ENoChange;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool WlanActiveModePowerModeMgr::OnActiveToLightPsTimerTimeout()
    {
    OsTracePrint( KPwrStateTransition, (TUint8*)
        ("UMAC: WlanActiveModePowerModeMgr::OnActiveToLightPsTimerTimeout: ToLightPsFrameCount: %d"),
        iToLightPsFrameCount );

    if ( iToLightPsFrameCount < iToLightPsFrameThreshold )
        {
        return ETrue;
        }
    else
        {
        iToLightPsFrameCount = 0;
        return EFalse;
        }
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void WlanActiveModePowerModeMgr::DoReset()
    {
    OsTracePrint( KPwrStateTransition, (TUint8*)
        ("UMAC: WlanActiveModePowerModeMgr::DoReset()") );

    iToLightPsFrameCount = 0;
    }
