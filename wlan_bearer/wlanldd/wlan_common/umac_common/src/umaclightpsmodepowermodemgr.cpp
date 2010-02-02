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
* Description:   Implementation of the WlanLightPsModePowerModeMgr class
*
*/

/*
* %version: 3 %
*/

#include "config.h"
#include "umaclightpsmodepowermodemgr.h"
#include "UmacContextImpl.h"
#include "802dot11.h"

// Default Rx/Tx frame count threshold for considering change to Active mode.
// This value is used if another value hasn't been provided
const TUint KDefaultToActivePacketThreshold = 4;     

// Default Rx/Tx frame count threshold for considering change to Deep PS mode.
// This value is used if another value hasn't been provided
const TUint KDefaultToDeepPsPacketThreshold = 1;     

// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
WlanLightPsModePowerModeMgr::WlanLightPsModePowerModeMgr() : 
    iToActiveFrameCount( 0 ),
    iToActiveFrameThreshold( KDefaultToActivePacketThreshold ),
    iToDeepPsFrameCount( 0 ),
    iToDeepPsFrameThreshold( KDefaultToDeepPsPacketThreshold )
    {
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
WlanLightPsModePowerModeMgr::~WlanLightPsModePowerModeMgr() 
    {
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TPowerMgmtModeChange WlanLightPsModePowerModeMgr::OnFrameTx( 
    WlanContextImpl& /*aCtxImpl*/, 
    WHA::TQueueId /*aQueueId*/,
    TUint16 aEtherType,
    TBool aIgnoreThisFrame )
    {
    TPowerMgmtModeChange powerMgmtModeChange( ENoChange );

    if ( aEtherType == KEapolType ||
         aEtherType == KWaiType )
        {
        powerMgmtModeChange = EToActive;

        OsTracePrint( KPwrStateTransition, (TUint8*)
            ("UMAC: WlanLightPsModePowerModeMgr::OnFrameTx: EAPOL or WAI frame; change to Active") );            
        }
    else
        {
        if ( !aIgnoreThisFrame )
            {
            OsTracePrint( KPwrStateTransition, (TUint8*)
                ("UMAC: WlanLightPsModePowerModeMgr::OnFrameTx: count this frame") );

            ++iToActiveFrameCount;
            ++iToDeepPsFrameCount;
            
            if ( iToActiveFrameCount >= iToActiveFrameThreshold )
                {
                powerMgmtModeChange = EToActive;

                OsTracePrint( KPwrStateTransition, (TUint8*)
                    ("UMAC: WlanLightPsModePowerModeMgr::OnFrameTx: threshold exceeded; change to Active") );            
                }
            }
        else
            {
            OsTracePrint( KPwrStateTransition, (TUint8*)
                ("UMAC: WlanLightPsModePowerModeMgr::OnFrameTx: do no count this frame") );
            
            // no further action needed
            }
        }
    
    return powerMgmtModeChange;    
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TPowerMgmtModeChange WlanLightPsModePowerModeMgr::OnFrameRx( 
    WlanContextImpl& aCtxImpl,
    WHA::TQueueId aAccessCategory,
    TUint16 aEtherType,
    TBool aIgnoreThisFrame,
    TUint aPayloadLength,
    TDaType aDaType ) 
    {
    TPowerMgmtModeChange powerMgmtModeChange( ENoChange );

    if ( aEtherType == KEapolType ||
         aEtherType == KWaiType )
        {
        powerMgmtModeChange = EToActive;

        OsTracePrint( KPwrStateTransition, (TUint8*)
            ("UMAC: WlanLightPsModePowerModeMgr::OnFrameRx: EAPOL or WAI frame; change to Active") );            
        }
    else
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
            ++iToActiveFrameCount;
            ++iToDeepPsFrameCount;
            
            if ( iToActiveFrameCount >= iToActiveFrameThreshold )
                {
                powerMgmtModeChange = EToActive;

                OsTracePrint( KPwrStateTransition, (TUint8*)
                    ("UMAC: WlanLightPsModePowerModeMgr::OnFrameRx: threshold exceeded; change to Active") );            
                }
            }
        }
    
    return powerMgmtModeChange;    
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool WlanLightPsModePowerModeMgr::OnLightPsToActiveTimerTimeout(
    WlanContextImpl& aCtxImpl )
    {
    OsTracePrint( KPwrStateTransition, (TUint8*)
        ("UMAC: WlanLightPsModePowerModeMgr::OnLightPsToActiveTimerTimeout: ToActiveFrameCount: %d"),
        iToActiveFrameCount );

    iToActiveFrameCount = 0;
    
    if ( // we are seeking for the opportunity to perform tests on the 
         // current AP
         aCtxImpl.ApTestOpportunitySeekStarted() && 
         // we have not yet indicated to WLAN Mgmt Client the opportunity 
         // to perform tests on the current AP
         !aCtxImpl.ApTestOpportunityIndicated() )
        {
        // Inform WLAN Mgmt Client that this would be the time to start 
        // the testing
        aCtxImpl.iUmac.OnInDicationEvent( EApTestOpportunity );
        // make a note that AP test opportunity has been indicated to 
        // WLAN Mgmt Client
        aCtxImpl.ApTestOpportunityIndicated( ETrue );
        }
    
    return EFalse;
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
TBool WlanLightPsModePowerModeMgr::OnLightPsToDeepPsTimerTimeout()
    {
    OsTracePrint( KPwrStateTransition, (TUint8*)
        ("UMAC: WlanLightPsModePowerModeMgr::OnLightPsToDeepPsTimerTimeout: ToDeepPsFrameCount: %d"),
        iToDeepPsFrameCount );

    if ( iToDeepPsFrameCount < iToDeepPsFrameThreshold )
        {
        return ETrue;
        }
    else
        {
        iToDeepPsFrameCount = 0;
        return EFalse;
        }
    }

// ---------------------------------------------------------------------------
// 
// ---------------------------------------------------------------------------
//
void WlanLightPsModePowerModeMgr::DoReset()
    {
    OsTracePrint( KPwrStateTransition, (TUint8*)
        ("UMAC: WlanLightPsModePowerModeMgr::DoReset()") );

    iToActiveFrameCount = 0;
    iToDeepPsFrameCount = 0;
    }
