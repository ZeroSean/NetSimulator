#include "Coordinator.h"

#include <QDebug>

struct timeval startTime;

instanceConfig_t chConfig[4] ={
                    //mode 1 - S1: 2 off, 3 off
                    {
                        4,//2,            	// channel
                        20,//4,             // preambleCode(20-80m, 17-40m, 9-24m,4-11m )
                        DWT_PRF_64M,    	// prf		(DWT_PRF_16M)
                        DWT_BR_110K,        // datarate
                        DWT_PLEN_1024,   	// preambleLength
                        DWT_PAC32,          // pacSize
                        1,                  // non-standard SFD
                        (1025 + 64 - 32)    // SFD timeout
                    },
                    //mode 2 - S1: 2 on, 3 off
                    {
                        4,//2,            	// channel
                        20,//4,             // preambleCode
                        DWT_PRF_64M,		//DWT_PRF_16M,   		// prf
                        DWT_BR_6M8,        	// datarate
                        DWT_PLEN_128,   	// preambleLength
                        DWT_PAC8,           // pacSize
                        1,//0,                  // non-standard SFD
                        (129 + 8 - 8)       // SFD timeout
                    },
                    //mode 3 - S1: 2 off, 3 on
                    {
                        5,             		// channel
                        3,              	// preambleCode
                        DWT_PRF_16M,    	// prf
                        DWT_BR_110K,        // datarate
                        DWT_PLEN_1024,   	// preambleLength
                        DWT_PAC32,          // pacSize
                        1,                  // non-standard SFD
                        (1025 + 64 - 32)    // SFD timeout
                    },
                    //mode 4 - S1: 2 on, 3 on
                    {
                        5,            		// channel
                        3,             		// preambleCode
                        DWT_PRF_16M,   		// prf
                        DWT_BR_6M8,        	// datarate
                        DWT_PLEN_128,   	// preambleLength
                        DWT_PAC8,           // pacSize
                        0,                  // non-standard SFD
                        (129 + 8 - 8)       // SFD timeout
                    }
};

//Slot and Superframe Configuration
sfConfig_t sfConfig[4] ={
/*
    uint16 BCNslotDuration_ms;
    uint16 BCNslot_num;
    uint16 SVCslotDuration_ms;
    uint16 SVCslot_num;
    uint16 twrSlotDuration_ms;
    uint16 twrSlots_num;
    uint16 sfPeriod_ms;
    uint16 grpPollTx2RespTxDly_us;
*/
    //{10, 16, 20, 2, 50, 15, 1000, 20000},	//110k, channel 2
    {4, 16, 4, 2, 35, 15, 600, 20000},	//110k, channel 2
    {3, 16, 3, 2, 10, 15, 204, 5000},	//6.81M, channel 2
    {10, 16, 20, 2, 50, 15, 1000, 20000},	//110k, channel 5
    {1,  16, 2,  2, 5,  15, 100,  2000}	//6.81M, channel 5
};


void portGetTickCntInit() {
    gettimeofday(&startTime, NULL);
}

uint32_t portGetTickCnt_10us() {
    struct timeval tvTime;
    uint64_t usec10 = 0;

    gettimeofday(&tvTime, NULL);

    usec10 = 100000 * (tvTime.tv_sec - startTime.tv_sec) + (tvTime.tv_usec - startTime.tv_usec) / 10;

    return usec10;
}

uint32_t portGetTickCnt() {
    return (portGetTickCnt_10us() / 100);
}

u8 SysTick_TimeIsOn_10us(uint32_t time_10us) {
    uint32_t tickstart = portGetTickCnt_10us();
    if(tickstart >= time_10us) {
        return 1;
    } else {
        uint32_t delta = time_10us - tickstart;
        if(delta >= 0x7fffffff) {
            return 1;
        }
    }
    return 0;
}

Coordinator::Coordinator() :
    msgListDummy(0, 0, NULL, NULL)
{
    listSize = 0;
    portGetTickCntInit();
}

double distance(const double pos[3], const double pos2[3]) {
    double dpow2 = (pos[0] - pos2[0]) * (pos[0] - pos2[0])
            + (pos[1] - pos2[1]) * (pos[1] - pos2[1])
            + (pos[2] - pos2[2]) * (pos[2] - pos2[2]);
    return sqrt(dpow2);
}

void Coordinator::addAnchor(InstanceCommon *anc, uint32 id, double x, double y, double z, double range, uint8 mode) {
    anc->init(ANCHOR);

    anc->set_16bit_address(id);
    qDebug() << "Address:" << id;

    mode = 1;

    anc->setPos(x, y, z);
    anc->setRange(range);

    anc->instance_config(&chConfig[mode], &sfConfig[mode]);

    for(InstanceCommon *ins : instances) {
        double dis = distance(ins->getPos(), anc->getPos());
        if(dis <= anc->getRange()) {
            inRangeInstances[anc->address()].insert(ins->address(), ins);
        }
        if(dis <= ins->getRange()) {
            inRangeInstances[ins->address()].insert(anc->address(), anc);
        }
    }

    instances.insert(id, anc);
}

void Coordinator::run() {
    while(true) {
        for(InstanceCommon* instance : instances) {
            instance->run();    //运行该节点
            //qDebug() << "run instance:" << instance->address();
        }
        if(!msgListEmpty()) {
            MsgListNode *msg = pop();
            uint64 srcID = msg->instance->address();

            qDebug() << "msg instance:" << msg->instance->address();

            //循环运行每个节点的接收成功中断函数和发送节点的发送成功函数
            for(InstanceCommon* instance : inRangeInstances[srcID]) {
                instance->rx_ok_cb(&msg->data);
            }
            msg->instance->tx_conf_cb(&msg->data);

            delete msg;
        }

        //qDebug() << "utime:" << portGetTickCnt_10us();
    }
}

int Coordinator::sendMsg(InstanceCommon *inst, uint8 *data, uint16 len, uint32 dely) {
    MsgListNode *msg = new MsgListNode();

    if(data == NULL) {
        return -1;
    }

    msg->len = len;
    msg->instance = inst;
    memcpy(&msg->data.msgu, data, len);

    msg->data.rxLength = len;

    msg->time = dely;
    msg->next = NULL;

    push(msg);

    return 0;
}

void Coordinator::push(MsgListNode *node) {
    MsgListNode *pre = &msgListDummy;
    MsgListNode *next = msgListDummy.next;

    if(node == NULL) {
        return;
    }

    while(next != NULL) {
        if(next->time > node->time) {
            break;
        }
        pre = next;
        next = next->next;
    }
    pre->next = node;
    node->next = next;
    listSize++;
}

MsgListNode* Coordinator::pop() {
    MsgListNode *next = msgListDummy.next;
    if(next == NULL) {
        return NULL;
    }

    msgListDummy.next = next->next;
    next->next = NULL;
    listSize--;

    return next;
}

int Coordinator::msgListSize() {
    return listSize;
}

bool Coordinator::msgListEmpty() {
    return (listSize == 0);
}

void Coordinator::msgListClear() {

}
