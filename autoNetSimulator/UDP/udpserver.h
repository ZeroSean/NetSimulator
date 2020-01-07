#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <QObject>

class QUdpSocket;


class UDPServer : public QObject {
    Q_OBJECT

public:
    explicit UDPServer(quint16 port = 8080);
    ~UDPServer();

    qint16 fillAnchorsPosition(const char *recBuf, quint16 recSize, char *sendBuf, quint16 maxSize);
    qint16 getResponse(const char *recBuf, quint16 recSize, char *sendBuf, quint16 maxSize);
    int parseTagInfo(const char *data, quint16 len);

    QString getServerIP(void);

private slots:
   void readUDPData(void);

signals:
    void recTagPosition(quint64 id, double x, double y, double z);

private:
    QUdpSocket* _serversocket;
    quint16 _port;
    QString _serverIP;
};

#endif // UDPSERVER_H
