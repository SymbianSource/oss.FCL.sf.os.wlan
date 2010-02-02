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
* Description:   Declaration of the MgmtFrameMemMngr class.
*
*/

/*
* %version: 14 %
*/

#ifndef MGMTFRAMEMMNGR_H
#define MGMTFRAMEMMNGR_H

#include "EthernetFrameMemMngr.h"

class WlanChunk;


/**
*  Memory manager for management client frame Rx memory
*
*  @since S60 v3.1
*/
class MgmtFrameMemMngr : public DEthernetFrameMemMngr
    {

public:

    /** Ctor */
    MgmtFrameMemMngr( 
        DWlanLogicalChannel& aParent,
        WlanChunk*& aRxFrameMemoryPool,
        TBool aUseCachedMemory,
        TInt aRxFrameBufAllocationUnit ) : 
        DEthernetFrameMemMngr( aParent, aRxFrameMemoryPool ),
        iUseCachedMemory( aUseCachedMemory ),
        iRxFrameBufAllocationUnit ( aRxFrameBufAllocationUnit ),
        iChunkSize( 
            Kern::RoundToPageSize( 
                4096 * 15 + KProtocolStackSideTxDataChunkSize ) ) // bytes
        {};

    /** Dtor */
    virtual ~MgmtFrameMemMngr() {};

protected:

    /**
    * From DEthernetFrameMemMngr
    * Allocates a shared memory chunk for frame transfer between user
    * and kernel address spaces
    *
    * @since S60 3.1
    * @param aSharedMemoryChunk The shared memory chunk
    * @return system wide error code, KErrNone upon success
    */
    virtual TInt DoAllocate( DChunk*& aSharedMemoryChunk );

    /**
    * Opens a handle for user mode client to the shared memory chunk
    * allocated for frame transfer between user and kernel address spaces
    *
    * @since S60 3.1
    * @param aThread The user mode client thread
    * @param aSharedChunkInfo After successful return contains the handle to the
    *        chunk
    * @param aSharedMemoryChunk The shared memory chunk
    * @return system wide error code, KErrNone upon success
    */
    virtual TInt DoOpenHandle(
        DThread& aThread,
        TSharedChunkInfo& aSharedChunkInfo,
        DChunk* aSharedMemoryChunk );

    /**
    * From DEthernetFrameMemMngr
    * Gets a free rx buffer
    *
    * @since S60 3.1
    * @param aLengthinBytes Requested buffer length
    * @return buffer for Rx data upon success
    *         NULL otherwise
    */
    virtual TUint8* DoGetNextFreeRxBuffer( TUint aLengthinBytes );

    /**
    * From DEthernetFrameMemMngr
    * Gets called when rx frame read cycle has ended.
    *
    * @since S60 3.1
    * @param aBufferStart first element of the array that holds pointers to
    *        Rx frame meta headers
    * @param aNumOfBuffers number of meta header pointers in the array
    * @return ETrue if a pending user mode frame read request exists 
    *         and callee should complete it, 
    *         EFalse otherwise
    */
    virtual TBool DoEthernetFrameRxComplete( 
        const TDataBuffer*& aBufferStart, 
        TUint32 aNumOfBuffers );

    /**
    * From DEthernetFrameMemMngr
    * Gets start address of Rx buffers (their offset addresses)
    * that are waiting for completion to user mode
    *
    * @since S60 3.1
    * @return see above statement
    */
    virtual TUint32* DoGetTobeCompletedBuffersStart();

    /**
    * From DEthernetFrameMemMngr
    * Gets start address of Rx buffers (their offset addresses)
    * that have been completed to user mode
    *
    * @since S60 3.1
    * @return see above statement
    */
    virtual TUint32* DoGetCompletedBuffersStart();

    /**
    * From DEthernetFrameMemMngr
    * Gets called when user mode client issues a frame receive request 
    * and Rx buffers have been completed to it. The completed Rx frame 
    * buffers are freed.
    *
    * @since S60 3.1
    */
    virtual void DoFreeRxBuffers();

    /**
    * From DEthernetFrameMemMngr
    * Frees the specified Rx frame buffer
    *
    * @since S60 3.1
    * @param aBufferToFree The buffer to free
    */
    virtual void DoMarkRxBufFree( TUint8* aBufferToFree );

private:

    /**
    * Returns the number of extra bytes required to align Rx buffer start
    * address, to be returned to WHA layer, to allocation unit boundary
    * @return See above
    */
    TInt RxBufAlignmentPadding() const;
    
    /**
    * From DEthernetFrameMemMngr
    * Memory finalization method.
    * Deallocates the shared memory chunk
    *
    * @since S60 3.1
    */
    virtual void OnReleaseMemory();

    // Prohibit copy constructor.
    MgmtFrameMemMngr( const MgmtFrameMemMngr& );
    // Prohibit assigment operator.
    MgmtFrameMemMngr& operator= ( const MgmtFrameMemMngr & ); 
    
private:    // Data

    /** 
    * kernel address of the shared memory chunk
    */
    TLinAddr iChunkKernelAddr;

    /** 
    * array of TDataBuffer offset addresses, denoting Rx buffers,
    * which are waiting here in kernel mode to be completed 
    * to user mode, when the next frame receive request arrives
    */
    TUint32  iTobeCompletedBuffers[KMaxToBeCompletedRxBufs];

    /** 
    * array of TDataBuffer offset addresses, denoting Rx buffers, that are
    * currently under processing in user mode
    */
    TUint32  iCompletedBuffers[KMaxCompletedRxBufs];

    /** 
    * ETrue if cached frame transfer memory shall be used,
    * EFalse otherwise
    */
    TBool iUseCachedMemory;

    /**
    * size of the Rx frame buffer allocation unit in bytes
    */
    TInt iRxFrameBufAllocationUnit;

    /** size of the shared memory chunk */
    TInt iChunkSize;
    };

#endif // MGMTFRAMEMMNGR_H
