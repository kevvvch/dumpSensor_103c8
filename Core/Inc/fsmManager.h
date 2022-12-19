#include <stdint.h>

typedef struct _fsm_t {
    uint32_t fsmState;
    uint32_t fsmStatePrev;
    uint8_t stateIn;
    uint8_t stateOut;
} fsm_t;

void fsmManager_init(fsm_t *fsm, uint32_t fsmStateInit);
void fsmManager_gotoState(fsm_t *fsm, uint32_t fsmStateNext);
void fsmManager_gotoStatePrev(fsm_t *fsm);
uint32_t fsmManager_getState(fsm_t *fsm);
uint32_t fsmManager_getStatePrev(fsm_t *fsm);
uint8_t fsmManager_isStateIn(fsm_t *fsm);
uint8_t fsmManager_isState(fsm_t *fsm, uint32_t fsmState);
uint8_t fsmManager_isStateOut(fsm_t *fsm);
void fsmManager_stateIn(fsm_t *fsm);
void fsmManager_stateOut(fsm_t *fsm);
