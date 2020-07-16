#ifndef LOG_H
#define LOG_H

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>


//默认调试级别为debug，如果需要修改级别，需要在启动时输入对应级别号码
//1:warnging
//2:critical
//3:fatal
//只有release版本时，才会讲终端信息输出到日志中，debug版本正常输出到终端

namespace QT_LOG
{
    //日志文件名以启动时间命令
    static int m_LogLevel = 0;
    static QString m_logFile = QString("%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    QMutex m_LogMutex;

    void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        if(type < m_LogLevel) {
            //小于设置级别的日志不会写到文件中
            return;
        }

        QString log_info;
        switch(type) {
            case QtDebugMsg:
                log_info = QString("%1: %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), msg);
                break;
            case QtWarningMsg:
                log_info = QString("%1[Warning]: %2  (file: %3, line: %4)").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                                                                             msg, QString(context.file), QString::number(context.line));
            break;
            case QtCriticalMsg:
                log_info = QString("%1[Critical]: %2  (file: %3, line: %4)").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                                                                              msg, QString(context.file), QString::number(context.line));
                break;
            case QtFatalMsg:
                log_info = QString("%1[Fatal]: %2  (file: %3, line: %4)").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                                                                           msg, QString(context.file), QString::number(context.line));
                abort();
            default:
                break;
        }

        m_LogMutex.lock(); //线程安全

        QFile outFile(m_logFile);
        outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        QTextStream ts(&outFile);
        ts << log_info << endl;
        outFile.close();

        m_LogMutex.unlock();
    }

    void logInit(QString logFile = "", int logLevel = 0) {
        //实现debug版本输出到终端，release输出到日志文件
#ifndef QT_DEBUG
        if(logLevel < 0 || logLevel > 3) {
            m_LogLevel = 0;
        } else {
            m_LogLevel = logLevel;
        }

        if(!logFile.isEmpty()) {
            m_logFile = "./Logs/" + logFile;
        } else {
            m_logFile = "./Logs/" + m_logFile;
        }

        qInstallMessageHandler(customMessageHandler);
#endif
    }

};

#endif // LOG_H
