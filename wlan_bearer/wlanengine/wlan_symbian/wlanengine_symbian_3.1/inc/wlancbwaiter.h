/*
* Copyright (c) 2007-2008 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef WLANCBWAITER_H
#define WLANCBWAITER_H

#include <e32base.h>

/**
 * This class implements an active object with callback functionality.
 * 
 * @since S60 v3.2
 */
NONSHARABLE_CLASS( CWlanCbWaiter ) : public CActive
    {

public:

    /**
     * Factory for creating an instance of CWlanTestWaiter.
     *
     * @param aCallback Callback to call when active object completes.
     * @return NULL if unable create an instance, a pointer to the instance otherwise.
     */
    static CWlanCbWaiter* NewL(
        const TCallBack& aCallback );

    /**
     * Destructor.
     */
    virtual ~CWlanCbWaiter();

    /**
     * Issue an asynchronous request.
     *
     * @since S60 v3.2
     */
    void IssueRequest();

    /**
     * Return the status of the request.
     *
     * @return Reference to status of the request.
     */
    TRequestStatus& RequestStatus();

// from base class CActive

    /**
     * From CActive.
     * Called by the active object framework when the request has been completed.
     */
    void RunL();

    /**
     * From CActive.
     * Called by the framework if RunL leaves.
     *
     * @param aError The error code RunL leaved with.
     * @return KErrNone if leave was handled, one of the system-wide error codes otherwise.
     */
    TInt RunError(
        TInt aError );

    /**
     * From CActive.
     * Called by the framework when Cancel() has been called.
     */
    void DoCancel();

private:

    /**
     * Constructor.
     *
     * @param aCallback Callback to call when active object completes.
     */
    CWlanCbWaiter(
        const TCallBack& aCallback );

    /**
     * By default Symbian 2nd phase constructor is private.
     */
    void ConstructL();

private: // data

    /** Function to call once request has been completed. */
    TCallBack iCallback;

    };

#endif // WLANCBWAITER_H
