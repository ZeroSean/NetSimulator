#include "InstanceTag.h"

InstanceTag::InstanceTag(Coordinator *co)
{
    coor = co;
}


uint8 InstanceTag::findFirstZeroBit(uint16 bitMap, uint8 num) {
    uint8 i = 0;
    uint16 mask = 0x01;
    for(i = 0; i < num; ++i) {
        if((bitMap & mask) == 0x00) {
            return i;
        }
        mask <<= 1;
    }
    return i;
}

void InstanceTag::fill_msg_addr(instance_data_t* inst, uint16 dstaddr, uint8 msgid) {
    inst->msg_f.sourceAddr[0] = inst->instanceAddress16 & 0xff; //copy the address
    inst->msg_f.sourceAddr[1] = (inst->instanceAddress16 >> 8) & 0xff;
    inst->msg_f.destAddr[0] = dstaddr & 0xff;
    inst->msg_f.destAddr[1] = (dstaddr >> 8) & 0xff;
    inst->msg_f.seqNum = inst->frameSN++;
    inst->msg_f.messageData[MIDOF] = msgid;
}

void InstanceTag::enable_rx(uint32 dlyTime, int fwto_sy) {
    //instance_data_t* inst = get_local_structure_ptr(0);
    Q_UNUSED(dlyTime)
    Q_UNUSED(fwto_sy)
}

uint8 InstanceTag::setPollRxStamp(instance_data_t *inst, uint8 idx, uint8 error) {
    uint8 type_pend = DWT_SIG_RX_TWR_ERROR;

    idx = idx & (MAX_ANCHOR_LIST_SIZE - 1);
    if(error == 0) {
        inst->remainingRespToRx = MAX_ANCHOR_LIST_SIZE - 1 - idx;
    }

    if(inst->remainingRespToRx == 0 || idx == (MAX_ANCHOR_LIST_SIZE - 1)) {
        if(inst->twrmag.pollMask != 0) {
            inst->occupySlot = findFirstZeroBit(inst->slotMoniMap, inst->TWRslots_num);
            type_pend = send_response_msg(inst);
        } else {
            type_pend = DWT_SIG_RX_TWR_ERROR;
        }
    } else {
        uint32 txStamp32h = (inst->grpPollTxStamp >> 8) + inst->fixedOffsetDelay32h;
        txStamp32h += (MAX_ANCHOR_LIST_SIZE + 1 - inst->remainingRespToRx) * inst->fixedPollDelayAnc32h;
        enable_rx(txStamp32h, inst->fwto4PollFrame_sy);
        type_pend = DWT_SIG_RX_PENDING;
    }
    return type_pend;
}

uint8 InstanceTag::setFinalRxStamp(instance_data_t *inst, uint8 idx, uint8 error) {
    uint8 type_pend = DWT_SIG_DW_IDLE;

    idx = idx & (MAX_ANCHOR_LIST_SIZE - 1);
    if(error == 0) {
        //inst->remainingRespToRx = MAX_ANCHOR_LIST_SIZE - 1 - idx;
    }

    if(inst->remainingRespToRx == 0) {
        if(inst->twrmag.finalCfm != (inst->twrmag.pollMask & 0x0f)) {
            inst->occupySlot = 0xff;
            //error_printf("cfm:%x, finalMask:%x--pollMask:%x\n", inst->twrmag.finalCfm, inst->twrmag.finalMask, inst->twrmag.pollMask);
        }
        //debug_printf("cfm:%x--valid:%x\n", inst->twrmag.finalCfm, inst->twrmag.validNum);
        if(inst->occupySlot < inst->TWRslots_num) {
            type_pend = DWT_SIG_RX_TWR_OKAY;
            //info_printf("finished TWR:%ld\n", dwt_readsystimestamphi32());
        } else {
            type_pend = DWT_SIG_RX_TWR_ERROR;
        }
        inst->wait4type = 0;
    } else {
        uint32 txStamp32h = inst->responseTxStamp32h;
        txStamp32h += (MAX_ANCHOR_LIST_SIZE + 1 - inst->remainingRespToRx) * inst->fixedFinalDelayAnc32h;
        enable_rx(txStamp32h, inst->fwto4FinalFrame_sy);
        type_pend = DWT_SIG_RX_PENDING;
    }

    return type_pend;
}

