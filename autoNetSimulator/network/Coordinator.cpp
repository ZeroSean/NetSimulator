#include "Coordinator.h"

struct timeval startTime;

void portGetTickCntInit() {
    gettimeofday(&startTime, NULL);
}

uint32_t portGetTickCnt_10us() {
    struct timeval tvTime;
    uint32_t usec10 = 0;

    gettimeofday(&tvTime, NULL);

    return tvTime.tv_usec / 10;

    usec10 = 100000 * (tvTime.tv_sec - startTime.tv_sec) + (tvTime.tv_usec - startTime.tv_usec);

    return usec10;
}

uint32_t portGetTickCnt() {
    return (portGetTickCnt_10us() / 100);
}

Coordinator::Coordinator() :
    msgListDummy(),
    listSize(0)
{

}

void Coordinator::run() {
    while(true) {
        for(InstanceCommon* instance : instances) {
            instance->run();    //运行该节点
        }
        if(!msgListEmpty()) {
            MsgListNode *msg = pop();
            uint64 srcID = msg->instance->address();

            //循环运行每个节点的接收成功中断函数和发送节点的发送成功函数
            for(InstanceCommon* instance : inRangeInstances[srcID]) {
                instance->rx_ok_cb(&msg->data);
            }
            msg->instance->tx_conf_cb(&msg->data);
        }
    }
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
