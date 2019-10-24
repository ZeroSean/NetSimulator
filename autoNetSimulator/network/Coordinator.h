#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <QObject>
#include <QMap>
#include <QThread>
#include <QDateTime>
#include <time.h>
#include <sys/time.h>

#include "InstanceCommon.h"

class InstanceCommon;

void portGetTickCntInit();
uint32_t portGetTickCnt_10us();
uint32_t portGetTickCnt();
u8 SysTick_TimeIsOn_10us(uint32_t time_10us);

struct MsgListNode{
    MsgListNode(uint32_t l = 0, uint32_t t = 0, InstanceCommon *ins = NULL, MsgListNode *nex = NULL) {
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

class Coordinator  : public QThread {
public:
    Coordinator();

    void push(MsgListNode *node);
    MsgListNode* pop(void);
    int msgListSize();
    bool msgListEmpty();
    void msgListClear();

    int sendMsg(InstanceCommon *inst, uint8 *data, uint16 len, uint32 dely);
    void addAnchor(InstanceCommon *anc, uint32 id, double x, double y, double z, double range, uint8 mode);

    virtual void run();

private:
    MsgListNode msgListDummy;
    int listSize;

    QMap<uint64, InstanceCommon*> instances;
    QMap<uint64, QMap<uint64, InstanceCommon*> > inRangeInstances;

};

#endif // COORDINATOR_H