void InstanceTag::change_state_to_listen(instance_data_t *inst) {
    inst->testAppState = TA_RXE_WAIT;
    inst->wait4type = UWBMAC_FRM_TYPE_BCN;
    inst->wait4ack = 0;
    inst->twrMode = BCN_LISTENER;
}

void InstanceTag::process_rx_event(instance_data_t *inst, uint8 eventTypePend) {
    if(eventTypePend == DWT_SIG_RX_PENDING) {

    } else if(eventTypePend == DWT_SIG_TX_PENDING) {
        inst->testAppState = TA_TX_WAIT_CONF;
        inst->previousState = TA_TXRESPONSE_WAIT_SEND;
    } else if(eventTypePend == DWT_SIG_RX_TWR_OKAY) {
        inst->instToSleep = TRUE;
        inst->testAppState = TA_TXE_WAIT;
        inst->nextState = TA_TXGRPPOLL_WAIT_SEND;
        //info_printf("finished TWR, slot:%d\n", inst->occupySlot);
    } else if(eventTypePend == DWT_SIG_RX_TWR_ERROR || eventTypePend == DWT_SIG_TX_ERROR){
        change_state_to_listen(inst);
    } else if((inst->twrMode == TWR_INITIATOR) && (inst->previousState == TA_TXRESPONSE_WAIT_SEND)) {
        inst->instToSleep = TRUE;
        inst->testAppState = TA_TXE_WAIT;
        inst->nextState = TA_TXGRPPOLL_WAIT_SEND;
    } else {
        change_state_to_listen(inst);
    }
}

void InstanceTag::handle_error_unknownframe(event_data_t *dw_event) {
    instance_data_t* inst = get_local_structure_ptr(0);

    if(inst->twrMode == TWR_INITIATOR) {
        dw_event->typePend = DWT_SIG_RX_PENDING;
        if(inst->wait4type == UWBMAC_FRM_TYPE_TWR_POLL) {
            inst->remainingRespToRx -= 1;
            dw_event->typePend = setPollRxStamp(inst, 0, 1);
        } else if(inst->wait4type == UWBMAC_FRM_TYPE_TWR_FINAL) {
            inst->remainingRespToRx -= 1;
            dw_event->typePend = setFinalRxStamp(inst, 0, 1);
        } else {
            dw_event->typePend = DWT_SIG_RX_PENDING;
        }
    } else {
        dw_event->typePend = DWT_SIG_DW_IDLE;
    }

    dw_event->type = 0;
    dw_event->rxLength = 0;
    putevent(dw_event, DWT_SIG_RX_TIMEOUT);
}

void InstanceTag::clearBCNLog() {
    instance_data_t* inst = get_local_structure_ptr(0);

    memset(inst->bcnlog, 0, sizeof(beacon_msg_log) * 16);

    depthToGateway = 0xff;
    uploadPort = 0xffff;
}

