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
* Description:  This class implements an active object with callback functionality.
*
*/


#include "wlancbwaiter.h"
#include "am_debug.h"

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
CWlanCbWaiter::CWlanCbWaiter(
    const TCallBack& aCallback ) :
    CActive( CActive::EPriorityStandard ),
    iCallback( aCallback )
    {
    DEBUG( "CWlanCbWaiter::CWlanCbWaiter()" );
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void CWlanCbWaiter::ConstructL()
    {
    DEBUG( "CWlanCbWaiter::ConstructL()" );
    CActiveScheduler::Add( this );
    DEBUG( "CWlanCbWaiter::ConstructL() - done" );
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
CWlanCbWaiter* CWlanCbWaiter::NewL(
    const TCallBack& aCallback )
    {
    DEBUG( "CWlanCbWaiter::NewL()" );
    CWlanCbWaiter* self = new (ELeave) CWlanCbWaiter( aCallback );

    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );

    return self;
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
CWlanCbWaiter::~CWlanCbWaiter()
    {
    DEBUG( "CWlanCbWaiter::~CWlanCbWaiter()" );
    Cancel();
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void CWlanCbWaiter::IssueRequest()
    {
    DEBUG( "CWlanCbWaiter::IssueRequest()" );
    SetActive();
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
TRequestStatus& CWlanCbWaiter::RequestStatus()
    {
    DEBUG1( "CWlanCbWaiter::RequestStatus() - iStatus = %d", iStatus.Int() );
    return iStatus;
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void CWlanCbWaiter::RunL()
    {
    DEBUG1( "CWlanCbWaiter::RunL() - iStatus = %d", iStatus.Int() );

    iCallback.CallBack();
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
TInt CWlanCbWaiter::RunError(
    TInt /* aError */ )
    {
    DEBUG( "CWlanCbWaiter::RunError()" );

    return KErrNone;
    }

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//
void CWlanCbWaiter::DoCancel()
    {
    DEBUG( "CWlanCbWaiter::DoCancel()" );
    }
