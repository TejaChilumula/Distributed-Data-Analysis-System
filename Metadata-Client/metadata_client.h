#ifndef METADATA_CLIENT_H
#define METADATA_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QHash>

class MetadataClient : public QObject {
    Q_OBJECT

public:
    MetadataClient(QObject* parent = nullptr);
    void start();
    void sendRegistrationRequest();
    void participateInElection();

private slots:
    void onServerConnected();
    void onReadyRead();
    void onElectionTimeout();
    void onDisconnected();

private:
    void processNodeDiscovery(const QJsonObject& message);
    void processElectionMessage(const QJsonObject& message);
    void processLeaderAnnouncement(const QJsonObject& message);

    QTcpSocket* socket;
    QTimer* electionTimer;
    int currentNumber;
    QString ipAddress;
    bool isLeader;
    QString metadataAnalyticsLeader;
    QString metadataIngestionLeader;
    QStringList nodeList;
    QStringList folderNames = {
        "20200810", "20200814", "20200815", "20200816", "20200817", "20200818", "20200819",
        "20200820", "20200821", "20200822", "20200823", "20200824", "20200825", "20200826",
        "20200827", "20200828", "20200829", "20200830", "20200831", "20200901", "20200902",
        "20200903", "20200904", "20200905", "20200906", "20200907", "20200908", "20200909",
        "20200910", "20200911", "20200912", "20200913", "20200914", "20200915", "20200916",
        "20200917", "20200918", "20200919"
    };
};

#endif // METADATA_CLIENT_H