uint8 InstanceTag::process_beacon_msg(instance_data_t* inst, event_data_t *dw_event, uint16 srcAddr, uint8 *messageData) {
    uint8 clfnum = messageData[CFNOF];

    inst->testAppState = TA_RXE_WAIT;

    inst->bcnlog[clfnum].srcAddr = srcAddr;
    inst->bcnlog[clfnum].slotMap = messageData[SMPOF] + (messageData[SMPOF + 1] << 8);
    inst->bcnlog[clfnum].clusMap = messageData[CMPOF] + (messageData[CMPOF + 1] << 8);
    inst->bcnlog[clfnum].receNum += 1;

    //route update
    if(messageData[DEPTHOF] <= depthToGateway) {
        depthToGateway = messageData[DEPTHOF];
        uploadPort = srcAddr;
    }
    if(messageData[FLGOF] & BCN_GATEWAY) {
        depthToGateway = 0;
        uploadPort = srcAddr;
    }

    if(inst->bcnlog[clfnum].receNum > 1) {
        uint8 i = 0;
        inst->slotMoniMap = 0;
        for(i = 0; i < inst->TWRslots_num; ++i) {
            inst->slotMoniMap |= inst->bcnlog[i].slotMap;
        }
        i = findFirstZeroBit(inst->slotMoniMap, inst->TWRslots_num);
        inst->occupySlot = i;
        if(inst->occupySlot < inst->TWRslots_num) {
            uint8 selectNum = 0;
            int timelength = 0;

            timelength = ((inst->BCNslots_num - clfnum) * inst->BCNslotDuration_ms * 100);
            timelength += (inst->SVCslots_num * inst->SVCslotDuration_ms * 100);
            inst->refSlotStartTime_us = dw_event->uTimeStamp + timelength;
            inst->refSlotStartStamp = (dw_event->timeStamp + convert_usec_to_devtimeu(timelength * 10)) & MASK_TXDTS;

            timelength = inst->occupySlot * inst->TWRslotDuration_ms * 100;
            inst->grpPollTxTime_us = inst->refSlotStartTime_us + timelength;
            inst->grpPollTxStamp = (inst->refSlotStartStamp + convert_usec_to_devtimeu(timelength * 10)) & MASK_TXDTS;

            inst->fatherAddr = inst->bcnlog[clfnum].srcAddr;
            inst->fatherRef = clfnum;

            inst->nextState = TA_TXGRPPOLL_WAIT_SEND;
            inst->testAppState = TA_TXE_WAIT;
            //qDebug() << "found slot:" << inst->occupySlot << ", start[" << (inst->grpPollTxStamp / 256) << "] twr";

            inst->twrmag.flag = 0;
            inst->twrmag.ancPosMask = 0;
            for(i = 0; i < inst->TWRslots_num; ++i) {
                if(inst->bcnlog[i].receNum > 0) {
                    inst->twrmag.flag |= (0x01 << i);
                    inst->twrmag.ancAddr[selectNum] = inst->bcnlog[i].srcAddr;
                    inst->twrmag.idist[i] = 0;
                    inst->twrmag.avgDist[i] = 0;
//                    if(getAncPosition(inst->twrmag.ancAddr[selectNum], inst->twrmag.ancPos[selectNum]) == 0) {
//                        inst->twrmag.ancPosMask |= (0x01 << selectNum);
//                    }
                    selectNum += 1;
                }
                if(selectNum >= 4)	break;
            }
            for(i = selectNum; i < 4; ++i) {
                inst->twrmag.ancAddr[i] = 0xffff;
                inst->twrmag.idist[i] = 0;
                inst->twrmag.avgDist[i] = 0;
            }
            inst->twrmag.pollMask = selectNum;

            //show route path on the scene
            emit tagConnectFinished(inst->instanceAddress16, uploadPort, true);
        }
        for(i = 0; i < inst->TWRslots_num; ++i) {
            inst->bcnlog[i].receNum = 0;
            inst->bcnlog[i].slotMap = 0;
        }
    }
    return 0;
}

uint8 InstanceTag::sent_grppoll_msg(instance_data_t* inst) {
    uint16 *data = NULL;
    uint32 *pos = NULL;
    uint32 txStamp32 = 0;
    uint8  i = 0;

    fill_msg_addr(inst, 0xffff, UWBMAC_FRM_TYPE_TWR_GRP_POLL);

    inst->twrmag.quaFac = 0;
    inst->twrmag.seqNum = inst->rangeNum++;

    data = (uint16 *)&(inst->msg_f.messageData[GP_FLGOF]);
    data[0] = inst->twrmag.flag;
    //data[1] = getElectronicValue();
    for(i = 0; i < 4; ++i) {
        data[2 + i] = inst->twrmag.ancAddr[i];
    }

    inst->msg_f.messageData[GP_QFAOF] = 0;

    inst->twrmag.warnFlag &= 0xfe;

    inst->msg_f.messageData[GP_QFAOF] |= inst->twrmag.warnFlag;
    inst->twrmag.warnFlag = 0;
    if(inst->twrmag.posValid) {
        inst->msg_f.messageData[GP_QFAOF] |= 0x80;
        inst->twrmag.posValid = 0;
    }
    inst->twrmag.seqNum = (inst->twrmag.seqNum + 1) & 0xff;
    inst->msg_f.messageData[GP_SQNOF] = inst->twrmag.seqNum;

    pos = (uint32 *)&(inst->msg_f.messageData[GP_COROF]);
    for(i = 0; i < 3; ++i) {
        pos[i] = inst->twrmag.pos[i];	//×?±ê
    }

    inst->psduLength = (TWR_GRP_POLL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);

    if(coor->sendMsg(this, (uint8 *)&inst->msg_f, inst->psduLength, inst->grpPollTxTime_us) == DWT_ERROR) {
        qDebug() << "error: starttx";
        change_state_to_listen(inst);
        return INST_NOT_DONE_YET;
    }
    inst->twrmag.gpollTxTime = txStamp32;
    inst->twrmag.gpollTxTime = ((inst->twrmag.gpollTxTime << 8) + inst->txAntennaDelay) & MASK_40BIT;
    //debug_printf("T:%u, s:%u\n", (u32)txStamp32, (u32)dwt_readsystimestamphi32());


    inst->remainingRespToRx = MAX_ANCHOR_LIST_SIZE; //expecting 4 anchor responses of poll
    inst->wait4type = UWBMAC_FRM_TYPE_TWR_POLL;
    inst->twrMode = TWR_INITIATOR;
    inst->wait4ack = DWT_RESPONSE_EXPECTED;
    inst->idxOfAnc = 0;
    inst->twrmag.pollMask = 0;
    inst->slotMoniMap = 0;

    //debug_printf("s:%u, c:%d\n",(u32)dwt_readsystimestamphi32(), portGetTickCnt_10us());
    inst->previousState = TA_TXGRPPOLL_WAIT_SEND;
    inst->testAppState = TA_TX_WAIT_CONF;
    return INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set above);
}

