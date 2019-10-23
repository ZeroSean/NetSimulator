#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <QObject>
#include <QMap>
#include <QDateTime>
#include <time.h>
#include <sys/time.h>

#include "InstanceCommon.h"

void portGetTickCntInit();
uint32_t portGetTickCnt_10us();
uint32_t portGetTickCnt();

struct MsgListNode {
    ListNode(uint32_t l = 0, uint32_t t = 0, InstanceCommon *ins = NULL, MsgListNode *nex = NULL) {
        len = l;
        time = t;
        instance = ins;
        next = nex;
    }
    uint32_t len;
    uint32_t time;
    InstanceCommon *instance;
    MsgListNode *next;
    event_data_t data;
};

class Coordinator
{
public:
    Coordinator();

    void push(MsgListNode *node);
    MsgListNode* pop(void);
    int msgListSize();
    bool msgListEmpty();
    void msgListClear();

    void run();

private:
    MsgListNode msgListDummy;
    int listSize;

    QMap<uint64, InstanceCommon*> instances;
    QMap<uint64, QMap<uint64, InstanceCommon*> > inRangeInstances;

};

#endif // COORDINATOR_H
