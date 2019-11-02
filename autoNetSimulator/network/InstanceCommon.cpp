#include "InstanceCommon.h"

const uint16 rfDelays[2] = {
        (uint16) ((DWT_PRF_16M_RFDLY/ 2.0) * 1e-9 / DWT_TIME_UNITS),//PRF 16
        (uint16) ((DWT_PRF_64M_RFDLY/ 2.0) * 1e-9 / DWT_TIME_UNITS)
};

const uint8 dwnsSFDlen[3] =
{
    DW_NS_SFD_LEN_110K,
    DW_NS_SFD_LEN_850K,
    DW_NS_SFD_LEN_6M8
};

InstanceCommon::InstanceCommon()
{

}

void InstanceCommon::setPos(double x, double y, double z) {
    pos[0] = x;
    pos[1] = y;
    pos[2] = z;
}

void InstanceCommon::setRange(double r) {
    range = r;
}

const double* InstanceCommon::getPos(void) {
    return pos;
}

double InstanceCommon::getRange() {
    return range;
}


void InstanceCommon::set_position(double *pos, u8 flag) {
    if(pos != NULL) {
        instance_data[0].mypos.pos[0] = (pos[0] * 1000) / 1;
        instance_data[0].mypos.pos[1] = (pos[1] * 1000) / 1;
        instance_data[0].mypos.pos[2] = (pos[2] * 1000) / 1;
        instance_data[0].mypos.flag = flag;
        instance_data[0].mypos.shelftimes = 2;
    } else {
        //test code
        instance_data[0].mypos.pos[0] = 0x2018;
        instance_data[0].mypos.pos[1] = 0x3149;
        instance_data[0].mypos.pos[2] = 0x5689;
        instance_data[0].mypos.flag = 3;
        instance_data[0].mypos.shelftimes = 2;
    }
}

instance_data_t* InstanceCommon::get_local_structure_ptr(unsigned int x) {
    if(x >= 2) {
        return NULL;
    }

    return &instance_data[x];
}

uint64 InstanceCommon::convert_usec_to_devtimeu(double microsecu) {
    uint64 dt;
    long double dtime;

    dtime = (microsecu / (double) DWT_TIME_UNITS) / 1e6 ;

    dt =  (uint64) (dtime) ;

    return dt;
}

int InstanceCommon::get_role(void) {
    instance_data_t* inst = get_local_structure_ptr(0);

    return inst->mode;
}

int InstanceCommon::newrange(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    int x = inst->newRange;
    inst->newRange = TOF_REPORT_NUL;
    return x;
}

int InstanceCommon::newrangeancadd(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->newRangeAncAddress;
}

int InstanceCommon::newrangetagadd(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->newRangeTagAddress;
}

int InstanceCommon::newrangetim(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->newRangeTime;
}

void InstanceCommon::clearcounts(void)
{
    instance_data_t* inst = get_local_structure_ptr(0);
    int i= 0 ;

    //dwt_configeventcounters(1); //enable and clear - NOTE: the counters are not preserved when in DEEP SLEEP

    inst->frameSN = 0;
    inst->longTermRangeCount  = 0;

    for(i=0; i<MAX_ANCHOR_LIST_SIZE; i++) {
        inst->tofArray[i] = INVALID_TOF;
    }

    for(i=0; i<MAX_TAG_LIST_SIZE; i++) {
        inst->tof[i] = INVALID_TOF;
    }
}

