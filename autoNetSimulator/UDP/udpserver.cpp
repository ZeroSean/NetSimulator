#include "UDPServer.h"
#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QList>
#include <QDebug>
#include <QDateTime>
#include <DisplayApplication.h>
#include <ViewSettings.h>
#include <QFile>

#define STAT_CODE   (0x68)
#define MSG_MIN_LEN (1 + 6 + 1 + 2 + 1 + 1)
#define CTRL_OFFSET (1 + 6)
#define SIZE_OFFSET (1 + 6 + 1)
#define DATA_OFFSET (1 + 6 + 1 + 2)

#define POWERLINK   (0x00)
#define SETCLOCK    (0x01)
#define HEART       (0x05)
#define TAGPOSITION (0xda)
#define ANCPOSITION (0xc3)

#define TAGINFOSIZE (19)    // 6 + 4 + 4 + 4 +1

QString byteArrayToString(const char *data, quint16 len) {
    QString msg = "";

    for(int i = 0; i < len; i++) {
        msg += " " + QString::number((quint8)data[i], 16);
    }

    return msg;
}

quint8 checkSumCode(const char *data, quint16 len) {
    quint8 sum = 0;
    for(int i = 0; i < len; ++i) {
        sum += data[i];
    }
    return sum;
}

qint16 fillData(char *sendBuf, quint8 ctrl, const char *recBuf, char *data=NULL, quint16 len=0) {
    if(data == NULL) {
        len = 0;
    }

    memcpy((void *)sendBuf, (void *)recBuf, CTRL_OFFSET);

    sendBuf[CTRL_OFFSET] = ctrl;
    sendBuf[SIZE_OFFSET] = (len >> 8) & 0xff;
    sendBuf[SIZE_OFFSET+1] = len & 0xff;

    if(data != NULL) {
        for(int i = 0; i < len; i++) {
            sendBuf[DATA_OFFSET+i] = data[i];
        }
    }

    quint16 size = DATA_OFFSET + len;
    sendBuf[size + 0] = checkSumCode(sendBuf + CTRL_OFFSET, size - CTRL_OFFSET);
    sendBuf[size + 1] = 0x16;

    return (size + 2);
}


UDPServer::UDPServer(quint16 port)
{
    _port = port;

    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    for(int i = 0; i < ipAddressesList.size(); ++i) {
        if(ipAddressesList.at(i) != QHostAddress::LocalHost &&
           ipAddressesList.at(i).toIPv4Address()) {
            _serverIP = ipAddressesList.at(i).toString();
        }
    }
    if(_serverIP.isNull() || _serverIP.isEmpty()) {
        _serverIP = QHostAddress(QHostAddress::LocalHost).toString();
    }

    qDebug() << "Starting udp server:" << _serverIP << "  port:" << _port;

     _serversocket = new QUdpSocket(this);
    //取消绑定端口号使用close
    _serversocket->bind(QHostAddress(_serverIP), _port);

    QObject::connect(_serversocket, SIGNAL(readyRead()), this, SLOT(readUDPData()));
}

UDPServer::~UDPServer() {
    qDebug() << "Close udp server:" << _serverIP << "  port:" << _port;
    delete _serversocket;
}

QString UDPServer::getServerIP(void) {
    return _serverIP;
}


void UDPServer::readUDPData() {
    char recBuf[1024] = {0};
    char sendBuf[1024] = {0};
    QHostAddress dstAddr;
    quint16 dstPort = 0;

    while(_serversocket->hasPendingDatagrams()) {
        qint16 len = _serversocket->readDatagram(recBuf, sizeof(recBuf), &dstAddr, &dstPort);

        qDebug() << "rec datagram: " << byteArrayToString(recBuf, len) << "src IP:" << dstAddr.toString();
        //qDebug() << dstAddr.toString();

        qint16 sendSize = getResponse(recBuf, len, sendBuf, 1024);

        if(sendSize > 0) {
            //qDebug() << "send datagram: " << byteArrayToString(sendBuf, sendSize);
            _serversocket->writeDatagram(sendBuf, sendSize, dstAddr, dstPort);

            if(recBuf[CTRL_OFFSET] == SETCLOCK) {
                sendSize = fillAnchorsPosition(recBuf, len, sendBuf, 1024);
                if(sendSize > 0) {
                    _serversocket->writeDatagram(recBuf, sendSize, dstAddr, dstPort);
                }
            }
        }
    }
}

