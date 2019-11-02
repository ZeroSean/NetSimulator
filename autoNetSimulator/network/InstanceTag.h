#ifndef INSTANCETAG_H
#define INSTANCETAG_H

#include <QObject>

#include "InstanceCommon.h"
#include "Coordinator.h"


class InstanceTag : public InstanceCommon {

public:
    explicit InstanceTag(Coordinator *co);

    int app_run(instance_data_t *inst);

    virtual int run();
    virtual void rx_ok_cb(const event_data_t *cb_data);
    virtual void rx_to_cb(const event_data_t *cb_data);
    virtual void rx_err_cb(const event_data_t *cb_data);

    uint8 findFirstZeroBit(uint16 bitMap, uint8 num);
    void fill_msg_addr(instance_data_t* inst, uint16 dstaddr, uint8 msgid);
    void enable_rx(uint32 dlyTime, int fwto_sy);
    uint8 setPollRxStamp(instance_data_t *inst, uint8 idx, uint8 error);
    uint8 setFinalRxStamp(instance_data_t *inst, uint8 idx, uint8 error);
    void change_state_to_listen(instance_data_t *inst);
    void process_rx_event(instance_data_t *inst, uint8 eventTypePend);
    void handle_error_unknownframe(event_data_t *dw_event);
    uint8 process_beacon_msg(instance_data_t* inst, event_data_t *dw_event, uint16 srcAddr, uint8 *messageData);
    uint8 sent_grppoll_msg(instance_data_t* inst);
    uint8 send_response_msg(instance_data_t* inst);
    uint8 process_poll_msg(uint16 sourceAddress, uint8* recData, event_data_t *event);
    uint8 process_final_msg(uint16 sourceAddress, uint8* recData, event_data_t *event);
    void tag_init(instance_data_t* inst);

    void clearBCNLog();

public:
    Coordinator *coor;

    uint8 depthToGateway;
    uint16 uploadPort;

};

#endif // INSTANCETAG_H