uint8 InstanceTag::send_response_msg(instance_data_t* inst) {
    uint32 txStamp32h = 0;

    fill_msg_addr(inst, 0xffff, UWBMAC_FRM_TYPE_TWR_RESP);
    inst->msg_f.messageData[RP_FLGOF] = 0;
    inst->msg_f.messageData[RP_DSLOF] = inst->occupySlot;

    inst->psduLength = (TWR_RESP_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);

    txStamp32h = (inst->grpPollTxStamp >> 8) + inst->fixedOffsetDelay32h + MAX_ANCHOR_LIST_SIZE * inst->fixedPollDelayAnc32h + inst->fixedGuardDelay32h;
    txStamp32h = txStamp32h & MASK_TXT_32BIT;

    uint32 dly = inst->grpPollTxTime_us + RX_RESPONSE_TURNAROUND / 10 + MAX_ANCHOR_LIST_SIZE * inst->fixedPollDelayAnc_us;

    if(coor->sendMsg(this, (uint8 *)&inst->msg_f, inst->psduLength, dly) == DWT_ERROR) {
        return DWT_SIG_TX_ERROR;
    }

    inst->responseTxStamp32h = txStamp32h;
    inst->twrmag.respTxTime = txStamp32h;
    inst->twrmag.respTxTime = ((inst->twrmag.respTxTime << 8) + inst->txAntennaDelay) & MASK_40BIT;

    inst->remainingRespToRx = MAX_ANCHOR_LIST_SIZE;
    inst->wait4type = UWBMAC_FRM_TYPE_TWR_FINAL;
    inst->wait4ack = DWT_RESPONSE_EXPECTED;
    inst->idxOfAnc = 0;
    inst->twrmag.finalMask = 0;
    inst->twrmag.finalCfm = 0;

    return DWT_SIG_TX_PENDING;
}

uint8 InstanceTag::process_poll_msg(uint16 sourceAddress, uint8* recData, event_data_t *event) {
    instance_data_t* inst = get_local_structure_ptr(0);
    uint16 dataSlotMap = (recData[PL_SMPOF + 1] << 8) + recData[PL_SMPOF];
    uint8 i = 0;

    --inst->remainingRespToRx;
    if(inst->remainingRespToRx >= MAX_ANCHOR_LIST_SIZE)	return DWT_SIG_RX_ERROR;
    for(i = MAX_ANCHOR_LIST_SIZE - 1 - inst->remainingRespToRx; i < MAX_ANCHOR_LIST_SIZE; ++i) {
        if(inst->twrmag.ancAddr[i] == sourceAddress) break;
    }
    if(i == MAX_ANCHOR_LIST_SIZE) return DWT_SIG_RX_ERROR;

    inst->twrmag.pollMask |= (0x01 << i);
    inst->twrmag.pollMask += 0x10;

    inst->twrmag.pollRxTime[i] = event->timeStamp;

    inst->slotMoniMap |= dataSlotMap;
    return setPollRxStamp(inst, i, 0);
}