int UDPServer::parseTagInfo(const char *data, quint16 len) {
    if(len >= TAGINFOSIZE) {
        quint16 id = (data[4] << 8) + data[5];
        quint8 warnState = data[18] & 0x03;
        quint8 locationState = (data[18] >> 4) & 0x0f;

        if(warnState) {
            qDebug() << "tag[" << id << "] wanring state: " << warnState;
        }

        if(locationState >= 3) {
            qint32 qx = 0, qy = 0, qz = 0;
            qx = (data[6] << 24) + (data[7] << 16) + (data[8] << 8) + data[9];
            qy = (data[10] << 24) + (data[11] << 16) + (data[12] << 8) + data[13];
            qz = (data[14] << 24) + (data[15] << 16) + (data[16] << 8) + data[17];

            qDebug() << "Tag " << id << " position: " << qx << " " << qy << " " << qz;

            emit recTagPosition(id, qx / 1000.0, qy / 1000.0, qz / 1000.0);

        } else {
            qDebug() << "tag[" << id << "] only receive " << locationState << "anchors.";
        }
        return 0;

    } else {
        qDebug() << "error: tag info size less than " << TAGINFOSIZE;
        return 1;
    }
}


qint16 UDPServer::getResponse(const char *recBuf, quint16 recSize, char *sendBuf, quint16 maxSize) {
    quint8 error = 0;
    quint8 checkcode = 0;
    quint16 dataSize = 0;
    qint16 sendSize = 0;

    if(recSize < MSG_MIN_LEN) {
        error = 1;
        qDebug() << "error: msg size less than " << MSG_MIN_LEN;
    }

    if(recBuf[0] != STAT_CODE) {
        error = 1;
        qDebug() << "error msg start code -> " << recBuf[0];
    }

    checkcode = checkSumCode(recBuf + CTRL_OFFSET, recSize - CTRL_OFFSET - 2);
    if(checkcode != (quint8)recBuf[recSize - 2]) {
        error = 1;
        qDebug() << "error msg checkcode -> " << checkcode;
    }

    dataSize = (recBuf[SIZE_OFFSET] << 8) + recBuf[SIZE_OFFSET + 1];
    if(dataSize + MSG_MIN_LEN != recSize) {
        error = 1;
        qDebug() << "error msg size -> " << dataSize;
    }

    if(error) {
        qDebug() << "error datagram: " <<byteArrayToString(recBuf, recSize);
        return 0;
    }

    switch((quint8)recBuf[CTRL_OFFSET]) {
        case POWERLINK:
        {
            QString version = "V" + QString::number(recBuf[DATA_OFFSET]);
            version += "." + QString::number(recBuf[DATA_OFFSET+1]);
            qDebug() << "Anchor Version: " << version;

            sendSize = fillData(sendBuf, POWERLINK, recBuf);
            break;
        }

        case SETCLOCK:
        {
            qDebug() << "Set Anchor Clock: ";
            QDateTime curDateTime = QDateTime::currentDateTime();
            QDate curDate = curDateTime.date();
            QTime curTime = curDateTime.time();
            char clock[6] = {0};

            clock[0] = curDate.year() - 2000;
            clock[1] = curDate.month();
            clock[2] = curDate.day();
            clock[3] = curTime.hour();
            clock[4] = curTime.minute();
            clock[5] = curTime.second();

            sendSize = fillData(sendBuf, SETCLOCK, recBuf, clock, 6);
            break;
        }

        case TAGPOSITION:
        {
            QDateTime curDateTime;
            QDate curDate;
            QTime curTime;

            curDate.setDate(recBuf[DATA_OFFSET] + 2000, recBuf[DATA_OFFSET+1], recBuf[DATA_OFFSET+2]);
            curTime.setHMS(recBuf[DATA_OFFSET+3], recBuf[DATA_OFFSET+4], recBuf[DATA_OFFSET+5]);
            curDateTime.setDate(curDate);
            curDateTime.setTime(curTime);

            quint16 expectSize = recBuf[DATA_OFFSET+6] * TAGINFOSIZE + 7;
            if( expectSize != dataSize) {
                qDebug() << curDateTime.toString("yyyy-MM-dd hh:mm:ss") <<"Rec tag info, but data format invalid.";
                break;
            }

            quint16 offset = DATA_OFFSET + 6 + 1;
            for(int i = 0; i < recBuf[DATA_OFFSET+6]; ++i) {
                parseTagInfo(recBuf + offset, recSize - offset - 2);
                offset += TAGINFOSIZE;
            }

            char d[2] = {(char)0xaa, (char)0x55};
            sendSize = fillData(sendBuf, SETCLOCK, recBuf, d, 2);

            break;
        }

        case ANCPOSITION:
        {
            if(recBuf[DATA_OFFSET]) {
                qDebug() << "Gateway success to receive anchors position.";
            } else {
                qDebug() << "Gateway fail to receive anchors position.";

                sendSize = fillAnchorsPosition(recBuf, recSize, sendBuf, maxSize);
            }
            break;
        }

        default:
        {
            qDebug() << "Unknow ctrl code: " << recBuf[CTRL_OFFSET];
            qDebug() << "error datagram: " <<byteArrayToString(recBuf, recSize);
            break;
        }
    }

    return sendSize;
}