int InstanceCommon::init(int inst_mode) {
    instance_data_t* inst = get_local_structure_ptr(0);
    int result;

    inst->mode = INST_MODE(inst_mode);        // assume listener,
    inst->twrMode = LISTENER;
    inst->testAppState = TA_INIT;
    inst->instToSleep = FALSE;

    // this initialises DW1000 and uses specified configurations from OTP/ROM
    //result = dwt_initialise(DWT_LOADUCODE) ;

    // this is platform dependent - only program if DW EVK/EVB
    //dwt_setleds(3) ; //configure the GPIOs which control the leds on EVBs

    if (DWT_SUCCESS != result) {
        //return (-1) ;   // device initialise has failed
    }

    clearcounts() ;
    inst->wait4ack = 0;
    inst->instanceTimerEn = 0;
    clearevents();

#if (DISCOVERY == 1)
    //dwt_geteui(inst->eui64);
    inst->panID = 0xdada ;
#else
    memset(inst->eui64, 0, ADDR_BYTE_SIZE_L);
    inst->panID = 0xdeca ;
#endif

    inst->tagSleepCorrection_ms = 0;
    //dwt_setdblrxbuffmode(0); 			//disable double RX buffer

    //dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | (DWT_INT_ARFE | DWT_INT_RFSL | DWT_INT_SFDT | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO | DWT_INT_RXPTO), 1);

    if(inst_mode == ANCHOR) {
        //dwt_setcallbacks(tx_conf_cb, rx_ok_cb_anch, rx_to_cb_anch, rx_err_cb_anch);
    } else {
        //dwt_setcallbacks(tx_conf_cb, rx_ok_cb_tag, rx_to_cb_tag, rx_err_cb_tag);
    }
    inst->monitor = 0;

    inst->remainingRespToRx = -1; 	//initialise
    inst->rxResps = 0;
    //dwt_setlnapamode(1, 1); 		//enable TX, RX state on GPIOs 6 and 5
    inst->delayedTRXTime32h = 0;

#if (READ_EVENT_COUNTERS == 1)
    //dwt_configeventcounters(1);
#endif
    return 0 ;
}

uint32 InstanceCommon::readdeviceid(void) {
    //return dwt_readdevid();
    return 0xDECA0130;
}

//OTP memory addresses for TREK calibration data
#define TXCFG_ADDRESS  (0x10)
#define ANTDLY_ADDRESS (0x1C)
#define TREK_ANTDLY_1  (0xD)
#define TREK_ANTDLY_2  (0xE)
#define TREK_ANTDLY_3  (0xF)
#define TREK_ANTDLY_4  (0x1D)

void InstanceCommon::instance_config(instanceConfig_t *config, sfConfig_t *sfConfig) {
    instance_data_t* inst = get_local_structure_ptr(0);
    uint32 power = 0;

    inst->configData.chan = config->channelNumber ;
    inst->configData.rxCode =  config->preambleCode ;
    inst->configData.txCode = config->preambleCode ;
    inst->configData.prf = config->pulseRepFreq ;
    inst->configData.dataRate = config->dataRate ;
    inst->configData.txPreambLength = config->preambleLen ;
    inst->configData.rxPAC = config->pacSize ;
    inst->configData.nsSFD = config->nsSFD ;
    inst->configData.phrMode = DWT_PHRMODE_STD ;
    inst->configData.sfdTO = config->sfdTO;

    if(inst->configData.dataRate == DWT_BR_6M8) {
        inst->smartPowerEn = 1;
    } else {
        inst->smartPowerEn = 0;
    }

    //Configure TX power and PG delay
    inst->configTX.power = power;

    inst->txAntennaDelay = rfDelays[config->pulseRepFreq - DWT_PRF_16M];
    inst->rxAntennaDelay = inst->txAntennaDelay;

    inst->BCNslotDuration_ms = sfConfig->BCNslotDuration_ms;
    inst->SVCslotDuration_ms = sfConfig->SVCslotDuration_ms;
    inst->TWRslotDuration_ms = sfConfig->TWRslotDuration_ms;
    inst->BCNslots_num = sfConfig->BCNslots_num;
    inst->SVCslots_num = sfConfig->SVCslots_num;
    inst->TWRslots_num = sfConfig->TWRslots_num;
    inst->sfPeriod_ms = sfConfig->sfPeriod_ms;
    inst->SVCstartTime_ms = inst->BCNslots_num * inst->BCNslotDuration_ms;
    inst->TWRstartTime_ms = inst->SVCslots_num * inst->SVCslotDuration_ms + inst->SVCstartTime_ms;

    //set the default response delays
    set_replydelay(sfConfig->grpPollTx2RespTxDly_us);
}

int InstanceCommon::get_rnum(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->rangeNum;
}

int InstanceCommon::get_rnuma(int idx) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->rangeNumA[idx];
}

int InstanceCommon::get_rnumanc(int idx) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->rangeNumAAnc[idx];
}

int InstanceCommon::get_lcount(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    int x = inst->longTermRangeCount;
    return (x);
}

