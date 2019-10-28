#include "InstanceAnch.h"

InstanceAnch::InstanceAnch(Coordinator *co)
{
    coor = co;
    memset(&posUWBBuf, 0, sizeof(PosUWBBufStr));
}

void InstanceAnch::addTagPos_ToBuf(uint16 addr, uint8* pos, u8 alarm, uint16 elecValue, u8 seqNum) {
    uint16 offset = posUWBBuf.length;
    if((offset + ONE_POS_INFO_SIZE) > POS_UWB_BUF_MAX_SIZE)	return;
    posUWBBuf.data[offset + 0] = addr & 0xff;
    posUWBBuf.data[offset + 1] = (addr >> 8) & 0xff;
    memcpy(&posUWBBuf.data[offset + 2], pos, 12);
    posUWBBuf.data[offset + 14] = alarm;
    posUWBBuf.data[offset + 15] = elecValue & 0xff;
    posUWBBuf.data[offset + 16] = (elecValue >> 8) & 0xff;
    posUWBBuf.data[offset + 17] = seqNum;
    posUWBBuf.length = offset + 18;
}

uint8 InstanceAnch::cpy_to_PosBuf(uint8 num, uint8 *src) {
    int8 leftNum = 4 - posUWBBuf.num;
    if(leftNum <= 0) {
        leftNum = 0;
    }
    if(num < leftNum) {
        leftNum = num;
    }
    if(leftNum > 0) {
        memcpy(&(posUWBBuf.data[posUWBBuf.length]), src, ONE_POS_INFO_SIZE * leftNum);
        posUWBBuf.length += ONE_POS_INFO_SIZE * leftNum;
    }
    return (4 - leftNum);
}

void InstanceAnch::clearTagPosBuf(void) {
    posUWBBuf.length = 0;
    posUWBBuf.num = 0;
}