qint16 UDPServer::fillAnchorsPosition(const char *recBuf, quint16 recSize, char *sendBuf, quint16 maxSize) {
    Q_UNUSED(recSize);

    QString path = DisplayApplication::viewSettings()->getAncConfigFilePath();

    if(path.isEmpty() || path.isNull()) {
        return 0;
    }

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug(qPrintable(QString("Error: cannot read file (%1) %2").arg(path).arg(file.errorString())));
        return 0;
    } else {
        QTextStream stream(&file);
        QString line = "";
        char ancbuf[1024] = {0};
        quint8 ancnum = 0;
        quint16 off = 1;

        while(!(line = stream.readLine()).isNull()) {

            std::string cLine = line.toStdString();
            int id;
            double x, y, z;
            qint32 qx = 0, qy = 0, qz = 0;
            int gateway = 0;

            sscanf(cLine.c_str(), "%d:(%lf, %lf, %lf):%d", &id, &x, &y, &z, &gateway);
            qDebug() << "anchor: "<< id << x << y << z << gateway;

            qx = (x * 1000) / 1;
            qy = (y * 1000) / 1;
            qz = (z * 1000) / 1;

            ancbuf[off+4] = (id >> 8) & 0xff;
            ancbuf[off+5] = id & 0xff;
            for(int i = 3; i >= 0; --i) {
                ancbuf[off+6+i] = qx & 0xff;
                ancbuf[off+10+i] = qy & 0xff;
                ancbuf[off+14+i] = qz & 0xff;
                qx >>= 8;
                qy >>= 8;
                qz >>= 8;
            }

            ancnum += 1;
            off += 18;

            if(off >= (maxSize - MSG_MIN_LEN)) {
                qDebug() << "The number of Anchors too much: " << ancnum;
                break;
            }
        }

        ancbuf[0] = ancnum;
        return fillData(sendBuf, ANCPOSITION, recBuf, ancbuf, off);
    }

    return 0;
}