void InstanceCommon::set_16bit_address(uint16 address) {
    instance_data_t* inst = get_local_structure_ptr(0);
    inst->instanceAddress16 = address ;       // copy configurations
}

uint16 InstanceCommon::address(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->instanceAddress16;
}

void InstanceCommon::config_frameheader_16bit(instance_data_t *inst) {
    //set frame type (0-2), SEC (3), Pending (4), ACK (5), PanIDcomp(6)
    inst->msg_f.frameCtrl[0] = 0x1 /*frame type 0x1 == data*/ | 0x40 /*PID comp*/;

    //source/dest addressing modes and frame version
    inst->msg_f.frameCtrl[1] = 0x8 /*dest extended address (16bits)*/ | 0x80 /*src extended address (16bits)*/;

    inst->msg_f.panID[0] = (inst->panID) & 0xff;
    inst->msg_f.panID[1] = inst->panID >> 8;

    inst->msg_f.seqNum = 0;
}

//延时发送实现
int InstanceCommon::send_delayed_frame(instance_data_t *inst, int delayedTx) {
    int result = 0;

    //dwt_writetxfctrl(inst->psduLength, 0, 1);
    if(delayedTx == DWT_START_TX_DELAYED) {
        //dwt_setdelayedtrxtime(inst->delayedTRXTime32h) ; //should be high 32-bits of delayed TX TS
    }

    //begin delayed TX of frame
//    if (dwt_starttx(delayedTx | inst->wait4ack))  // delayed start was too late
//    {
//        result = 1; //late/error
//        //inst->lateTX++;
//    }
//    else
    {
        inst->timeofTx = portGetTickCnt();
        inst->monitor = 1;
    }
    return result;
}

void InstanceCommon::tx_conf_cb(const event_data_t *cb_data) {
    instance_data_t* inst = get_local_structure_ptr(0);
    uint8 txTimeStamp[5] = {0, 0, 0, 0, 0};
    event_data_t dw_event;

    Q_UNUSED(cb_data)

    dw_event.uTimeStamp = portGetTickCnt_10us();

    if(inst->twrMode == RESPONDER_B) {
        inst->twrMode = LISTENER ;
    }
#if(DISCOVERY == 1)
    else if (inst->twrMode == GREETER) {
        //don't report TX event ...
    }
#endif
    else {
        //dwt_readtxtimestamp(txTimeStamp) ;
        seteventtime(&dw_event, txTimeStamp);

        dw_event.type =  0;
        dw_event.typePend =  0;
        dw_event.rxLength = 0;//inst->psduLength;

        putevent(&dw_event, DWT_SIG_TX_DONE);
    }

    inst->monitor = 0;
}

void InstanceCommon::seteventtime(event_data_t *dw_event, uint8* timeStamp) {
    dw_event->timeStamp32l =  (uint32)timeStamp[0] + ((uint32)timeStamp[1] << 8) + ((uint32)timeStamp[2] << 16) + ((uint32)timeStamp[3] << 24);
    dw_event->timeStamp = timeStamp[4];
    dw_event->timeStamp <<= 32;
    dw_event->timeStamp += dw_event->timeStamp32l;
    dw_event->timeStamp32h = ((uint32)timeStamp[4] << 24) + (dw_event->timeStamp32l >> 8);
}

int InstanceCommon::peekevent(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->dwevent[inst->dweventPeek].type; //return the type of event that is in front of the queue
}


void InstanceCommon::putevent(event_data_t *newevent, uint8 etype)
{
    instance_data_t* inst = get_local_structure_ptr(0);

    //copy event
    //inst->dwevent[inst->dweventIdxIn] = *newevent;
    inst->dwevent[inst->dweventIdxIn].typePend = newevent->typePend ;
    inst->dwevent[inst->dweventIdxIn].rxLength = newevent->rxLength ;
    inst->dwevent[inst->dweventIdxIn].timeStamp = newevent->timeStamp ;
    inst->dwevent[inst->dweventIdxIn].timeStamp32l = newevent->timeStamp32l ;
    inst->dwevent[inst->dweventIdxIn].timeStamp32h = newevent->timeStamp32h ;
    inst->dwevent[inst->dweventIdxIn].uTimeStamp = newevent->uTimeStamp ;
    if(newevent->rxLength > 0) {
        memcpy(&inst->dwevent[inst->dweventIdxIn].msgu, &newevent->msgu, newevent->rxLength);
    }

    //set type - this makes it a new event (making sure the event data is copied before event is set as new)
    //to make sure that the get event function does not get an incomplete event
    inst->dwevent[inst->dweventIdxIn].type = etype;
    inst->dweventIdxIn++;

    if(MAX_EVENT_NUMBER == inst->dweventIdxIn)
    {
        inst->dweventIdxIn = 0;
    }
}