uint8 InstanceTag::process_final_msg(uint16 sourceAddress, uint8* recData, event_data_t *event) {
    instance_data_t* inst = get_local_structure_ptr(0);
    uint8 i = 0;

    --inst->remainingRespToRx;
    if(inst->remainingRespToRx >= MAX_ANCHOR_LIST_SIZE)	return DWT_SIG_RX_ERROR;
    for(i = 0; i < MAX_ANCHOR_LIST_SIZE; ++i) {
        if(inst->twrmag.ancAddr[i] == sourceAddress) break;
    }
    if(i == MAX_ANCHOR_LIST_SIZE) return DWT_SIG_RX_ERROR;

    inst->twrmag.finalMask |= (0x01 << i);
    inst->twrmag.finalMask += 0x10;
    if(recData[FN_FLGOF] == 1) {
        inst->twrmag.finalCfm |= (0x01 << i);
    }
    inst->twrmag.finalRxTime[i] = event->timeStamp;
    //Tx time of Poll message
    memcpy((uint8 *)&inst->twrmag.pollTxTime[i], &recData[FN_TPTOF], 5);
    //Rx time of response message
    memcpy((uint8 *)&inst->twrmag.respRxTime[i], &recData[FN_RRTOF], 5);
    //Tx time of final message
    memcpy((uint8 *)&inst->twrmag.finalTxTime[i], &recData[FN_TFTOF], 5);

    return setFinalRxStamp(inst, i, 0);
}

void InstanceTag::tag_init(instance_data_t* inst) {
    memcpy(inst->eui64, &inst->instanceAddress16, ADDR_BYTE_SIZE_S);

    inst->twrmag.seqNum = 0;
    inst->twrmag.upPeriod = inst->sfPeriod_ms;
    inst->slotMoniMap = 0;
    inst->rangeNum = 0;
    inst->wait4ack = 0;

    config_frameheader_16bit(inst);
    change_state_to_listen(inst);
}

void InstanceTag::rx_to_cb(const event_data_t *cb_data) {
    event_data_t dw_event;

    Q_UNUSED(cb_data)

    dw_event.uTimeStamp = portGetTickCnt_10us();
    handle_error_unknownframe(&dw_event);
}

void InstanceTag::rx_err_cb(const event_data_t *cb_data) {
    event_data_t dw_event;

    Q_UNUSED(cb_data)

    dw_event.uTimeStamp = portGetTickCnt_10us();
    handle_error_unknownframe(&dw_event);
}

