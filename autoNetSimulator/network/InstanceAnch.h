#ifndef INSTANCEANCH_H
#define INSTANCEANCH_H

#include <QObject>

#include "InstanceCommon.h"
#include "Coordinator.h"


class Coordinator;


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
    explicit InstanceAnch(Coordinator *co);
    //virtual ~InstanceAnch() {}

    int app_run(instance_data_t *inst);

    virtual int run();
    virtual void rx_ok_cb(const event_data_t *cb_data);
    virtual void rx_to_cb(const event_data_t *cb_data);
    virtual void rx_err_cb(const event_data_t *cb_data);

    void addTagPos_ToBuf(uint16 addr, uint8* pos, u8 alarm, uint16 elecValue, u8 seqNum);
    uint8 cpy_to_PosBuf(uint8 num, uint8 *src);
    void clearTagPosBuf(void);

    void anch_no_timeout_rx_reenable(void);
    void anch_fill_msg_addr(instance_data_t* inst, uint16 dstaddr, uint8 msgid);
    void prepare_beacon_frame(instance_data_t* inst, uint8_t flag);
    uint8 prepare_join_request_frame(instance_data_t* inst);
    void BCNLog_Clear(instance_data_t* inst);

    uint8 process_beacon_msg(instance_data_t* inst, event_data_t *dw_event, uint16 srcAddr16, uint8 *messageData);
    uint8 process_join_request_msg(instance_data_t* inst, event_data_t *dw_event, uint16 srcAddr16, uint8 *messageData);
    void send_beacon_msg(instance_data_t* inst, uint8 flag);
    uint8 process_grp_poll_msg(uint16 srcAddr, uint8* recData, event_data_t *event);
    uint8 process_response_msg(uint16 srcAddr, uint8* recData, event_data_t *event);

    void anchor_init(instance_data_t* inst);
    void anch_handle_error_unknownframe_timeout(event_data_t *dw_event);

private:
    PosUWBBufStr posUWBBuf;

public:
    Coordinator *coor;
};

#endif // INSTANCEANCH_H