event_data_t* InstanceCommon::getevent(int x) {
    instance_data_t* inst = get_local_structure_ptr(0);
    int indexOut = inst->dweventIdxOut;

    Q_UNUSED(x)

    if(inst->dwevent[indexOut].type == 0) //exit with "no event"
    {
        dw_event_g.type = 0;
        return &dw_event_g;
    }

    //copy the event
    dw_event_g.typePend = inst->dwevent[indexOut].typePend ;
    dw_event_g.rxLength = inst->dwevent[indexOut].rxLength ;
    dw_event_g.timeStamp = inst->dwevent[indexOut].timeStamp ;
    dw_event_g.timeStamp32l = inst->dwevent[indexOut].timeStamp32l ;
    dw_event_g.timeStamp32h = inst->dwevent[indexOut].timeStamp32h ;
    dw_event_g.uTimeStamp = inst->dwevent[indexOut].uTimeStamp ;

    //memcpy(&dw_event_g.msgu, &inst->dwevent[indexOut].msgu, sizeof(inst->dwevent[indexOut].msgu));
    if(inst->dwevent[indexOut].rxLength > 0) {
        memcpy(&dw_event_g.msgu, &inst->dwevent[indexOut].msgu, inst->dwevent[indexOut].rxLength);
    }

    dw_event_g.type = inst->dwevent[indexOut].type ;
    inst->dwevent[indexOut].type = 0; //clear the event

    inst->dweventIdxOut++;
    if(MAX_EVENT_NUMBER == inst->dweventIdxOut) //wrap the counter
    {
        inst->dweventIdxOut = 0;
    }
    inst->dweventPeek = inst->dweventIdxOut; //set the new peek value

    return &dw_event_g;
}

void InstanceCommon::clearevents(void) {
    int i = 0;
    instance_data_t* inst = get_local_structure_ptr(0);

    for(i=0; i<MAX_EVENT_NUMBER; i++) {
        memset(&inst->dwevent[i], 0, sizeof(event_data_t));
    }

    inst->dweventIdxIn = 0;
    inst->dweventIdxOut = 0;
    inst->dweventPeek = 0;
}

void InstanceCommon::config_txpower(uint32 txpower) {
    instance_data_t* inst = get_local_structure_ptr(0);
    inst->txPower = txpower ;

    inst->txPowerChanged = 1;
}

void InstanceCommon::set_txpower(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    if(inst->txPowerChanged == 1) {
        //Configure TX power
        //dwt_write32bitreg(0x1E, inst->txPower);

        inst->txPowerChanged = 0;
    }
}

void InstanceCommon::config_antennadelays(uint16 tx, uint16 rx) {
    instance_data_t* inst = get_local_structure_ptr(0);
    inst->txAntennaDelay = tx ;
    inst->rxAntennaDelay = rx ;

    inst->antennaDelayChanged = 1;
}

void InstanceCommon::set_antennadelays(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    if(inst->antennaDelayChanged == 1) {
        //dwt_setrxantennadelay(inst->rxAntennaDelay);
        //dwt_settxantennadelay(inst->txAntennaDelay);

        inst->antennaDelayChanged = 0;
    }
}

uint16 InstanceCommon::get_txantdly(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->txAntennaDelay;
}

uint16 InstanceCommon::get_rxantdly(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    return inst->rxAntennaDelay;
}

uint8 InstanceCommon::validranges(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    uint8 x = inst->rxResponseMaskReport;
    inst->rxResponseMaskReport = 0; //reset mask as we have printed out the ToFs
    return x;
}

