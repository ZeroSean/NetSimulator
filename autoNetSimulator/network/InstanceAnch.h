#ifndef INSTANCEANCH_H
#define INSTANCEANCH_H

#include <QObject>

#include "InstanceCommon.h"

#define ONE_POS_INFO_SIZE		(18)
#define POS_UWB_BUF_MAX_SIZE	(4 * ONE_POS_INFO_SIZE)

typedef struct {
    u16 length;
    u8 num;
    //addr(2 bytes), xyz(12 bytes), alarm(1 byte), elecValue(2 bytes), seqNum(1 byte) = 18 bytes
    u8 data[POS_UWB_BUF_MAX_SIZE];
}PosUWBBufStr;



class InstanceAnch : public InstanceCommon {
    Q_OBJECT
public:
    InstanceAnch();

private:
    PosUWBBufStr posUWBBuf[2] = {0, 0};
};

#endif // INSTANCEANCH_H
