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
* Description:   Declaration of the DataFrameMemMngr class.
*
*/

/*
* %version: 11 %
*/

#ifndef DATAFRAMEMMNGR_H
#define DATAFRAMEMMNGR_H

#include "EthernetFrameMemMngr.h"

class WlanChunk;

/**
*  Memory manager for protocol stack side client frame Rx memory
*
*  @since S60 v3.1
*/
class DataFrameMemMngr : public DEthernetFrameMemMngr
    {
public: 

    /** Ctor */
    DataFrameMemMngr( 
        DWlanLogicalChannel& aParent,
        WlanChunk*& aRxFrameMemoryPool,
        TInt aTxFrameBufAllocationUnit ) : 
        DEthernetFrameMemMngr( aParent, aRxFrameMemoryPool ),
        iFrameXferBlockProtoStack( NULL ),
        iTxDataChunk( NULL ),
        iTxFrameMemoryPool( NULL ),
        iTxFrameBufAllocationUnit ( aTxFrameBufAllocationUnit )
        {};

    /** Dtor */
    virtual ~DataFrameMemMngr() 
        {
        iFrameXferBlockProtoStack = NULL;
        iTxDataChunk = NULL;
        iTxFrameMemoryPool = NULL;
        };

protected: 

    /**
    * From DEthernetFrameMemMngr
    * Opens a handle to the allocated shared memory chunk
    *
    * @since S60 5.0
    * @param aThread
    * @param aSharedChunkInfo
    * @param aSharedMemoryChunk The shared memory chunk
    * @return system wide error code, KErrNone upon success
    */
    virtual TInt DoOpenHandle(
        DThread& aThread,
        TSharedChunkInfo& aSharedChunkInfo,
        DChunk* aSharedMemoryChunk );

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
    * @return see above
    */
    virtual TUint32* DoGetTobeCompletedBuffersStart();

    /**
    * From DEthernetFrameMemMngr
    * Gets start address of Rx buffers (their offset addresses)
    * that have been completed to user mode
    *
    * @since S60 3.1
    * @return see above
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
     * Allocates a Tx packet from the shared memory.
     * 
     * @param aLength Length of the requested Tx buffer in bytes
     * @return Pointer to the meta header attached to the allocated packet, on
     *         success.
     *         NULL, in case of failure.
     */
    virtual TDataBuffer* AllocTxBuffer( TUint aLength );

    /**
     * From DEthernetFrameMemMngr
     * Deallocates a Tx packet.
     * 
     * All Tx packets allocated with AllocTxBuffer() must be deallocated using
     * this method.
     * 
     * @param aPacket Meta header of the packet to the deallocated
     */ 
    virtual void FreeTxPacket( TDataBuffer*& aPacket );    
    
private:

    /**
    * From DEthernetFrameMemMngr
    * Memory finalization method.
    */
    virtual void OnReleaseMemory();
    
    // Prohibit copy constructor.
    DataFrameMemMngr( const DataFrameMemMngr& );
    // Prohibit assigment operator.
    DataFrameMemMngr& operator= ( const DataFrameMemMngr& );  
    
private:    // Data

    /** 
    * array of TDataBuffer offset addresses, denoting Rx buffers,
    * which are waiting here in kernel mode to be completed 
    * to user mode, when the next frame receive request arrives
    */
    TUint32 iTobeCompletedBuffers[KMaxToBeCompletedRxBufs];

    /** 
    * array of TDataBuffer offset addresses, denoting Rx buffers, that are
    * currently under processing in user mode
    */
    TUint32 iCompletedBuffers[KMaxCompletedRxBufs];
    
    RFrameXferBlockProtocolStack* iFrameXferBlockProtoStack;
    
    /** 
    * pointer to protocol stack side Tx area start in the kernel address 
    * space 
    */
    TUint8* iTxDataChunk;

    /** 
    * Tx frame memory pool manager
    * Own.
    */
    WlanChunk* iTxFrameMemoryPool;
    
    /**
    * size of the Tx frame buffer allocation unit in bytes
    */
    TInt iTxFrameBufAllocationUnit;
    };

#endif // DATAFRAMEMMNGR_H
