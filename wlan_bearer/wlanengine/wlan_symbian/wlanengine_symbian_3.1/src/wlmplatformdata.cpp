/*
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Interface class to CenRep and PubSub
*
*/


// INCLUDE FILES
#include <e32base.h>
#include <etelmm.h>
#include <mmtsy_names.h>
#include <bt_subscribe.h>
#include <startupdomainpskeys.h>
#include <ctsydomainpskeys.h>
#include "wlaninternalpskeys.h"
#include "wlmplatformdata.h"
#include "am_debug.h"

// ================= MEMBER FUNCTIONS =======================

// C++ default constructor can NOT contain any code, that
// might leave.
//
CWlmPlatformData::CWlmPlatformData( MWlmSystemNotify& aCallback ) :
    iBtConnections( NULL ),
    iCallback( aCallback ),
    iSystemMode( EWlanSystemStartupInProgress ),
    iBtConnectionCount( 0 ),
    iCurrentIcon( EWlmIconStatusNotAvailable ),
    iIsStartupComplete( EFalse ),
    iIsInOffline( EFalse ),
    iIsEmergencyCall( EFalse )
    {
    DEBUG( "CWlmPlatformData::CWlmPlatformData()" );
    }

// Symbian 2nd phase constructor can leave.
void CWlmPlatformData::ConstructL()
    {
    DEBUG( "CWlmPlatformData::ConstructL()" );

    // Allow everyone to read the P&S properties.
    _LIT_SECURITY_POLICY_PASS(KWlmPSReadPolicy); //lint !e648
    // Require SID for writing the P&S properties.
    _LIT_SECURITY_POLICY_S0(KWlmPSWritePolicy, KPSUidWlan.iUid); //lint !e648

    // Create subscriber for system state
    iPropertySystemState = CWlmPlatformSubscriber::NewL(
        EWlmSubscribeTypePubSub, *this,
        KPSUidStartup, KPSGlobalSystemState );
    iPropertySystemState->IssueRequest();

    // Create subscriber for BT connections    
    iBtConnections = CWlmPlatformSubscriber::NewL(
        EWlmSubscribeTypePubSub, *this,
        KPropertyUidBluetoothCategory, KPropertyKeyBluetoothGetPHYCount );
    iBtConnections->IssueRequest();   

    // Create subscriber for Emergency Call Info.
    iEmergencyCall = CWlmPlatformSubscriber::NewL(
        EWlmSubscribeTypePubSub, *this,
        KPSUidCtsyEmergencyCallInfo, KCTSYEmergencyCallInfo );
    iEmergencyCall->IssueRequest();

    // Create PubSub property for publishing MAC address
    TInt ret( KErrNone );
    ret = RProperty::Define( KPSWlanMacAddress, KPSWlanMacAddressType,
        KWlmPSReadPolicy, KWlmPSWritePolicy, KPSWlanMacAddressLength );
    if ( ret != KErrAlreadyExists )
        {
        User::LeaveIfError( ret );
        }
    User::LeaveIfError( iPsMacAddress.Attach( KPSUidWlan,
       KPSWlanMacAddress, EOwnerThread ) );

    // Create PubSub property for publishing WLAN indicator information
    ret = RProperty::Define( KPSWlanIndicator, KPSWlanIndicatorType,
        KWlmPSReadPolicy, KWlmPSWritePolicy );
    if ( ret != KErrAlreadyExists )
        {
        User::LeaveIfError( ret );
        }
    User::LeaveIfError( iPsIndicator.Attach( KPSUidWlan,
       KPSWlanIndicator, EOwnerThread ) );
    }

