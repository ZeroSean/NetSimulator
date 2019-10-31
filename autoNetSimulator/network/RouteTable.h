#ifndef ROUTETABLE_H
#define ROUTETABLE_H

#include <QObject>

/**
 * LRU
 **/
struct RouteElement {
    RouteElement() {
        dest = 0;
        out = 0;
        depth = 0;
        isGateway = 0;
    }
    uint16_t dest;
    uint16_t out;
    uint8_t depth;
    uint8_t isGateway;
};

class RouteTable : public QObject {
    Q_OBJECT

public:
    RouteTable(int maxSize);
    ~RouteTable();

    int RouteSize(void);
    bool RouteEmpty(void);
    void update(uint16_t dest, uint16_t out, uint8_t depth, uint8_t isGateway);
    const RouteElement * cfind(uint16_t dest);
    const RouteElement * at(uint16_t index);
    int getOutPort(uint16_t dest);

    const RouteElement * getNextElement();

    const RouteElement * findShortestGateway();
    uint8_t  getGatewayDepth();
    uint16_t getGatewayAddr();
    uint16_t getGatewayOut();

    QString toString(int dest = -1);

private:
    RouteElement * find(uint16_t dest);

private:
    int _maxSize;
    int _size;
    RouteElement *_table;

    RouteElement _uploadGateway;

    uint16_t _next;
};

#endif // ROUTETABLE_H