float InstanceCommon::calc_length_data(float msgdatalen) {
    instance_data_t* inst = get_local_structure_ptr(0);

    int x = 0;

    x = (int) ceil(msgdatalen*8/330.0f);

    msgdatalen = msgdatalen*8 + x*48;

    //assume PHR length is 172308ns for 110k and 21539ns for 850k/6.81M
    if(inst->configData.dataRate == DWT_BR_110K) {
        msgdatalen *= 8205.13f;
        msgdatalen += 172308; // PHR length in nanoseconds

    } else if(inst->configData.dataRate == DWT_BR_850K) {
        msgdatalen *= 1025.64f;
        msgdatalen += 21539; // PHR length in nanoseconds
    } else {
        msgdatalen *= 128.21f;
        msgdatalen += 21539; // PHR length in nanoseconds
    }

    return msgdatalen ;
}

void InstanceCommon::set_replydelay(int delayus) {
    instance_data_t *inst = &instance_data[0];

    int margin = 3000; //2000 symbols
    int grppollframe = 0;
    int pollframe = 0;
    int finalframe = 0;
    int gpollframe_sy = 0;
    int pollframe_sy = 0;
    int respframe_sy = 0;
    int finalframe_sy = 0;

    //configure the rx delay receive delay time, it is dependent on the message length
    float preamblelen = 0;
    float msgdatalen_gpollT = 0.0;
    float msgdatalen_pollA = 0.0;
    float msgdatalen_respT = 0.0;
    float msgdatalen_finalA = 0.0;
    int sfdlen = 0;

    //Set the RX timeouts based on the longest expected message
//	msgdatalen_bcn = calc_length_data(BCN_MSG_LEN + ANC_POS_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
//	msgdatalen_svc = calc_length_data(SVC_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
    msgdatalen_gpollT = calc_length_data(TWR_GRP_POLL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
    msgdatalen_pollA = calc_length_data(TWR_POLL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
    msgdatalen_respT = calc_length_data(TWR_RESP_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
    msgdatalen_finalA = calc_length_data(TWR_FINAL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);

    //SFD length is 64 for 110k (always)
    //SFD length is 8 for 6.81M, and 16 for 850k, but can vary between 8 and 16 bytes
    sfdlen = dwnsSFDlen[inst->configData.dataRate];

    switch (inst->configData.txPreambLength) {
        case DWT_PLEN_4096 : preamblelen = 4096.0f; break;
        case DWT_PLEN_2048 : preamblelen = 2048.0f; break;
        case DWT_PLEN_1536 : preamblelen = 1536.0f; break;
        case DWT_PLEN_1024 : preamblelen = 1024.0f; break;
        case DWT_PLEN_512  : preamblelen = 512.0f; 	break;
        case DWT_PLEN_256  : preamblelen = 256.0f; 	break;
        case DWT_PLEN_128  : preamblelen = 128.0f; 	break;
        case DWT_PLEN_64   : preamblelen = 64.0f; 	break;
    }

    //preamble  = plen * (994 or 1018) depending on 16 or 64 PRF
    if(inst->configData.prf == DWT_PRF_16M) {
        preamblelen = (sfdlen + preamblelen) * 0.99359f;
    } else {
        preamblelen = (sfdlen + preamblelen) * 1.01763f;
    }

    gpollframe_sy = (DW_RX_ON_DELAY + (int)((preamblelen + ((msgdatalen_gpollT + margin)/1000.0)) / 1.0256));
    pollframe_sy = (DW_RX_ON_DELAY + (int)((preamblelen + ((msgdatalen_pollA + margin)/1000.0)) / 1.0256));
    respframe_sy = (DW_RX_ON_DELAY + (int)((preamblelen + ((msgdatalen_respT + margin)/1000.0)) / 1.0256));
    finalframe_sy = (DW_RX_ON_DELAY + (int)((preamblelen + ((msgdatalen_finalA + margin)/1000.0)) / 1.0256));

    inst->grpPollTx2RespTxDly_us = convert_usec_to_devtimeu (delayus);

    grppollframe = (int)(preamblelen + (msgdatalen_gpollT / 1000.0));
    pollframe = (int)(preamblelen + (msgdatalen_pollA / 1000.0));
    finalframe = (int)(preamblelen + (msgdatalen_finalA / 1000.0));
    if(inst->configData.dataRate == DWT_BR_110K) {
        //preamble duration + 16 us for RX on
        inst->preambleDuration32h = (uint32) (((uint64) convert_usec_to_devtimeu (preamblelen)) >> 8) + DW_RX_ON_DELAY;

        inst->ancRespRxDelay_sy = RX_RESPONSE_TURNAROUND - DW_RX_ON_DELAY;
        inst->tagPollRxDelay_sy = pollframe_sy - 3 * RX_RESPONSE_TURNAROUND;//(grppollframe / 2) + RX_RESPONSE_TURNAROUND + pollframe_sy - gpollframe_sy;
        inst->tagFinalRxDelay_sy = RX_RESPONSE_TURNAROUND + finalframe_sy - respframe_sy;

        inst->fixedPollDelayAnc32h = ((uint64)convert_usec_to_devtimeu(pollframe + RX_RESPONSE_TURNAROUND)) >> 8;
        inst->fixedFinalDelayAnc32h = ((uint64)convert_usec_to_devtimeu(finalframe + RX_RESPONSE_TURNAROUND)) >> 8;

        inst->fixedGuardDelay32h = ((uint64)convert_usec_to_devtimeu(pollframe + 2 * RX_RESPONSE_TURNAROUND)) >> 8;
        inst->fixedOffsetDelay32h = ((uint64)convert_usec_to_devtimeu((grppollframe / 2))) >> 8;
    } else {
        //preamble duration + 16 us for RX on
        inst->preambleDuration32h = (uint32) (((uint64)convert_usec_to_devtimeu (preamblelen)) >> 8) + DW_RX_ON_DELAY;

        inst->ancRespRxDelay_sy = RX_RESPONSE_TURNAROUND_6M8 - DW_RX_ON_DELAY;
        inst->tagPollRxDelay_sy = (grppollframe / 2 + RX_RESPONSE_TURNAROUND_6M8 / 2) + RX_RESPONSE_TURNAROUND_6M8 + pollframe_sy - gpollframe_sy;
        inst->tagFinalRxDelay_sy = RX_RESPONSE_TURNAROUND_6M8 + finalframe_sy - respframe_sy;

        inst->fixedPollDelayAnc32h = ((uint64)convert_usec_to_devtimeu(pollframe + RX_RESPONSE_TURNAROUND_6M8)) >> 8;
        inst->fixedFinalDelayAnc32h = ((uint64)convert_usec_to_devtimeu(finalframe + RX_RESPONSE_TURNAROUND_6M8)) >> 8;

        inst->fixedPollDelayAnc_us = pollframe + RX_RESPONSE_TURNAROUND_6M8;

        inst->fixedGuardDelay32h = ((uint64)convert_usec_to_devtimeu(pollframe + 2 * RX_RESPONSE_TURNAROUND_6M8)) >> 8;
        inst->fixedOffsetDelay32h = ((uint64)convert_usec_to_devtimeu((grppollframe / 2 + RX_RESPONSE_TURNAROUND_6M8 / 2))) >> 8;
    }

    inst->fwto4PollFrame_sy = pollframe_sy;
    inst->fwto4RespFrame_sy = respframe_sy;
    inst->fwto4FinalFrame_sy = finalframe_sy;

    qDebug() << "gpollframe:" << gpollframe_sy
             << ", pollframe:" << pollframe_sy
             << ", respframe:" << respframe_sy
             << ", finalframe:" << finalframe_sy
             << ", tagPollRxDelay:" << (uint32)inst->tagPollRxDelay_sy
             << ", tagFinalRxDelay:" << (uint32)inst->tagFinalRxDelay_sy;

    //printf("fixedPoll:%u, fixedFinal:%u, fixedGuard:%u\n", (uint32)inst->fixedPollDelayAnc32h, (uint32)inst->fixedFinalDelayAnc32h, (uint32)inst->fixedGuardDelay32h);

    inst->BCNfixTime32h = (convert_usec_to_devtimeu (inst->BCNslotDuration_ms * 1000) >> 8);
    inst->SVCfixTime32h = (convert_usec_to_devtimeu (inst->SVCslotDuration_ms * 1000) >> 8);
    //inst->TWRfixTime32h = (instance_convert_usec_to_devtimeu (inst->TWRslotDuration_ms * 1000) >> 8);
}