void InstanceAnch::anch_no_timeout_rx_reenable(void) {
    //dwt_setrxtimeout(0); //reconfigure the timeout
    //dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

void InstanceAnch::anch_fill_msg_addr(instance_data_t* inst, uint16 dstaddr, uint8 msgid) {
    inst->msg_f.sourceAddr[0] = inst->instanceAddress16 & 0xff; //copy the address
    inst->msg_f.sourceAddr[1] = (inst->instanceAddress16 >> 8) & 0xff;
    inst->msg_f.destAddr[0] = dstaddr & 0xff;
    inst->msg_f.destAddr[1] = (dstaddr >> 8) & 0xff;
    inst->msg_f.seqNum = inst->frameSN++;
    inst->msg_f.messageData[MIDOF] = msgid;
}

void InstanceAnch::prepare_beacon_frame(instance_data_t* inst, uint8_t flag) {
    anch_fill_msg_addr(inst, 0xffff, UWBMAC_FRM_TYPE_BCN);
    inst->msg_f.messageData[SIDOF] = inst->bcnmag.sessionID;
    inst->msg_f.messageData[FLGOF] = flag;
    inst->msg_f.messageData[CSNOF] = inst->bcnmag.clusterSlotNum;
    inst->msg_f.messageData[CFNOF] = inst->bcnmag.clusterFrameNum;
    inst->msg_f.messageData[CMPOF] = inst->bcnmag.clusterSelfMap & 0xff;
    inst->msg_f.messageData[CMPOF+1] = (inst->bcnmag.clusterSelfMap >> 8) & 0xff;
    inst->msg_f.messageData[NMPOF] = inst->bcnmag.clusterNeigMap & 0xff;
    inst->msg_f.messageData[NMPOF+1] = (inst->bcnmag.clusterNeigMap >> 8) & 0xff;

    inst->bcnmag.slotSelfMap = inst->bcnmag.nextSlotMap;
    inst->bcnmag.nextSlotMap = 0;

    inst->msg_f.messageData[SMPOF] = inst->bcnmag.slotSelfMap & 0xff;
    inst->msg_f.messageData[SMPOF+1] = (inst->bcnmag.slotSelfMap >> 8) & 0xff;

    inst->msg_f.messageData[VEROF] = 0;       //(getVersionOfArea() << 4) | getVersionOfAncs();

    inst->psduLength = 0;
    if(flag & BCN_EXT) {
        inst->msg_f.messageData[VEROF+1] = inst->jcofmsg.msgID;
        inst->msg_f.messageData[VEROF+2] = inst->jcofmsg.chldAddr & 0xff;
        inst->msg_f.messageData[VEROF+3] = (inst->jcofmsg.chldAddr>>8) & 0xff;
        inst->msg_f.messageData[VEROF+4] = --inst->jcofmsg.clusterLock;
        inst->msg_f.messageData[VEROF+5] = 0xff;
        if((inst->bcnmag.clusterSelfMap & (0x01 << inst->jcofmsg.clusterSeat)) == 0) {
            inst->msg_f.messageData[VEROF+5] = inst->jcofmsg.clusterSeat;
        }
        inst->psduLength = 5;

        if(inst->jcofmsg.clusterLock == 0) {
            inst->bcnmag.bcnFlag &= (~BCN_EXT);
            inst->bcnmag.clusterSelfMap |= (0x01 << inst->jcofmsg.clusterSeat);
        }
    } else if(posUWBBuf.num > 0){
        if((posUWBBuf.num * ONE_POS_INFO_SIZE) == posUWBBuf.length) {
            inst->msg_f.messageData[FLGOF] |= BCN_EXT;
            inst->msg_f.messageData[VEROF+1] = UWBMAC_FRM_TYPE_TAGS_POS;
            inst->msg_f.messageData[VEROF+2] = inst->fatherAddr & 0xff;
            inst->msg_f.messageData[VEROF+3] = (inst->fatherAddr>>8) & 0xff;
            inst->msg_f.messageData[VEROF+4] = posUWBBuf.num;
            memcpy(&(inst->msg_f.messageData[VEROF+5]), posUWBBuf.data, posUWBBuf.length);
            inst->psduLength = posUWBBuf.length + 4;
        }
        clearTagPosBuf();
    }
    inst->psduLength += (BCN_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);
}

uint8 InstanceAnch::prepare_join_request_frame(instance_data_t* inst) {
    uint8_t i = 0;
    uint16_t mask = 0x01;

    anch_fill_msg_addr(inst, 0xffff, UWBMAC_FRM_TYPE_CL_JOIN);
    for(i = 0; i < inst->BCNslots_num; i++) {
        if((inst->bcnmag.clusterNeigMap & mask) == 0x00) {
            inst->bcnmag.clusterFrameNum = i;
            break;
        }
        mask <<= 1;
    }
    if(i == inst->BCNslots_num)	{
        qDebug() << inst->instanceAddress16 << "unfound idle slot:" << inst->bcnmag.clusterNeigMap;
        return 1;
    }

    inst->msg_f.messageData[REQOF] = inst->bcnmag.clusterFrameNum;

    inst->psduLength = (JOIN_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);

    connectSrcAddrs.clear();

    return 0;
}

void InstanceAnch::BCNLog_Clear(instance_data_t* inst) {
    memset((void *)&inst->bcnlog, 0, sizeof(beacon_msg_log) * 16);
    return;
}

uint8 InstanceAnch::process_beacon_msg(instance_data_t* inst, event_data_t *dw_event, uint16 srcAddr16, uint8 *messageData) {
    uint16_t clustermap = messageData[CMPOF] + (messageData[CMPOF+1] << 8);
    uint16_t neigmap = messageData[NMPOF] + (messageData[NMPOF+1] << 8);
    uint16 addr = 0;
    u8 srcClusterFraNum = messageData[CFNOF];

    inst->testAppState = TA_RXE_WAIT;

    if(messageData[FLGOF] & BCN_EXT) {
        uint8 *dat = messageData + BCN_MSG_LEN;
        addr = dat[1] + (dat[2] << 8);

        if((inst->wait4type == dat[0]) && (dat[0] == UWBMAC_FRM_TYPE_CL_JOIN_CFM)) {
            if((addr == inst->instanceAddress16) && (inst->bcnmag.clusterFrameNum < 16)) {
                qDebug() << "Rec[" << srcAddr16 << "] Join Cof: " << addr << "[" << dat[4] << "]";

                if((inst->bcnmag.clusterFrameNum == dat[4]) && (inst->bcnlog[srcClusterFraNum].flag & 0x01)) {
                    inst->bcnlog[srcClusterFraNum].flag &= (~0x01);
                    inst->wait4ackNum -= 1;

                    connectSrcAddrs.insert(srcAddr16);

                    if(inst->wait4ackNum == 0) {
                        inst->joinedNet = 1;
                        inst->wait4type = 0;
                        inst->bcnmag.clusterSelfMap = 0x01 << inst->bcnmag.clusterFrameNum;
                        inst->bcnmag.clusterNeigMap = 0;
                        inst->bcnmag.slotSelfMap = 0;

                        qDebug() << inst->instanceAddress16 << "success to join net:" << inst->bcnmag.clusterFrameNum;

                        emit netConnectFinished(inst->instanceAddress16, connectSrcAddrs, inst->bcnmag.clusterFrameNum);

                        BCNLog_Clear(inst);
                    }
                    return 0;
                } else if(dat[4] == 0xff) {
                    inst->bcnmag.clusterFrameNum = 0xff;
                    inst->fatherRef = 0xff;
                    inst->wait4type = 0;
                    BCNLog_Clear(inst);
                }
            } else {
                inst->bcnmag.clusterFrameNum = 0xff;
                inst->fatherRef = 0xff;
                inst->wait4type = 0;
                BCNLog_Clear(inst);
            }
        } else if((dat[0] == UWBMAC_FRM_TYPE_TAGS_POS) && (addr == inst->instanceAddress16)) {
            if(inst->gatewayAnchor) {
                //cpy_from_UWBBuf_to_PosNetBuf(dat[4], &dat[5]);
                qDebug() << "add tag info to service";
            } else {
                //printf("%d tag info overflow:%d\n", dat[4], cpy_to_PosBuf(dat[4], &dat[5]));
                qDebug() << dat[4] << "tag info overflow: " << cpy_to_PosBuf(dat[4], &dat[5]);
            }
        }
    }

    if(inst->joinedNet > 0) {
        inst->bcnmag.clusterNeigMap |= clustermap;
        inst->bcnmag.clusterSelfMap |= (0x01 << srcClusterFraNum);

        if(srcAddr16 == inst->fatherAddr && (!inst->gatewayAnchor)) {
            //uint8 version = messageData[VEROF] & 0x0f;
            int dif = inst->bcnmag.clusterFrameNum - srcClusterFraNum;

            inst->sampleNum += 1;

            inst->fatherRefTime = dw_event->timeStamp;
            inst->instanceTimerEn = 1;
            inst->beaconTXTimeCnt = dw_event->uTimeStamp + dif * inst->BCNslotDuration_ms * 100;
            inst->svcStartStamp32h = dw_event->timeStamp32h + (inst->BCNslots_num - srcClusterFraNum) * inst->BCNfixTime32h;
            if(dif <= 0) {
                inst->beaconTXTimeCnt += inst->sfPeriod_ms * 100;
            }
        } else if(inst->gatewayAnchor) {

        }
        if(inst->sampleNum >= 5) {
            inst->bcnmag.clusterSelfMap = (0x01 << inst->bcnmag.clusterFrameNum);
            inst->bcnmag.clusterNeigMap = 0;
            inst->sampleNum = 0;
            BCNLog_Clear(inst);
        }
        if(srcClusterFraNum < 16) {
            inst->bcnlog[srcClusterFraNum].srcAddr = srcAddr16;
            inst->bcnlog[srcClusterFraNum].clusMap = clustermap;
            inst->bcnlog[srcClusterFraNum].neigMap = neigmap;
        }
        return 0;
    }

    if(srcClusterFraNum < inst->fatherRef) {
        inst->fatherRef = srcClusterFraNum;
        inst->fatherAddr = srcAddr16;
        inst->sampleNum = 0;
        inst->bcnmag.clusterNeigMap = 0;
        inst->bcnmag.sessionID = messageData[SIDOF];
        inst->wait4ackNum = 0;
        return 0;
    } else if(srcClusterFraNum == inst->fatherRef) {
        inst->sampleNum += 1;
    }

    inst->bcnmag.clusterNeigMap |= clustermap;

    if(srcClusterFraNum < 16) {
        inst->bcnlog[srcClusterFraNum].srcAddr = srcAddr16;
        inst->bcnlog[srcClusterFraNum].clusMap = clustermap;
        inst->bcnlog[srcClusterFraNum].neigMap = neigmap;
        if((inst->bcnlog[srcClusterFraNum].flag & 0x01) == 0) {
            inst->bcnlog[srcClusterFraNum].flag |= 0x01;
            inst->wait4ackNum += 1;
        }
    }

    if(inst->sampleNum > 5 && inst->joinedNet == 0) {
        inst->sampleNum = 0;
        if(prepare_join_request_frame(inst)) {
            inst->bcnmag.clusterFrameNum = 0xff;
            inst->fatherRef = 0xff;
            BCNLog_Clear(inst);
            inst->testAppState = TA_RXE_WAIT;
            return 0;
        }
        inst->bcnmag.clusterNeigMap = 0;
        inst->svcStartStamp32h = dw_event->timeStamp32h + inst->BCNfixTime32h * (inst->BCNslots_num - messageData[CFNOF]);

        uint32 delay = dw_event->uTimeStamp + inst->BCNslotDuration_ms * (inst->BCNslots_num - messageData[CFNOF]) * 100;

        if(coor->sendMsg(this, (uint8 *)&inst->msg_f, inst->psduLength, delay) == DWT_ERROR) {
            qDebug() << "error: join net request error!\n";
            inst->bcnmag.clusterFrameNum = 0xff;
            inst->fatherRef = 0xff;
            BCNLog_Clear(inst);
            return 1;
        }

        qDebug() << inst->instanceAddress16 << " join net requesting [" << inst->bcnmag.clusterFrameNum << "]-num[" << inst->wait4ackNum << "]";

        inst->wait4ack = DWT_RESPONSE_EXPECTED;
        inst->wait4type = UWBMAC_FRM_TYPE_CL_JOIN_CFM;
        inst->testAppState = TA_TX_WAIT_CONF; // wait confirmation
        inst->previousState = TA_TXSVC_WAIT_SEND;
    }
    return 0;
}

uint8 InstanceAnch::process_join_request_msg(instance_data_t* inst, event_data_t *dw_event, uint16 srcAddr16, uint8 *messageData) {
    uint8 seat = messageData[REQOF];
    uint16 mask = 0x01 << seat;

    Q_UNUSED(dw_event)

    inst->testAppState = TA_RXE_WAIT;

    if((inst->joinedNet > 0) && (inst->jcofmsg.clusterLock == 0) && ((inst->bcnmag.clusterSelfMap & mask) == 0)) {
        inst->bcnmag.bcnFlag |= BCN_EXT;
        inst->jcofmsg.chldAddr = srcAddr16;
        inst->jcofmsg.clusterLock = 8;
        inst->jcofmsg.clusterSeat = seat;
        inst->jcofmsg.msgID = UWBMAC_FRM_TYPE_CL_JOIN_CFM;

        qDebug() << srcAddr16 << "Req Join seat [" << seat << "]";
    } else if(inst->joinedNet > 0){
        if(inst->jcofmsg.clusterLock != 0) {
            qDebug() << srcAddr16 << "Req Join seat [" << seat << "], but busy:" << inst->jcofmsg.clusterLock;
        } else if((inst->bcnmag.clusterSelfMap & mask) == 0) {
            qDebug() << srcAddr16 << "Req Join seat [" << seat << "], but seat be ocupied!";
        }
    }
    return 0;
}

void InstanceAnch::send_beacon_msg(instance_data_t* inst, uint8 flag) {
    if(inst->gatewayAnchor) {
        prepare_beacon_frame(inst, BCN_INIT | flag);
        inst->sampleNum += 1;
    } else {
        prepare_beacon_frame(inst, flag);
    }

    coor->sendMsg(this, (uint8 *)&inst->msg_f, inst->psduLength, portGetTickCnt_10us());

    if(inst->gatewayAnchor) {
        inst->instanceTimerEn = 1;
        inst->beaconTXTimeCnt = portGetTickCnt_10us() + 100 * inst->sfPeriod_ms;
    }

    inst->testAppState = TA_TX_WAIT_CONF;
    inst->previousState = TA_TXBCN_WAIT_SEND;
}

uint8 InstanceAnch::process_grp_poll_msg(uint16 srcAddr, uint8* recData, event_data_t *event) {
    uint16 *data = (uint16 *)&recData[GP_FLGOF];
    instance_data_t* inst = get_local_structure_ptr(0);
    uint16 mask = 0x01 << (inst->bcnmag.clusterFrameNum);

    if((data[0] & mask) == mask) {
        uint32 timeStamp32 = 0;

        for(inst->idxOfAnc = 0; inst->idxOfAnc < 4; ++inst->idxOfAnc) {
            if(data[2+inst->idxOfAnc] == inst->instanceAddress16)	break;
        }
        if(inst->idxOfAnc >= 4)	return 1;

        anch_fill_msg_addr(inst, srcAddr, UWBMAC_FRM_TYPE_TWR_POLL);
        inst->msg_f.messageData[PL_FLGOF] =	0;
        inst->msg_f.messageData[PL_SMPOF + 0] = inst->bcnmag.nextSlotMap & 0xff;
        inst->msg_f.messageData[PL_SMPOF + 1] = (inst->bcnmag.nextSlotMap >> 8) & 0xff;

        inst->psduLength = FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC + TWR_POLL_MSG_LEN;

        inst->delayedTRXTime32h = event->timeStamp32h;//µ¥Î»4ns
        timeStamp32 = (inst->delayedTRXTime32h + inst->fixedOffsetDelay32h + (inst->idxOfAnc + 1) * inst->fixedPollDelayAnc32h) & MASK_TXT_32BIT;

        uint32 dely = event->uTimeStamp + (inst->fixedOffsetDelay32h + (inst->idxOfAnc + 1) * inst->fixedPollDelayAnc32h) / 2500;

        if(coor->sendMsg(this, (uint8 *)&inst->msg_f, inst->psduLength, dely) == DWT_ERROR) {
            qDebug() << "error:time is pass: " << timeStamp32;
            return 1;
        }
        inst->twrmag.pollTxTime[0] = timeStamp32;
        inst->twrmag.pollTxTime[0] = ((inst->twrmag.pollTxTime[0] << 8) + inst->txAntennaDelay) & MASK_40BIT;
        inst->twrmag.gpollRxTime[0] = event->timeStamp;

        inst->wait4ack = DWT_RESPONSE_EXPECTED;
        inst->wait4type = UWBMAC_FRM_TYPE_TWR_RESP;
        inst->wait4ackOfAddr = srcAddr;

        return 0;
    } else {
        inst->wait4ackOfAddr = 0xffff;
    }
    return 1;
}

uint8 InstanceAnch::process_response_msg(uint16 srcAddr, uint8* recData, event_data_t *event) {
    instance_data_t* inst = get_local_structure_ptr(0);
    uint16 mask = 0x01 << recData[RP_DSLOF];

    if((inst->wait4type == UWBMAC_FRM_TYPE_TWR_RESP) && (inst->wait4ackOfAddr == srcAddr)) {
        uint8 idxOfAnc = 3 - inst->idxOfAnc;
        uint32 timeStamp32 = (event->timeStamp32h + (idxOfAnc + 1) * inst->fixedFinalDelayAnc32h) & MASK_TXT_32BIT;
        uint32 dely = event->uTimeStamp + ((idxOfAnc + 1) * inst->fixedFinalDelayAnc32h) / 2500;

        inst->twrmag.finalTxTime[0] = timeStamp32;
        inst->twrmag.finalTxTime[0] = ((inst->twrmag.finalTxTime[0] << 8) + inst->txAntennaDelay) & MASK_40BIT;

        /********************start:11?¡§final msg??*************************/
        anch_fill_msg_addr(inst, srcAddr, UWBMAC_FRM_TYPE_TWR_FINAL);
        inst->msg_f.messageData[FN_FLGOF] = ((mask & inst->bcnmag.nextSlotMap) == 0) ? 1 : 0;
        inst->bcnmag.nextSlotMap |= mask;
        //Tx time of Poll message-- or ------Db(plllTxTime - gpollRxTime)
        inst->twrmag.pollTxTime[1] = inst->twrmag.pollTxTime[0] - inst->twrmag.gpollRxTime[0];
        memcpy(&(inst->msg_f.messageData[FN_TPTOF]), (uint8 *)&inst->twrmag.pollTxTime[1], 5);
        //Rx time of response message or ----Rb(respRxTime - plllTxTime)
        inst->twrmag.respRxTime[1] = event->timeStamp - inst->twrmag.pollTxTime[0];
        memcpy(&(inst->msg_f.messageData[FN_RRTOF]), (uint8 *)&inst->twrmag.respRxTime[1], 5);
        //Tx time of final message or -------Db1(finalTxTime - respRxTime)
        inst->twrmag.finalTxTime[1] = inst->twrmag.finalTxTime[0] - event->timeStamp;
        memcpy(&(inst->msg_f.messageData[FN_TFTOF]), (uint8 *)&inst->twrmag.finalTxTime[1], 5);

        inst->psduLength = FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC + TWR_FINAL_MSG_LEN;

        inst->wait4ackOfAddr = 0xffff;
        inst->wait4type = 0;

        inst->delayedTRXTime32h = event->timeStamp32h;

        if(coor->sendMsg(this, (uint8 *)&inst->msg_f, inst->psduLength, dely) == DWT_ERROR) {
            qDebug() << "error:time is pass:" << timeStamp32;
            return 1;
        }
        inst->wait4ack = DWT_RESPONSE_EXPECTED;

        return 0;
    }
    return 1;
}

void InstanceAnch::anchor_init(instance_data_t* inst) {
    memcpy(inst->eui64, &inst->instanceAddress16, ADDR_BYTE_SIZE_S);

    inst->shortAdd_idx = (inst->instanceAddress16 & 0xff);
    if(inst->instanceAddress16 == 0x0000) {
        inst->gatewayAnchor = TRUE;
    } else {
        inst->gatewayAnchor = FALSE;
    }

    config_frameheader_16bit(inst);

    if(inst->gatewayAnchor) {
        inst->testAppState = TA_TXBCN_WAIT_SEND;
        inst->joinedNet = 1;
        inst->bcnmag.sessionID = 0x55;
        inst->fatherAddr = inst->instanceAddress16;
        inst->bcnmag.clusterSlotNum = 15;
        inst->bcnmag.clusterFrameNum = 0;
        inst->bcnmag.clusterSelfMap = 0x0001;
        while(portGetTickCnt_10us() % (inst->sfPeriod_ms * 100) != 0);
    } else {
        inst->wait4ack = 0;
        inst->sampleNum = 0;
        inst->fatherRef = 0xff;
        inst->joinedNet = 0;
        inst->bcnmag.clusterFrameNum = 0;
        inst->bcnmag.clusterSelfMap = 0x0000;
        inst->testAppState = TA_RXE_WAIT ;
    }
    inst->bcnmag.clusterNeigMap = 0x0000;
    inst->bcnmag.slotSelfMap = 0x0000;
    inst->bcnmag.nextSlotMap = 0x0000;
    inst->bcnmag.bcnFlag = 0;

    inst->jcofmsg.clusterLock = 0;

    BCNLog_Clear(inst);
}


/****************************************************************************************************
 * this function handles frame error event, it will either signal timeout or re-enable the receiver
 ***************************************************************************************************/
void InstanceAnch::anch_handle_error_unknownframe_timeout(event_data_t *dw_event) {
    //instance_data_t* inst = get_local_structure_ptr(0);
    Q_UNUSED(dw_event)

    dw_event->type = 0;
    dw_event->rxLength = 0;
}

/************************************************************
 * @brief this is the receive timeout event callback handler
 ************************************************************/
void InstanceAnch::rx_to_cb(const event_data_t *cb_data) {
    event_data_t dw_event;

    Q_UNUSED(cb_data)

    dw_event.uTimeStamp = portGetTickCnt_10us();
    anch_handle_error_unknownframe_timeout(&dw_event);
}

/***********************************************************
 * @brief this is the receive error event callback handler
 **********************************************************/
void InstanceAnch::rx_err_cb(const event_data_t *cb_data) {
    event_data_t dw_event;

    Q_UNUSED(cb_data)

    dw_event.uTimeStamp = portGetTickCnt_10us();
    anch_handle_error_unknownframe_timeout(&dw_event);
}

void InstanceAnch::rx_ok_cb(const event_data_t *cb_data) {
    instance_data_t* inst = get_local_structure_ptr(0);
    uint8 rxTimeStamp[5]  = {0, 0, 0, 0, 0};
    uint8 rxd_event = 0, is_knownframe = 0;
    uint8 msgid_index  = 0, srcAddr_index = 0;
    event_data_t dw_event;

    dw_event.uTimeStamp = portGetTickCnt_10us();
    dw_event.rxLength = cb_data->rxLength;

    if((cb_data->msgu.rxmsg_ss.frameCtrl[0] == 0x41) && ((cb_data->msgu.rxmsg_ss.frameCtrl[1] & 0xcc) == 0x88)) {
        msgid_index = FRAME_CRTL_AND_ADDRESS_S;
        srcAddr_index = FRAME_CTRLP + ADDR_BYTE_SIZE_S;
        rxd_event = DWT_SIG_RX_OKAY;
    } else {
        rxd_event = SIG_RX_UNKNOWN;	//not supported
    }

    memcpy((uint8 *)&dw_event.msgu.frame[0], (uint8 *)&cb_data->msgu.frame[0], cb_data->rxLength);
    seteventtime(&dw_event, rxTimeStamp);
    dw_event.type = 0;
    dw_event.typePend = DWT_SIG_DW_IDLE;

    //Process good/known frame types
    if(rxd_event == DWT_SIG_RX_OKAY) {
        uint16 srcAddress = (((uint16)dw_event.msgu.frame[srcAddr_index+1]) << 8) + dw_event.msgu.frame[srcAddr_index];
        uint16 dstAddress = (((uint16)dw_event.msgu.frame[srcAddr_index-1]) << 8) + dw_event.msgu.frame[srcAddr_index-2];

        if((dw_event.msgu.rxmsg_ss.panID[0] != (inst->panID & 0xff)) || (dw_event.msgu.rxmsg_ss.panID[1] != (inst->panID >> 8))) {
            //dwt_rxenable(DWT_START_RX_IMMEDIATE);//anch_handle_error_unknownframe_timeout(&dw_event);
            return;
        }
        if(dstAddress != 0xffff && dstAddress != inst->instanceAddress16) {
            //dwt_rxenable(DWT_START_RX_IMMEDIATE);//anch_handle_error_unknownframe_timeout(&dw_event);
            return;
        }

        switch(dw_event.msgu.frame[msgid_index]) {
            case UWBMAC_FRM_TYPE_TWR_GRP_POLL:
                if(inst->mode == ANCHOR && process_grp_poll_msg(srcAddress, &dw_event.msgu.frame[msgid_index], &dw_event) == 0) {
                    dw_event.typePend = DWT_SIG_TX_PENDING;
                    is_knownframe = 1;
                }
                break;

            case UWBMAC_FRM_TYPE_TWR_RESP:
                if(inst->mode == ANCHOR && process_response_msg(srcAddress, &dw_event.msgu.frame[msgid_index], &dw_event) == 0) {
                    dw_event.typePend = DWT_SIG_TX_PENDING;
                    is_knownframe = 1;
                }
                break;

            case UWBMAC_FRM_TYPE_DOWNLOAD_REQ:
//                if(process_download_req(srcAddress, &dw_event.msgu.frame[msgid_index]) == 0) {
//                    dw_event.typePend = DWT_SIG_TX_PENDING;
//                    is_knownframe = 1;
//                }
                break;
            case UWBMAC_FRM_TYPE_BCN:
            case UWBMAC_FRM_TYPE_SVC:
            case UWBMAC_FRM_TYPE_CL_JOIN:
            case UWBMAC_FRM_TYPE_CL_JOIN_CFM:
            case UWBMAC_FRM_TYPE_POS:
            case UWBMAC_FRM_TYPE_DOWNLOAD_RESP:
            case UWBMAC_FRM_TYPE_ALMA:
                is_knownframe = 1;
                break;
            case UWBMAC_FRM_TYPE_TWR_POLL:
            case UWBMAC_FRM_TYPE_TWR_FINAL:
            default:	//ignore unknown frame
            {
                return;
            }
        }
    }
    if(is_knownframe == 1) {
        putevent(&dw_event, rxd_event);
    } else {
        qDebug() << "rec unknown frame";
        //need to re-enable the rx (got unknown frame type)
        anch_handle_error_unknownframe_timeout(&dw_event);
    }
}


int InstanceAnch::app_run(instance_data_t *inst) {
    int instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
    //get any of the received events from ISR
    int message = peekevent();
    u8 ldebug = 0;

    switch (inst->testAppState)
    {
        case TA_INIT:
            if(ldebug)	qDebug() << "tx TA_INIT:" << inst->instanceAddress16;

            anchor_init(inst);
            instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
            break;

        case TA_TXBCN_WAIT_SEND:
            if(ldebug)	qDebug() << "TA_TXBCN_WAIT_SEND:" << inst->instanceAddress16;
            //qDebug() << "TA_TXBCN_WAIT_SEND:" << inst->instanceAddress16;
            send_beacon_msg(inst, inst->bcnmag.bcnFlag);
            instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
            break;

        case TA_DOWNLOAD_REQ_WAIT_SEND:
            if(ldebug)	qDebug() << "TA_DOWNLOAD_REQ_WAIT_SEND:" << inst->instanceAddress16;
            //instDone = send_download_req_msg(inst, 0);
            break;

        case TA_TX_WAIT_CONF:
        {
            event_data_t* dw_event = getevent(11); //get and clear this event
            //wait for TX done confirmation
            if(ldebug)	qDebug() << "TA_TX_WAIT_CONF:" << inst->instanceAddress16;

            if(dw_event->type != DWT_SIG_TX_DONE) {
                instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
                break;
            }

            instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
            inst->testAppState = TA_RXE_WAIT;
            if(inst->previousState == TA_TXBCN_WAIT_SEND) {
                //qDebug() << "txBea ok";
            } else if(inst->previousState == TA_TXPOLL_WAIT_SEND) {
                inst->twrmag.pollTxTime[0] = dw_event->timeStamp;
                qDebug() << "txPoll ok";
            } else if(inst->previousState == TA_TXFINAL_WAIT_SEND) {
                qDebug() << "txFinal ok";
            } else if(inst->previousState == TA_DOWNLOAD_REQ_WAIT_SEND) {
                qDebug() << "txDownReq ok";
            } else if(inst->previousState == TA_DOWNLOAD_RESP_WAIT_SEND) {
                qDebug() << "txDownResp ok";
            }
            break;
        }

        case TA_RXE_WAIT:
            if(ldebug)	qDebug() << "TA_RXE_WAIT:" << inst->instanceAddress16;
            //if this is set the RX will turn on automatically after TX

            if(inst->wait4ack == 0) {
//                if(dwt_read16bitoffsetreg(0x19, 1) != 0x0505) {
//                    //turn on RX
//                    dwt_rxenable(DWT_START_RX_IMMEDIATE);
//                }
            } else {
                inst->wait4ack = 0;
            }
            instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
            inst->testAppState = TA_RX_WAIT_DATA;
            if(message == 0)	break;

        case TA_RX_WAIT_DATA:
            switch(message) {
                //if we have received a DWT_SIG_RX_OKAY event - this means that the message is IEEE data type
                //need to check frame control to know which addressing mode is used
                case DWT_SIG_RX_OKAY:
                {
                    event_data_t *dw_event = getevent(15);
                    int msgid = 0;
                    uint16 srcAddress16 = dw_event->msgu.rxmsg_ss.sourceAddr[0] + (dw_event->msgu.rxmsg_ss.sourceAddr[1] << 8);
                    uint8 *messageData = NULL;

                    if(ldebug)	qDebug() << "DWT_SIG_RX_OKAY:" << inst->instanceAddress16;

                    msgid = dw_event->msgu.rxmsg_ss.messageData[MIDOF];
                    messageData = &(dw_event->msgu.rxmsg_ss.messageData[0]);

                    switch(msgid) {
                        case UWBMAC_FRM_TYPE_BCN:
                            qDebug() << inst->instanceAddress16 << "rec bcn" << srcAddress16;
                            process_beacon_msg(inst, dw_event, srcAddress16, messageData);
                            break;

                        case UWBMAC_FRM_TYPE_CL_JOIN:
                            process_join_request_msg(inst, dw_event, srcAddress16, messageData);
                            break;

                        case UWBMAC_FRM_TYPE_TWR_GRP_POLL:
                            if(dw_event->typePend == DWT_SIG_TX_PENDING) {
                                inst->testAppState = TA_TX_WAIT_CONF;
                                inst->previousState = TA_TXPOLL_WAIT_SEND;
                            }
                            if(messageData[GP_QFAOF]){
                                uint16 firstAdd = messageData[GP_ADDOF] + (messageData[GP_ADDOF + 1] << 8);
                                uint16 elecValue = messageData[GP_UPPOF] + (messageData[GP_UPPOF + 1] << 8);
                                if(firstAdd == inst->instanceAddress16) {
                                    if(inst->gatewayAnchor == 1) {
                                        //small order transform to big order
                                        //ByteOrder_Inverse_Int(&messageData[GP_COROF], NULL, 3);
                                        //addTagInfo(srcAddress16, &messageData[GP_COROF], messageData[GP_QFAOF], elecValue, messageData[GP_SQNOF]);
                                        if(ldebug)	qDebug() << "add gag info to service:" << inst->instanceAddress16;
                                    } else {
                                        addTagPos_ToBuf(srcAddress16, &messageData[GP_COROF], messageData[GP_QFAOF], elecValue, messageData[GP_SQNOF]);
                                    }
                                }
                            }
                            break;

                        case UWBMAC_FRM_TYPE_TWR_RESP:
                            if(dw_event->typePend == DWT_SIG_TX_PENDING) {
                                inst->testAppState = TA_TX_WAIT_CONF;
                                inst->previousState = TA_TXFINAL_WAIT_SEND;
                            }
                            break;

                        case UWBMAC_FRM_TYPE_DOWNLOAD_REQ:
                            //printf("[%d]Download type: %d, %d\n", srcAddress16, messageData[1],  *((uint16*)&messageData[2]));
                            qDebug() << srcAddress16 << " Download type: " << messageData[1] << (*((uint16*)&messageData[2]));

                            if(dw_event->typePend == DWT_SIG_TX_PENDING) {
                                inst->testAppState = TA_TX_WAIT_CONF;
                                inst->previousState = TA_DOWNLOAD_RESP_WAIT_SEND;
                            }
                            break;

                        case UWBMAC_FRM_TYPE_DOWNLOAD_RESP:
                            //anchor_process_download_data(inst, dw_event, srcAddress16, messageData);
                            break;

                        default:
                            break;
                    }
                }
                    break;
                default:
                    if(message) {
                        getevent(20);
                    }
                    if(instDone == INST_NOT_DONE_YET) {
                        instDone = INST_DONE_WAIT_FOR_NEXT_EVENT;
                    }
                    if(ldebug)	qDebug() << "no msg:" << inst->instanceAddress16;
                    break;
            }
            break;	//end case TA_RX_WAIT_DATA
        default:
            if(ldebug)	qDebug() << "no event" << inst->instanceAddress16;
            break;
    } // end switch on testAppState

    return instDone;
}



int InstanceAnch::run(void) {
    instance_data_t* inst = get_local_structure_ptr(0);
    int done = INST_NOT_DONE_YET;

    if(inst->instanceTimerEn) {
        if(SysTick_TimeIsOn_10us(inst->beaconTXTimeCnt)) {
            inst->testAppState = TA_TXBCN_WAIT_SEND;
            inst->beaconTXTimeCnt += inst->sfPeriod_ms * 100;
            //clearevents(); //clear any events
        }
    }

    while(done == INST_NOT_DONE_YET) {
        done = app_run(inst); // run the communications application
    }
    return 0;
}
