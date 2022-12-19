#include "fsmManager.h"

void fsmManager_init(fsm_t *fsm, uint32_t fsmStateInit)
{
    fsm->fsmStatePrev = fsmStateInit;
	fsm->fsmState = fsmStateInit;
	
	fsm->stateIn = 1;
	fsm->stateOut = 0;
}

void fsmManager_gotoState(fsm_t *fsm, uint32_t fsmStateNext)
{
    fsm->fsmStatePrev = fsm->fsmState;
	fsm->fsmState = fsmStateNext;
	
	fsm->stateIn = 0;
	fsm->stateOut = 1;
}

void fsmManager_gotoStatePrev(fsm_t *fsm)
{
    uint32_t fsmStatePrev = fsm->fsmStatePrev;

    fsm->fsmStatePrev = fsm->fsmState;
	fsm->fsmState = fsmStatePrev;
	
	fsm->stateIn = 0;
	fsm->stateOut = 1;
}

uint32_t fsmManager_getState(fsm_t *fsm)
{
    return fsm->fsmState;
}

uint32_t fsmManager_getStatePrev(fsm_t *fsm)
{
    return fsm->fsmStatePrev;
}

uint8_t fsmManager_isState(fsm_t *fsm, uint32_t fsmState)
{
    return fsm->fsmState == fsmState;
}

uint8_t fsmManager_isStateIn(fsm_t *fsm)
{
    return fsm->stateIn;
}

uint8_t fsmManager_isStateOut(fsm_t *fsm)
{
    return fsm->stateOut;
}

void fsmManager_stateIn(fsm_t *fsm)
{
    fsm->stateIn = 0;
    fsm->stateOut = 0;
}

void fsmManager_stateOut(fsm_t *fsm)
{
    fsm->stateIn = 1;
    fsm->stateOut = 0;
}
