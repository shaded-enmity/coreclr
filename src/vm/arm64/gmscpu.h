//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//

/**************************************************************/
/*                       gmscpu.h                             */
/**************************************************************/
/* HelperFrame is defines 'GET_STATE(machState)' macro, which 
   figures out what the state of the machine will be when the 
   current method returns.  It then stores the state in the
   JIT_machState structure.  */

/**************************************************************/

#ifndef __gmscpu_h__
#define __gmscpu_h__

#define __gmscpu_h__

// X19 - X28
#define NUM_NONVOLATILE_CONTEXT_POINTERS 10

struct MachState {
    ULONG64        captureX19_X28[NUM_NONVOLATILE_CONTEXT_POINTERS]; // preserved registers
    PTR_ULONG64    ptrX19_X28[NUM_NONVOLATILE_CONTEXT_POINTERS]; // pointers to preserved registers
    TADDR          _pc;
    TADDR          _sp;        
    TADDR          _fp;
    BOOL           _isValid;
    
    BOOL   isValid()    { LIMITED_METHOD_DAC_CONTRACT; return _isValid; }
    TADDR  GetRetAddr() { LIMITED_METHOD_DAC_CONTRACT; return _pc; }
};

struct LazyMachState : public MachState{

    TADDR          captureSp;         // Stack pointer at the time of capture
    TADDR          captureIp;         // Instruction pointer at the time of capture
    TADDR          captureFp;         // Frame pointer at the time of the captues

    void setLazyStateFromUnwind(MachState* copy);
    static void unwindLazyState(LazyMachState* baseState,
                                MachState* lazyState,
                                int funCallDepth = 1,
                                HostCallPreference hostCallPreference = AllowHostCalls);
};

inline void LazyMachState::setLazyStateFromUnwind(MachState* copy)
{
#if defined(DACCESS_COMPILE)
    // This function cannot be called in DAC because DAC cannot update target memory.
    DacError(E_FAIL);
    return;
    
#else  // !DACCESS_COMPILE

    _sp = copy->_sp;
    _pc = copy->_pc;
    _fp = copy->_fp;

    // Now copy the preserved register pointers. Note that some of the pointers could be
    // pointing to copy->captureX19_X28[]. If that is case then while copying to destination
    // ensure that they point to corresponding element in captureX19_X28[] of destination.
    ULONG64* srcLowerBound = &copy->captureX19_X28[0];
    ULONG64* srcUpperBound = (ULONG64*)((BYTE*)copy + offsetof(MachState, ptrX19_X28));


    for (int i = 0; i<NUM_NONVOLATILE_CONTEXT_POINTERS; i++)
    {
        if (copy->ptrX19_X28[i] >= srcLowerBound && copy->ptrX19_X28[i] < srcUpperBound)
        {
            ptrX19_X28[i] = (PTR_ULONG64)((BYTE*)copy->ptrX19_X28[i] - (BYTE*)srcLowerBound + (BYTE*)captureX19_X28);
        }
        else
        {
            ptrX19_X28[i] = copy->ptrX19_X28[i];
        }
    }

    // this has to be last because we depend on write ordering to 
    // synchronize the race implicit in updating this struct
    VolatileStore(&_isValid, TRUE);
#endif // DACCESS_COMPILE
}

// Do the initial capture of the machine state.  This is meant to be 
// as light weight as possible, as we may never need the state that 
// we capture.  
EXTERN_C void LazyMachStateCaptureState(struct LazyMachState *pState);

#define CAPTURE_STATE(machState, ret)                       \
    LazyMachStateCaptureState(machState)


#endif
