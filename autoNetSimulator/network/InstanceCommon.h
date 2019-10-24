#ifndef INSTANCECOMMON_H
#define INSTANCECOMMON_H

#include <QObject>

#include "instance.h"
#include "Coordinator.h"

class InstanceCommon : public QObject {
    Q_OBJECT

public:
    explicit InstanceCommon();
    //virtual ~InstanceCommon() = 0;

    void set_position(double *pos, u8 flag);
    instance_data_t* get_local_structure_ptr(unsigned int x);
    uint64 convert_usec_to_devtimeu(double microsecu);
    int get_role(void);
    int newrange(void);
    int newrangeancadd(void);
    int newrangetagadd(void);
    int newrangetim(void);
    void clearcounts(void);

    int init(int inst_mode);

    uint32 readdeviceid(void);
    void instance_config(instanceConfig_t *config, sfConfig_t *sfConfig);
    int get_rnum(void);
    int get_rnuma(int idx);
    int get_rnumanc(int idx);
    int get_lcount(void);
    void set_16bit_address(uint16 address);
    uint16 address();

    void config_frameheader_16bit(instance_data_t *inst);
    //延时发送实现
    int send_delayed_frame(instance_data_t *inst, int delayedTx);

    void seteventtime(event_data_t *dw_event, uint8* timeStamp);
    int peekevent(void);
    void putevent(event_data_t *newevent, uint8 etype);
    event_data_t* getevent(int x);
    void clearevents(void);
    void config_txpower(uint32 txpower);
    void set_txpower(void);
    void config_antennadelays(uint16 tx, uint16 rx);
    void set_antennadelays(void);
    uint16 get_txantdly(void);
    uint16 get_rxantdly(void);
    uint8 validranges(void);
    float calc_length_data(float msgdatalen);
    void set_replydelay(int delayus);

    virtual int run() {return 0;}
    virtual void rx_ok_cb(const event_data_t *cb_data) {Q_UNUSED(cb_data)}
    virtual void rx_to_cb(const event_data_t *cb_data) {Q_UNUSED(cb_data)}
    virtual void rx_err_cb(const event_data_t *cb_data) {Q_UNUSED(cb_data)}
    void tx_conf_cb(const event_data_t *cb_data);

    void setPos(double x, double y, double z);
    void setRange(double r);
    const double* getPos(void);
    double getRange();

private:
    instance_data_t instance_data[2];

    event_data_t dw_event_g;


    double pos[3];
    double range;
};

#endif // INSTANCECOMMON_H