// ---------------------------------------------------------
// CWlmPlatformData::NewL
// ---------------------------------------------------------
//
CWlmPlatformData* CWlmPlatformData::NewL( MWlmSystemNotify& aCallback )
    {
    CWlmPlatformData* self = new (ELeave) CWlmPlatformData( aCallback );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------
// CWlmPlatformData::~CWlmPlatformData
// ---------------------------------------------------------
//
CWlmPlatformData::~CWlmPlatformData()
    {
    DEBUG( "CWlmPlatformData::~CWlmPlatformData()" );

    iPsIndicator.Close();
    RProperty::Delete( KPSUidWlan, KPSWlanIndicator );
    iPsMacAddress.Close();
    RProperty::Delete( KPSUidWlan, KPSWlanMacAddress );
    delete iPropertySystemState;
    delete iBtConnections;
    delete iEmergencyCall;    
    }

// ---------------------------------------------------------
// CWlmPlatformData::SetIconState
// Status : Draft
// ---------------------------------------------------------
//
TInt CWlmPlatformData::SetIconState( TWlmIconStatus aStatus )
    {
    DEBUG( "CWlmPlatformData::SetIconState()" );

    TInt ret( KErrNone );
    if ( aStatus != iCurrentIcon )
        {
        switch ( aStatus )
            {
            case EWlmIconStatusNotAvailable:
                DEBUG( "Setting icon to EPSWlanIndicatorNone" );
                ret = iPsIndicator.Set( EPSWlanIndicatorNone );
                break;
            case EWlmIconStatusAvailable:
                DEBUG( "Setting icon to EPSWlanIndicatorAvailable" );
                ret = iPsIndicator.Set( EPSWlanIndicatorAvailable );
                break;
            case EWlmIconStatusConnected:
                DEBUG( "Setting icon to EPSWlanIndicatorActive" );
                ret = iPsIndicator.Set( EPSWlanIndicatorActive );
                break;
            case EWlmIconStatusConnectedSecure:
                DEBUG( "Setting icon to EPSWlanIndicatorActiveSecure" );
                ret = iPsIndicator.Set( EPSWlanIndicatorActiveSecure );
                break;
            default:
                DEBUG( "CWlmPlatformData::SetIconState() - unsupported status" );
                ret = KErrNotSupported;
                break;
            }        
        iCurrentIcon = aStatus;
        }
    return ret;
    }

// ---------------------------------------------------------
// CWlmPlatformData::HandlePropertyChangedL
// Status : Draft
// ---------------------------------------------------------
//
void CWlmPlatformData::HandlePropertyChangedL(
    const TUid& aCategory,
    const TUint aKey )
    {
    DEBUG( "CWlmPlatformData::HandlePropertyChangedL()" );
    DEBUG2( "CWlmPlatformData::HandlePropertyChangedL() - aCategory = %08x, aKey = %08x",
        aCategory.iUid, aKey );

    if ( aCategory == KPropertyUidBluetoothCategory &&
         aKey == KPropertyKeyBluetoothGetPHYCount )
        {
        TInt value( 0 );
        iBtConnections->Get( value );

        DEBUG2( "CWlmPlatformData::HandlePropertyChangedL() - BT count has changed from %u to %u",
            iBtConnectionCount, value );

        if ( !iBtConnectionCount && value )
            {
            DEBUG( "CWlmPlatformData::HandlePropertyChangedL() - BT connection established" );
            iCallback.BtConnectionEstablished();
            }
        else if ( iBtConnectionCount && !value )
            {
            DEBUG( "CWlmPlatformData::HandlePropertyChangedL() - BT connection disconnected" );
            iCallback.BtConnectionDisabled();
            }
        iBtConnectionCount = value;
        }
    else if ( aCategory == KPSUidStartup &&
              aKey == KPSGlobalSystemState )
        {
        TInt value( 0 );
        iPropertySystemState->Get( value );

        TWlanSystemMode enumValue( iSystemMode );

        switch ( value )
            {
            case ESwStateNormalRfOn:
                enumValue = EWlanSystemNormal;
                iIsStartupComplete = ETrue;
                iIsInOffline = EFalse;
                break;
            case ESwStateNormalRfOff:
                enumValue = EWlanSystemFlight;
                iIsStartupComplete = ETrue;
                iIsInOffline = ETrue;
                break;
            case ESwStateNormalBTSap:
                enumValue = EWlanSystemNormal;
                iIsStartupComplete = ETrue;
                iIsInOffline = EFalse;
                break;
            }

        /**
         * Do not change the SystemMode during emergency call,
         * it will be updated after emergency call.
         */
        if ( !iIsEmergencyCall && iSystemMode != enumValue )
            {
            DEBUG2( "CWlmPlatformData::HandlePropertyChangedL() - system mode has changed from %u to %u",
                iSystemMode, enumValue );

            iCallback.SystemModeChanged( iSystemMode, enumValue );
            iSystemMode = enumValue;            
            }        
        }
    else if( aCategory == KPSUidCtsyEmergencyCallInfo &&
        aKey == KCTSYEmergencyCallInfo )
        {
        TInt value( 0 );
        iEmergencyCall->Get( value );

        DEBUG1( "CWlmPlatformData::HandlePropertyChangedL() - KCTSYEmergencyCallInfo changed to %u",
            value );

        /**
        * Emergency call has been established
        */
        if( value && !iIsEmergencyCall)
            {
            iIsEmergencyCall = ETrue;
            TWlanSystemMode enumValue( EWlanSystemNormal );
            iCallback.EmergencyCallEstablished();
            DEBUG2( "CWlmPlatformData::HandlePropertyChangedL() - system mode has been changed from %u to %u",
                iSystemMode, enumValue );
            iSystemMode = enumValue;
            }
        /**
        * Emergency call has ended
        */
        else if (!value && iIsEmergencyCall )
            {
            iIsEmergencyCall = EFalse;
            /**
            * Startup is not yet done
            */
            if ( !iIsStartupComplete )
                {
                iCallback.EmergencyCallCompleted ( EWlanSystemStartupInProgress );
                DEBUG2( "CWlmPlatformData::HandlePropertyChangedL() - system mode has been changed from %u to %u",
                         iSystemMode, EWlanSystemStartupInProgress );
                iSystemMode = EWlanSystemStartupInProgress;
                }
            else 
                {
                TWlanSystemMode enumValue( EWlanSystemNormal );
                if ( iIsInOffline )
                    {
                    enumValue = EWlanSystemFlight;
                    }
                iCallback.EmergencyCallCompleted ( enumValue );
                DEBUG2( "CWlmPlatformData::HandlePropertyChangedL() - system mode has been changed from %u to %u",
                         iSystemMode, enumValue );
                iSystemMode = enumValue;
                }
            }
        }
    }

// ---------------------------------------------------------
// CWlmPlatformData::GetCurrentOperatorMccL
// Status : Draft
// ---------------------------------------------------------
//
void CWlmPlatformData::GetCurrentOperatorMccL( TUint& aCountryCode )
    {
    DEBUG( "CWlmPlatformData::GetCurrentOperatorMccL()" );

    RTelServer server;
    TInt error = server.Connect();
    DEBUG1( "Connect returned with %d", error );
    User::LeaveIfError( error );
    CleanupClosePushL( server );

    error = server.LoadPhoneModule( KMmTsyModuleName );
    DEBUG1( "LoadPhoneModule returned with %d", error );
    User::LeaveIfError( error );

    TInt numPhones( 0 );
    error = server.EnumeratePhones( numPhones );
    DEBUG1( "EnumeratePhones returned with %d", error );
    DEBUG1( "Number of phones enumerated = %d", numPhones );
    User::LeaveIfError( error );
    if ( !numPhones )
        {
        User::Leave( KErrNotFound );
        }

    RTelServer::TPhoneInfo phoneInfo;    
    error = server.GetPhoneInfo( 0, phoneInfo );
    DEBUG1( "GetPhoneInfo returned with %d", error );
    User::LeaveIfError( error );

    RMobilePhone phone;
    error = phone.Open( server, phoneInfo.iName );
    DEBUG1( "Open returned with %d", error );
    User::LeaveIfError( error );
    CleanupClosePushL( phone );

    TRequestStatus status;
    RMobilePhone::TMobilePhoneNetworkInfoV1 network;
    RMobilePhone::TMobilePhoneNetworkInfoV1Pckg networkPckg( network );
    RMobilePhone::TMobilePhoneLocationAreaV1 area;
    phone.GetCurrentNetwork( status, networkPckg, area );
    User::WaitForRequest( status );
    DEBUG1( "GetCurrentNetwork returned with %d", status.Int() );
    User::LeaveIfError( status.Int() );
   
    TLex lex( network.iCountryCode );
    User::LeaveIfError( lex.Val( aCountryCode ) );
   
    CleanupStack::PopAndDestroy( &phone );
    CleanupStack::PopAndDestroy( &server );

    DEBUG( "CWlmPlatformData::GetCurrentOperatorMccL() done" );
    }

// ---------------------------------------------------------
// CWlmPlatformData::UpdateSystemStatuses
// Status : Draft
// ---------------------------------------------------------
//
void CWlmPlatformData::UpdateSystemStatuses()
    {
    DEBUG( "CWlmPlatformData::UpdateSystemStatuses()" );

    TRAP_IGNORE( HandlePropertyChangedL( KPropertyUidBluetoothCategory,
        KPropertyKeyBluetoothGetPHYCount ) );
    TRAP_IGNORE( HandlePropertyChangedL( KPSUidStartup,
        KPSGlobalSystemState ) );
    }

// ---------------------------------------------------------
// CWlmPlatformData::PublishMacAddress
// Status : Draft
// ---------------------------------------------------------
//
TInt CWlmPlatformData::PublishMacAddress( TMacAddress& aMacAddr )
    {
    DEBUG( "CWlmPlatformData::PublishMacAddress()" );

    DEBUG( "BSSID:" );
    DEBUG_MAC( aMacAddr.iMacAddress );

    TPtrC8 mac( aMacAddr.iMacAddress, KPSWlanMacAddressLength );
    return iPsMacAddress.Set( mac );
    }