void InstanceTag::rx_ok_cb(const event_data_t *cb_data) {
    instance_data_t* inst = get_local_structure_ptr(0);
    uint8 rxTimeStamp[5]  = {0, 0, 0, 0, 0};

    uint8 rxd_event = 0, is_knownframe = 0;
    uint8 msgid_index  = 0, srcAddr_index = 0;
    event_data_t dw_event;

    dw_event.uTimeStamp = portGetTickCnt_10us();
    dw_event.rxLength = cb_data->rxLength;

    if(cb_data->msgu.rxmsg_ss.frameCtrl[0] == 0x41) {
        if((cb_data->msgu.rxmsg_ss.frameCtrl[1] & 0xcc) == 0x88) {
            msgid_index = FRAME_CRTL_AND_ADDRESS_S;
            srcAddr_index = FRAME_CTRLP + ADDR_BYTE_SIZE_S;
            rxd_event = DWT_SIG_RX_OKAY;
        } else if((cb_data->msgu.rxmsg_ss.frameCtrl[1] & 0xcc) == 0x8c) {
            msgid_index = FRAME_CRTL_AND_ADDRESS_LS;
            srcAddr_index = FRAME_CTRLP + ADDR_BYTE_SIZE_L;
            rxd_event = DWT_SIG_RX_OKAY;
        } else {
            rxd_event = SIG_RX_UNKNOWN;
        }
    } else {
        rxd_event = SIG_RX_UNKNOWN;
    }

    memcpy((uint8 *)&dw_event.msgu.frame[0], (uint8 *)&cb_data->msgu.frame[0], cb_data->rxLength);
    seteventtime(&dw_event, rxTimeStamp);

    dw_event.type = 0;
    dw_event.typePend = DWT_SIG_DW_IDLE;

    //Process good/known frame types
    if(rxd_event == DWT_SIG_RX_OKAY) {
        uint16 sourceAddress = (((uint16)dw_event.msgu.frame[srcAddr_index+1]) << 8) + dw_event.msgu.frame[srcAddr_index];
        uint16 destAddress = (((uint16)dw_event.msgu.frame[srcAddr_index-1]) << 8) + dw_event.msgu.frame[srcAddr_index-2];

        if((destAddress != 0xffff) && (destAddress != inst->instanceAddress16)) {
            handle_error_unknownframe(&dw_event);
            return;
        }
        switch(dw_event.msgu.frame[msgid_index]) {
            case UWBMAC_FRM_TYPE_TWR_POLL:
                if(inst->twrMode == TWR_INITIATOR) {
                    dw_event.typePend = process_poll_msg(sourceAddress, &dw_event.msgu.frame[msgid_index], &dw_event);
                    is_knownframe = 1;
                }
                break;
            case UWBMAC_FRM_TYPE_TWR_FINAL:
                if(inst->twrMode == TWR_INITIATOR) {
                    dw_event.typePend = process_final_msg(sourceAddress, &dw_event.msgu.frame[msgid_index], &dw_event);
                    is_knownframe = 1;
                }
                break;
            case UWBMAC_FRM_TYPE_BCN:
            case UWBMAC_FRM_TYPE_SVC:
            case UWBMAC_FRM_TYPE_DOWNLOAD_RESP:
                is_knownframe = 1;
                break;
            case UWBMAC_FRM_TYPE_CL_JOIN:
            case UWBMAC_FRM_TYPE_TWR_GRP_POLL:
            case UWBMAC_FRM_TYPE_TWR_RESP:
            default:
                is_knownframe = 0;
                break;
        }

    }
    if(is_knownframe == 1) {
        putevent(&dw_event, rxd_event);
    } else {
        handle_error_unknownframe(&dw_event);
    }
}

