/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  This class implements WLAN background scan logic.
*
*/

/*
* %version: 12 %
*/

#include <e32base.h>
#include <e32const.h> 

#include "wlanscanproviderinterface.h"
#include "awsinterface.h"
#include "awsenginebase.h"
#include "wlancbwaiter.h"
#include "wlanbgscanawscomms.h"
#include "wlanbgscan.h"
#include "wlandevicesettings.h" // default values in case invalid data is passed in NotifyChangedSettings
#include "am_debug.h"

/**
 * One day to be added to TTime.
 */
const TInt KAddOneDay = 1;

/**
 * Get hours from CenRep variable.
 */
const TInt KGetHours = 100;

/**
 * Zero seconds.
 */
const TInt KZeroSeconds = 0;

/**
 * Last minute of a day.
 */
const TInt KWlanBgScanTwentyThreeFiftyNineOclock = 2359;

/**
 * Maximun interval for background scan.
 */
const TInt KWlanBgScanMaxInterval = 1800;

#ifdef _DEBUG
/**
 * Formatting of date time debug string.
 */
_LIT( KWlanBgScanDateTimeFormat, "%F %*E %*N %D %H:%T:%S" );

/**
 * Maximun length for date time debug string.
 */
const TInt KWlanBgScanMaxDateTimeStrLen = 50;
#endif

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CWlanBgScan::CWlanBgScan
// ---------------------------------------------------------------------------
//
CWlanBgScan::CWlanBgScan( MWlanScanResultProvider& aProvider, CWlanTimerServices& aTimerServices ) :
    iProvider ( aProvider ),
    iCurrentBgScanInterval( 0 ),
    iAwsComms( NULL ),
    iBgScanState( EBgScanOff ),
    iAutoPeriod( EAutoPeriodNone ),
    iTimerServices( aTimerServices ),
    iIntervalChangeRequestId( 0 ),
    iBgScanPeakStartTime( 0 ),
    iBgScanPeakEndTime( 0 ),
    iBgScanIntervalPeak( 0 ),
    iBgScanIntervalOffPeak( 0 )
    {
    DEBUG( "CWlanBgScan::CWlanBgScan()" );
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::ConstructL
// ---------------------------------------------------------------------------
//
void CWlanBgScan::ConstructL()
    {
    DEBUG( "CWlanBgScan::ConstructL()" );
    
    // create AWS comms interface
    TRAPD( err, iAwsComms = CWlanBgScanAwsComms::NewL( *this ) );
    if( err != KErrNone )
        {
        if( iAwsComms )
            {
            delete iAwsComms;
            iAwsComms = NULL;
            }
        DEBUG1( "CWlanBgScan::ConstructL() - AWS comms creation failed with code %i", err );
        }
    else
        {
        DEBUG( "CWlanBgScan::ConstructL() - AWS comms creation successful" );
        }
        
    DEBUG( "CWlanBgScan::ConstructL() - done" );    
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::NewL
// ---------------------------------------------------------------------------
//
CWlanBgScan* CWlanBgScan::NewL( MWlanScanResultProvider& aProvider, CWlanTimerServices& aTimerServices )
    {
    DEBUG( "CWlanBgScan::NewL()" );
    CWlanBgScan* self = new ( ELeave ) CWlanBgScan( aProvider, aTimerServices );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;    
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::~CWlanBgScan
// ---------------------------------------------------------------------------
//
CWlanBgScan::~CWlanBgScan()
    {
    DEBUG( "CWlanBgScan::CWlanBgScan()" );
    }

// ---------------------------------------------------------------------------
// From class MWlanBgScanProvider.
// CWlanBgScan::ScanComplete
// ---------------------------------------------------------------------------
//
void CWlanBgScan::ScanComplete()
    {
    DEBUG1( "CWlanBgScan::ScanComplete() - current interval %us", GetInterval() );

    if ( GetInterval() != KWlanBgScanIntervalNever )
        {
        DEBUG1( "CWlanBgScan::ScanComplete() - issue a new request with %us as expiry", GetInterval() );
        iProvider.Scan( GetInterval() );
        }
    }

// ---------------------------------------------------------------------------
// From class MWlanBgScanProvider.
// CWlanBgScan::IntervalChanged
// ---------------------------------------------------------------------------
//
void CWlanBgScan::IntervalChanged( TUint32 aNewInterval )
    {
    DEBUG1( "CWlanBgScan::IntervalChanged() - aNewInterval %u", aNewInterval );

    NextState( aNewInterval );
            
    DEBUG2( "CWlanBgScan::IntervalChanged() - current interval %u, current state %u",
            GetInterval(),
            iBgScanState );
    }

// ---------------------------------------------------------------------------
// From class MWlanBgScanProvider.
// CWlanBgScan::NotConnected
// ---------------------------------------------------------------------------
//
void CWlanBgScan::NotConnected()
    {
    DEBUG1( "CWlanBgScan::NotConnected() - current interval %us", GetInterval() );
        
    if ( GetInterval() != KWlanBgScanIntervalNever )
        {
        DEBUG( "CWlanBgScan::NotConnected() - issue a new request with immediate expiry" );
        iProvider.Scan( KWlanBgScanMaxDelayExpireImmediately );
        }
    }

// ---------------------------------------------------------------------------
// From class MWlanBgScanProvider.
// CWlanBgScan::IsBgScanEnabled
// ---------------------------------------------------------------------------
//
TBool CWlanBgScan::IsBgScanEnabled()
    {
    // If ( interval != never )                              -> Return True
    // Otherwise                                             -> return False
    DEBUG1( "CWlanBgScan::IsBgScanEnabled() - returning %u", 
           ( GetInterval() != KWlanBgScanIntervalNever ) ? 1 : 0 );
        
    return ( GetInterval() != KWlanBgScanIntervalNever );
    }

// ---------------------------------------------------------------------------
// From class MWlanBgScanProvider.
// CWlanBgScan::NotifyChangedSettings
// ---------------------------------------------------------------------------
//
void CWlanBgScan::NotifyChangedSettings( MWlanBgScanProvider::TWlanBgScanSettings& aSettings )
    {
    DEBUG6( "CWlanBgScan::NotifyChangedSettings( interval: %u, psm srv mode: %u, peak start: %04u, peak end: %04u, peak: %u, off-peak: %u )",
            aSettings.backgroundScanInterval,
            aSettings.psmServerMode,
            aSettings.bgScanPeakStartTime,
            aSettings.bgScanPeakEndTime,
            aSettings.bgScanIntervalPeak,
            aSettings.bgScanIntervalOffPeak);
    
    MWlanBgScanProvider::TWlanBgScanSettings settingsToUse;
    CheckSettings( settingsToUse, aSettings );
    
    iBgScanPeakStartTime = settingsToUse.bgScanPeakStartTime;
    iBgScanPeakEndTime = settingsToUse.bgScanPeakEndTime;
    iBgScanIntervalPeak = settingsToUse.bgScanIntervalPeak;
    iBgScanIntervalOffPeak = settingsToUse.bgScanIntervalOffPeak;
    
    IntervalChanged( settingsToUse.backgroundScanInterval );
        
    if( IsAwsPresent() )
        {
        CWlanBgScanAwsComms::TAwsMessage msg = { CWlanBgScanAwsComms::ESetPowerSaveMode, aSettings.psmServerMode };
        iAwsComms->SendOrQueueAwsCommand( msg );
        }
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::CheckSettings
// ---------------------------------------------------------------------------
//
void CWlanBgScan::CheckSettings(
        MWlanBgScanProvider::TWlanBgScanSettings& aSettingsToUse,                   // [OUT]
        const MWlanBgScanProvider::TWlanBgScanSettings& aProposedSettings ) const   // [IN]
    {
    aSettingsToUse = aProposedSettings;

    if( aSettingsToUse.bgScanPeakStartTime > KWlanBgScanTwentyThreeFiftyNineOclock ||
        aSettingsToUse.bgScanPeakEndTime > KWlanBgScanTwentyThreeFiftyNineOclock )
        {
        DEBUG2( "CWlanBgScan::CheckSettings() - peak start or end invalid, using default values (start %u, end %u)",
                KWlanDefaultBgScanPeakPeriodStart,
                KWlanDefaultBgScanPeakPeriodEnd );
        aSettingsToUse.bgScanPeakStartTime = KWlanDefaultBgScanPeakPeriodStart;
        aSettingsToUse.bgScanPeakEndTime = KWlanDefaultBgScanPeakPeriodEnd;
        }
    if( aSettingsToUse.bgScanIntervalPeak > KWlanBgScanMaxInterval )
        {
        DEBUG1( "CWlanBgScan::CheckSettings() - peak interval invalid, using default value %u",
                KWlanDefaultBgScanIntervalPeakPeriod );
        aSettingsToUse.bgScanIntervalPeak = KWlanDefaultBgScanIntervalPeakPeriod;
        }
    if( aSettingsToUse.bgScanIntervalOffPeak > KWlanBgScanMaxInterval )
        {
        DEBUG1( "CWlanBgScan::CheckSettings() - off-peak interval invalid, using default value %u",
                KWlanDefaultBgScanIntervalOffPeakPeriod );
        aSettingsToUse.bgScanIntervalOffPeak = KWlanDefaultBgScanIntervalOffPeakPeriod;
        }    
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::GetInterval
// ---------------------------------------------------------------------------
//
TUint32 CWlanBgScan::GetInterval()
    {
    return iCurrentBgScanInterval;
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::SetInterval
// ---------------------------------------------------------------------------
//
void CWlanBgScan::SetInterval( TUint32 aInterval )
    {
    iCurrentBgScanInterval = aInterval;
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::IsIntervalChangeNeeded
// ---------------------------------------------------------------------------
//
TBool CWlanBgScan::IsIntervalChangeNeeded()
    {
    TBool ret( ETrue );
    
    // no need to change interval if both peak and off-peak intervals are the same
    if( iBgScanPeakStartTime == iBgScanPeakEndTime )
        {
        ret = EFalse;
        }
    
    DEBUG1( "CWlanBgScan::IsIntervalChangeNeeded() - returning %d", ret );
    
    return ret;    
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::ScheduleAutoIntervalChange
// ---------------------------------------------------------------------------
//
void CWlanBgScan::ScheduleAutoIntervalChange()
    {
    DEBUG( "CWlanBgScan::ScheduleAutoIntervalChange()" );
    
    iTimerServices.StopTimer( iIntervalChangeRequestId );

    if( IsIntervalChangeNeeded() )
        {
        TTime intervalChangeAt = AutoIntervalChangeAt();
        
        if( KErrNone != iTimerServices.StartTimer( iIntervalChangeRequestId, intervalChangeAt, *this ) )
            {
            DEBUG( "CWlanBgScan::ScheduleAutoIntervalChange() - error: requesting timeout failed, peak <-> off-peak interval will not be changed" );
            }
        }
    else
        {
        DEBUG( "CWlanBgScan::ScheduleAutoIntervalChange() - peak <-> off-peak interval change not needed" );
        }
    
    DEBUG( "CWlanBgScan::ScheduleAutoIntervalChange() - returning" );
    
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::AutoIntervalChangeAt
// ---------------------------------------------------------------------------
//
TTime CWlanBgScan::AutoIntervalChangeAt()
    {
    TTime currentTime;
    currentTime.HomeTime();
    TDateTime change_time( currentTime.DateTime() );
    
    switch( TimeRelationToRange( currentTime, iBgScanPeakStartTime, iBgScanPeakEndTime ) )
        {
        case ESmaller:
            {
            change_time.SetHour( iBgScanPeakStartTime / KGetHours );
            change_time.SetMinute( iBgScanPeakStartTime % KGetHours );
            change_time.SetSecond( KZeroSeconds );
            currentTime = change_time;
            break;
            }
        case EInsideRange:
            {
            change_time.SetHour( iBgScanPeakEndTime / KGetHours );
            change_time.SetMinute( iBgScanPeakEndTime % KGetHours );
            change_time.SetSecond( KZeroSeconds );
            currentTime = change_time;
            if( iBgScanPeakStartTime > iBgScanPeakEndTime )
                {
                DEBUG( "CWlanBgScan::AutoIntervalChangeAt() - peak end happens tomorrow" );
                currentTime += TTimeIntervalDays( KAddOneDay );
                }
            else
                {
                DEBUG( "CWlanBgScan::AutoIntervalChangeAt() - peak end happens today" );
                }
            break;
            }
        case EGreater:
            {
            change_time.SetHour( iBgScanPeakStartTime / KGetHours );
            change_time.SetMinute( iBgScanPeakStartTime % KGetHours );
            change_time.SetSecond( KZeroSeconds );
            currentTime = change_time;
            currentTime += TTimeIntervalDays( KAddOneDay );
            break;
            }
        }
    
#ifdef _DEBUG
    change_time = currentTime.DateTime();
    TBuf<KWlanBgScanMaxDateTimeStrLen> dbgString;
    TRAP_IGNORE( currentTime.FormatL( dbgString, KWlanBgScanDateTimeFormat ) );
    DEBUG1( "CWlanBgScan::AutoIntervalChangeAt() - interval change to occur: %S", &dbgString );
#endif
    
    return currentTime;
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::TimeRelationToRange
// ---------------------------------------------------------------------------
//
CWlanBgScan::TRelation CWlanBgScan::TimeRelationToRange( const TTime& aTime, TUint aRangeStart, TUint aRangeEnd ) const
    {
#ifdef _DEBUG
    TBuf<KWlanBgScanMaxDateTimeStrLen> dbgString;
    TRAP_IGNORE( aTime.FormatL( dbgString, KWlanBgScanDateTimeFormat ) );
    DEBUG1( "CWlanBgScan::TimeRelationToRange() - time:  %S", &dbgString );
#endif
    
    TTime time( aTime );
    
    TDateTime start_time( aTime.DateTime() );
    start_time.SetHour( aRangeStart / KGetHours );
    start_time.SetMinute( aRangeStart % KGetHours );
    start_time.SetSecond( KZeroSeconds );
    
    if( aRangeStart > aRangeEnd )
        {
        DEBUG( "CWlanBgScan::TimeRelationToRange() - end time of range must to be tomorrow" );
        if( time.DayNoInMonth() == ( time.DaysInMonth() - 1 ) )
            {
            DEBUG( "CWlanBgScan::TimeRelationToRange() - last day of the month, move to next month" );
            time += TTimeIntervalMonths( 1 );
            DEBUG( "CWlanBgScan::TimeRelationToRange() - move to first day of the month" );
            TDateTime new_time( time.DateTime() );
            new_time.SetDay( 0 );

            time = TTime( new_time );
            }
        else
            {
            DEBUG( "CWlanBgScan::TimeRelationToRange() - add one day to end time" );
            time += TTimeIntervalDays( KAddOneDay );
            }
        }
    
    TDateTime end_time( time.DateTime() );
    end_time.SetHour( aRangeEnd / KGetHours );
    end_time.SetMinute( aRangeEnd % KGetHours );
    end_time.SetSecond( KZeroSeconds );
        
#ifdef _DEBUG
    TBuf<KWlanBgScanMaxDateTimeStrLen> rngStart, rngEnd;
    TRAP_IGNORE( TTime( start_time ).FormatL( rngStart, KWlanBgScanDateTimeFormat ) );
    TRAP_IGNORE( TTime( end_time ).FormatL( rngEnd, KWlanBgScanDateTimeFormat ) );
    DEBUG2( "CWlanBgScan::TimeRelationToRange() - range: %S - %S", &rngStart, &rngEnd );
#endif
    
    CWlanBgScan::TRelation relation( ESmaller );
    if( aTime < TTime( start_time ) )
        {
        DEBUG( "CWlanBgScan::TimeRelationToRange() - returning: ESmaller" );
        relation = ESmaller;
        }
    else if( aTime >= TTime( start_time ) && aTime < TTime( end_time ) )
        {
        DEBUG( "CWlanBgScan::TimeRelationToRange() - returning: EInsideRange" );
        relation = EInsideRange;
        }
    else
        {
        DEBUG( "CWlanBgScan::TimeRelationToRange() - returning: EGreater" );
        relation = EGreater;
        }
    
    return relation;
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::CurrentAutoInterval
// ---------------------------------------------------------------------------
//
TUint32 CWlanBgScan::CurrentAutoInterval()
    {
    TUint32 interval( 0 );
    TTime currentTime;
    currentTime.HomeTime();

    if( TimeRelationToRange( currentTime, iBgScanPeakStartTime, iBgScanPeakEndTime ) == CWlanBgScan::EInsideRange )
        {
        interval = iBgScanIntervalPeak;
        }
    else
        {       
        interval = iBgScanIntervalOffPeak;
        }
    
    DEBUG1( "CWlanBgScan::CurrentAutoInterval() - current interval: %u", interval );

    return interval;
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::NextState
// ---------------------------------------------------------------------------
//
void CWlanBgScan::NextState( TUint32 aNewBgScanSetting )
    {
    DEBUG1( "CWlanBgScan::NextState() - aNewBgScanSetting %u", aNewBgScanSetting );
    
    switch ( iBgScanState )
        {
        case EBgScanOff:
            {
            InStateOff( iBgScanState, aNewBgScanSetting );
            break;
            }
        case EBgScanOn:
            {
            InStateOn( iBgScanState, aNewBgScanSetting );
            break;
            }
        case EBgScanAuto:
            {
            InStateAuto( iBgScanState, aNewBgScanSetting );
            break;
            }
        case EBgScanAutoAws:
            {
            InStateAutoAws( iBgScanState, aNewBgScanSetting );
            break;
            }
        default:
            {
            ASSERT( 0 );
            break;
            }
        }
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::InStateOff
// ---------------------------------------------------------------------------
//
void CWlanBgScan::InStateOff( TWlanBgScanState& aState, TUint32 aNewBgScanSetting )
    {
    switch( aNewBgScanSetting )
        {
        case KWlanBgScanIntervalNever:
            {
            DEBUG( "CWlanBgScan::InStateOff() - no change in the interval" );
            aState = EBgScanOff;
            break;
            }
        case KWlanBgScanIntervalAutomatic:
            {
            if ( IsAwsPresent() )
                {
                DEBUG( "CWlanBgScan::InStateOff() - state change Off to AutoAws" );

                aState = EBgScanAutoAws;
                DEBUG( "CWlanBgScan::InStateOff() - calling SendOrQueueAwsCommand()" );
                CWlanBgScanAwsComms::TAwsMessage msg = { CWlanBgScanAwsComms::EStart, 0 };
                iAwsComms->SendOrQueueAwsCommand( msg );
                DEBUG( "CWlanBgScan::InStateOff() - SendOrQueueAwsCommand() returned" );
                }
            else
                {
                DEBUG( "CWlanBgScan::InStateOff() - state change Off to Auto" );
                DEBUG( "CWlanBgScan::InStateOff() - * determine next interval change time and request callback" );
                ScheduleAutoIntervalChange();
                SetInterval( CurrentAutoInterval() );
                if( GetInterval() != KWlanBgScanIntervalNever )
                    {
                    DEBUG( "CWlanBgScan::InStateOff() - * cause immediate background scan" );
                    NotConnected();
                    }
                else
                    {
                    DEBUG( "CWlanBgScan::InStateOff() - Auto interval zero, background scanning is off" );
                    }
                aState = EBgScanAuto;
                }
            break;
            }
        default:
            {
            DEBUG1( "CWlanBgScan::InStateOff() - state change Off to On (interval: %u)", aNewBgScanSetting );
            SetInterval( aNewBgScanSetting );
            // cause immediate background scan
            NotConnected();
            aState = EBgScanOn;
            }
        }
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::InStateOn
// ---------------------------------------------------------------------------
//
void CWlanBgScan::InStateOn( TWlanBgScanState& aState, TUint32 aNewBgScanSetting )
    {
    switch( aNewBgScanSetting )
        {
        case KWlanBgScanIntervalNever:
            {
            DEBUG( "CWlanBgScan::InStateOn() - state change On to Off" );
            SetInterval( KWlanBgScanIntervalNever );
            iProvider.CancelScan();
            aState = EBgScanOff;
            break;
            }
        case KWlanBgScanIntervalAutomatic:
            {
            DEBUG( "CWlanBgScan::InStateOn() - state change On to Auto" );
            SetInterval( KWlanBgScanIntervalNever );
            iProvider.CancelScan();
            if ( IsAwsPresent() )
                {
                aState = EBgScanAutoAws;
                DEBUG( "CWlanBgScan::InStateOn() - calling SendOrQueueAwsCommand()" );
                CWlanBgScanAwsComms::TAwsMessage msg = { CWlanBgScanAwsComms::EStart, 0 };
                iAwsComms->SendOrQueueAwsCommand( msg );
                DEBUG( "CWlanBgScan::InStateOn() - SendOrQueueAwsCommand() returned" );
                }
            else
                {
                DEBUG( "CWlanBgScan::InStateOn() - * determine next interval change time and request callback" );
                ScheduleAutoIntervalChange();
                SetInterval( CurrentAutoInterval() );
                if( GetInterval() != KWlanBgScanIntervalNever )
                    {
                    DEBUG( "CWlanBgScan::InStateOn() - * cause immediate background scan" );
                    NotConnected();
                    }
                else
                    {
                    DEBUG( "CWlanBgScan::InStateOn() - Auto interval zero, background scanning is off" );
                    }
                aState = EBgScanAuto;
                }
            break;
            }
        default:
            {
            DEBUG( "CWlanBgScan::InStateOn() - state change On to On" );
            if ( GetInterval() == aNewBgScanSetting ) 
                {
                DEBUG( "CWlanBgScan::InStateOn() - no change in the interval" );
                }
            else if ( GetInterval() > aNewBgScanSetting )
                {
                DEBUG( "CWlanBgScan::InStateOn() - current interval greater than the new interval" );
                DEBUG( "CWlanBgScan::InStateOn() - * cancel scan and cause immediate background scan" );
                iProvider.CancelScan();
                SetInterval( aNewBgScanSetting );
                NotConnected();
                }
            else
                {
                DEBUG( "CWlanBgScan::InStateOn() - current interval smaller than the new interval" );
                DEBUG( "CWlanBgScan::InStateOn() - * cancel scan and issue new with interval as expiry" );
                iProvider.CancelScan();
                SetInterval( aNewBgScanSetting );
                ScanComplete();
                }
            aState = EBgScanOn;
            }
        }
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::InStateAuto
// ---------------------------------------------------------------------------
//
void CWlanBgScan::InStateAuto( TWlanBgScanState& aState, TUint32 aNewBgScanSetting )
    {
    switch( aNewBgScanSetting )
        {
        case KWlanBgScanIntervalNever:
            {
            DEBUG( "CWlanBgScan::InStateAuto() - state change Auto to Off" );
            SetInterval( KWlanBgScanIntervalNever );
            iTimerServices.StopTimer( iIntervalChangeRequestId );
            iIntervalChangeRequestId = 0;
            iProvider.CancelScan();
            aState = EBgScanOff;
            break;
            }
        case KWlanBgScanIntervalAutomatic:
            {
            DEBUG( "CWlanBgScan::InStateAuto() - state still Auto" );
            
            ScheduleAutoIntervalChange();

            TUint32 currentInterval = GetInterval();
            
            TUint32 autoInterval = CurrentAutoInterval();
                        
            if ( autoInterval == KWlanBgScanIntervalNever ) 
                {
                DEBUG( "CWlanBgScan::InStateAuto() - Auto interval zero, background scanning is off" );
                DEBUG( "CWlanBgScan::InStateAuto() - * cancel scan" );
                iProvider.CancelScan();
                SetInterval( autoInterval );
                }
            else if ( currentInterval == autoInterval ) 
                {
                DEBUG( "CWlanBgScan::InStateAuto() - no change in the Auto interval" );
                }
            else if ( currentInterval > autoInterval )
                {
                DEBUG( "CWlanBgScan::InStateAuto() - current Auto interval greater than the new Auto interval" );
                DEBUG( "CWlanBgScan::InStateAuto() - * cancel scan and issue new with immediate expiry" );
                iProvider.CancelScan();
                SetInterval( autoInterval );
                NotConnected();
                }
            else
                {
                DEBUG( "CWlanBgScan::InStateAuto() - current Auto interval smaller than the new Auto interval" );
                DEBUG( "CWlanBgScan::InStateAuto() - * cancel scan and issue new with interval expiry" );
                iProvider.CancelScan();
                SetInterval( autoInterval );
                ScanComplete();
                }
            
            aState = EBgScanAuto;
            break;
            }
        default:
            {
            DEBUG( "CWlanBgScan::InStateAuto() - state change Auto to On" );
            SetInterval( aNewBgScanSetting );
            iTimerServices.StopTimer( iIntervalChangeRequestId );
            iIntervalChangeRequestId = 0;
            // need to issue new scan request as it is possible that currently there is
            // no scan requested
            iProvider.CancelScan();
            NotConnected();
            aState = EBgScanOn;
            }
        }
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::InStateAutoAws
// ---------------------------------------------------------------------------
//
void CWlanBgScan::InStateAutoAws( TWlanBgScanState& aState, TUint32 aNewBgScanSetting )
    {
    switch( aNewBgScanSetting )
        {
        case KWlanBgScanIntervalNever:
            {
            DEBUG( "CWlanBgScan::InStateAutoAws() - state change Auto to Off" );
            SetInterval( KWlanBgScanIntervalNever );
            aState = EBgScanOff;
            DEBUG( "CWlanBgScan::InStateAutoAws() - calling SendOrQueueAwsCommand()" );
            CWlanBgScanAwsComms::TAwsMessage msg = { CWlanBgScanAwsComms::EStop, 0 };
            iAwsComms->SendOrQueueAwsCommand( msg );
            DEBUG( "CWlanBgScan::InStateAutoAws() - SendOrQueueAwsCommand() returned" );
            iProvider.CancelScan();
			break;
            }
        case KWlanBgScanIntervalAutomatic:
            {
            DEBUG( "CWlanBgScan::InStateAutoAws() - no change in the interval" );
            aState = EBgScanAutoAws;
            break;
            }
        default:
            {
            DEBUG( "CWlanBgScan::InStateAutoAws() - state change Auto to On" );
            SetInterval( aNewBgScanSetting );
            aState = EBgScanOn;
            // need to issue new scan request as it is possible that currently there is
            // no scan requested
            DEBUG( "CWlanBgScan::InStateAutoAws() - calling SendAwsCommand()" );
            CWlanBgScanAwsComms::TAwsMessage msg = { CWlanBgScanAwsComms::EStop, 0 };
            iAwsComms->SendOrQueueAwsCommand( msg );
            DEBUG( "CWlanBgScan::InStateAutoAws() - SendAwsCommand() returned" );
            iProvider.CancelScan();
            NotConnected();
            }
        }
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::Timeout
// ---------------------------------------------------------------------------
//
void CWlanBgScan::OnTimeout()
    {
    DEBUG( "CWlanBgScan::OnTimeout()" );

    // by design, OnTimeout should only happen
    // in Auto state
    ASSERT( iBgScanState == EBgScanAuto );
    
    NextState( KWlanBgScanIntervalAutomatic );
    
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::DoSetInterval
// ---------------------------------------------------------------------------
//
void CWlanBgScan::DoSetInterval( TUint32 aNewInterval )
    {
    DEBUG1( "CWlanBgScan::DoSetInterval( aNewInterval: %u )", aNewInterval );
    
    if( iBgScanState != EBgScanAutoAws )
        {
        DEBUG( "CWlanBgScan::DoSetInterval() - state not AutoAws, ignoring request" );
        return;
        }
    
    TUint32 currentInterval( GetInterval() );
        
    if ( ( currentInterval == 0 ) && ( aNewInterval != 0 ) )
        {
        DEBUG( "CWlanBgScan::DoSetInterval() - current interval is zero and new interval is non-zero" );
        DEBUG( "CWlanBgScan::DoSetInterval() - cancel scan and issue new with immediate expiry" );
        iProvider.CancelScan();
        SetInterval( aNewInterval );
        NotConnected();
        }
    else if ( currentInterval == aNewInterval ) 
        {
        DEBUG( "CWlanBgScan::DoSetInterval() - no change in the interval" );
        }
    else if ( currentInterval > aNewInterval )
        {
        // if current interval greater than new interval -> cancel scan and 
        // issue new with immediate expiry
        DEBUG( "CWlanBgScan::DoSetInterval() - current interval greater than the new interval" );
        DEBUG( "CWlanBgScan::DoSetInterval() - cancel scan and issue new with immediate expiry" );
        iProvider.CancelScan();
        SetInterval( aNewInterval );
        NotConnected();
        }
    else
        {
        DEBUG( "CWlanBgScan::DoSetInterval() - current interval smaller than the new interval" );
        DEBUG( "CWlanBgScan::DoSetInterval() - take new interval into use after currently pending scan is completed" );
        SetInterval( aNewInterval );
        }
    
    }

// ---------------------------------------------------------------------------
// CWlanBgScan::IsAwsPresent
// --------------------------------------------------------------------------
//
TBool CWlanBgScan::IsAwsPresent()
    {
    TBool ret( ETrue );
    
    if( iAwsComms == NULL || !iAwsComms->IsAwsPresent() )
        {
        ret = EFalse;
        }

    DEBUG1( "CWlanBgScan::IsAwsPresent() - returning %i", ret );
    
    return ret;
    }
