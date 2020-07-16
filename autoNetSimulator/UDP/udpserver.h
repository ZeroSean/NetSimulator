#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QList>
#include <QDebug>
#include <QDateTime>
#include <DisplayApplication.h>
#include <ViewSettings.h>
#include <QFile>
#include <QMap>

class QUdpSocket;

struct ClientState {
    ClientState() {
        isReponse_ForAreaUpdate = true;
    }
    ClientState(QHostAddress addr, quint16 p) {
        address = addr;
        port = p;
        isReponse_ForAreaUpdate = true;
    }
    QHostAddress address;
    quint16 port;
    quint16 isReponse_ForAreaUpdate;
};


class UDPServer : public QObject {
    Q_OBJECT

public:
    explicit UDPServer(QString ip, quint16 port = 8080);
    ~UDPServer();

    qint16 fillAnchorsPosition(const quint8 *recBuf, quint16 recSize, quint8 *sendBuf, quint16 maxSize);
    qint16 fillDangerAreaData(const quint8 *recBuf, quint16 recSize, quint8 *sendBuf, quint16 maxSize);

    qint16 getResponse(const quint8 *recBuf, quint16 recSize, quint8 *sendBuf, quint16 maxSize, QString clientKey="");
    int parseTagInfo(const quint8 *data, quint16 len);

    QString getServerIP(void);

public slots:
    void dgAreaConfigChanged();

private slots:
   void readUDPData(void);

signals:
    void recTagPosition(quint64 id, double x, double y, double z, quint8 warnState);
    void recTagPositionCircle(quint64 tagId, quint64 anchId, double dist);
    void recTagPositionLine(quint64 tagId, quint64 anchId0, quint64 anchId1, double dist);

private:
    QUdpSocket* _serversocket;
    quint16 _port;
    QString _serverIP;

    QMap<QString, ClientState> _clients;
};

#endif // UDPSERVER_H