int InstanceTag::app_run(instance_data_t *inst) {
    int instDone = INST_NOT_DONE_YET;
    int message = peekevent();
    uint8 ldebug = 0;
    static uint8 lastState = 1;

    switch(inst->testAppState) {
        case TA_INIT:
            if(ldebug == 1) {
                qDebug() << "TA_INIT";
            }
            tag_init(inst);
            break;

        case TA_SLEEP_DONE:
        {
            event_data_t* dw_event = getevent(10);
            if(ldebug == 1 && lastState != inst->testAppState) {
                qDebug() << TA_SLEEP_DONE;
                lastState = inst->testAppState;
            }
            //wait timeout for wake up the DW1000 IC
            if (dw_event->type != DWT_SIG_RX_TIMEOUT) {
                instDone = INST_DONE_WAIT_FOR_NEXT_EVENT; //wait here for sleep timeout
                break;
            }
            instDone = INST_NOT_DONE_YET;
            inst->instToSleep = FALSE ;
            inst->testAppState = inst->nextState;
            inst->nextState = TA_INIT; //clear
            inst->instanceWakeTime_us = portGetTickCnt_10us(); // Record the time count when IC wake-up

#if (DEEP_SLEEP == 1)
            inst->grpPollTxStamp = inst->grpPollTxStamp * 256;
            inst->grpPollTxStamp += convert_usec_to_devtimeu((inst->refSlotStartTime_us + inst->occupySlot * inst->TWRslotDuration_ms * 100 - portGetTickCnt_10us()) * 10);

            inst->grpPollTxTime_us = inst->refSlotStartTime_us + inst->occupySlot * inst->TWRslotDuration_ms * 100;
#endif
       }
            break;

        case TA_TXE_WAIT:
            if(ldebug == 1) {
                qDebug() << "TA_TXE_WAIT";
            }

            //go to sleep before sending the next group poll/ starting new ranging exchange
            if((inst->nextState == TA_TXGRPPOLL_WAIT_SEND) && (inst->instToSleep)) {
                inst->rangeNum++; 							//increment the range number before going to sleep
                instDone = INST_DONE_WAIT_FOR_NEXT_EVENT_TO;//wait to timeout
                inst->testAppState = TA_SLEEP_DONE;			//change to sleep state
            } else {
                //in otheer situation, change to the nest state
                inst->testAppState = inst->nextState;
                inst->nextState = TA_INIT; //clear
            }
            break;

        case TA_TXGRPPOLL_WAIT_SEND:
            if(ldebug == 1) {
                qDebug() << "TA_TXGRPPOLL_WAIT_SEND";
            }
            instDone = sent_grppoll_msg(inst);
            break;

        case TA_DOWNLOAD_REQ_WAIT_SEND:
            //instDone = send_download_req_msg(inst, DWT_START_TX_DELAYED);
            break;
        case TA_TX_WAIT_CONF:
        {
            event_data_t *dw_event = getevent(11);	//get and clear this event
            if(ldebug == 1 && lastState != inst->testAppState) {
                qDebug() << "TX_WAIT_CON";
                lastState = inst->testAppState;
            }
            if(dw_event->type != DWT_SIG_TX_DONE) {
                instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
                break;
            }
            instDone = INST_NOT_DONE_YET;
            if(inst->previousState == TA_TXRESPONSE_WAIT_SEND) {
                inst->responseTxStamp32h = dw_event->timeStamp32h;
                inst->twrmag.respTxTime = dw_event->timeStamp;
            } else if(inst->previousState == TA_TXGRPPOLL_WAIT_SEND) {
                inst->grpPollTxStamp = dw_event->timeStamp;
                inst->twrmag.gpollTxTime = dw_event->timeStamp;
            }
            inst->testAppState = TA_RXE_WAIT;
        }
            break;

        case TA_RXE_WAIT:
            if(ldebug == 1) {
                qDebug() << "RXE_WAIT";
            }
            //if this is set the RX will turn on automatically after TX
            if(inst->wait4ack == 0) {
            } else {
                //clear the flag, the next time we want to turn the RX on it might not be auto
                inst->wait4ack = 0;
            }
            instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
            inst->testAppState = TA_RX_WAIT_DATA;
            if(message == 0)	break;

        case TA_RX_WAIT_DATA:
            if(ldebug == 1 && lastState != inst->testAppState) {
                lastState = inst->testAppState;
                qDebug() << "R_W_DATA";
            }
            switch(message) {
                case DWT_SIG_RX_OKAY:
                {
                    event_data_t* dw_event = getevent(15);
                    uint16 srcAdd16 = dw_event->msgu.rxmsg_ss.sourceAddr[0] + (dw_event->msgu.rxmsg_ss.sourceAddr[1] << 8);
                    uint8 msgID = 0;
                    uint8 *messageData = NULL;
                    if(ldebug == 1) {
                        qDebug() << "RX_OKAY";
                    }

                    msgID = dw_event->msgu.rxmsg_ss.messageData[FCODE];
                    messageData = &(dw_event->msgu.rxmsg_ss.messageData[0]);

                    switch(msgID) {
                        case UWBMAC_FRM_TYPE_BCN:
                            //qDebug() << "rbea";
                            process_beacon_msg(inst, dw_event, srcAdd16, messageData);
                            break;
                        case UWBMAC_FRM_TYPE_TWR_POLL:
                            process_rx_event(inst, dw_event->typePend);
                            //qDebug() << "rpol";
                            break;
                        case UWBMAC_FRM_TYPE_TWR_FINAL:
                            process_rx_event(inst, dw_event->typePend);

                            break;
                        case UWBMAC_FRM_TYPE_DOWNLOAD_RESP:
                            break;
                        default:
                            break;
                    }
                }
                    break;

                case DWT_SIG_RX_TIMEOUT:
                {
                    event_data_t* dw_event = getevent(17);//get and clear the event
                    process_rx_event(inst, dw_event->typePend);
                    message = 0;
                }
                    break;

                default:
                    if(message) {
                        getevent(20);
                    }
                    if(instDone == INST_NOT_DONE_YET) {
                        instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
                    }
                    break;
            }
            break;

        default:
            break;
    }

    return instDone;
}

int InstanceTag::run() {
    instance_data_t *inst = get_local_structure_ptr(0);
    int done = INST_NOT_DONE_YET;

    //只负责监听网络，不实现twr测距协议，因为无意义

    while(done == INST_NOT_DONE_YET) {
        done = app_run(inst);
        if(peekevent() != 0) {
            done = INST_NOT_DONE_YET;
        }
    }

    return 0;
}
