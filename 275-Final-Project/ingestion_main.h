#ifndef INGESTION_MAIN_H
#define INGESTION_MAIN_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

class IngestionClient : public QObject {
    Q_OBJECT

public:
    explicit IngestionClient(QObject* parent = nullptr);
    void start();
    void startListeningForMetadataClient(quint16 port);
    void sendQueryRequest(int param);

private slots:
    void onServerConnected();
    void onNewMetadataClientConnection();
    void sendRegistrationRequest();
    void onServerReadyRead();
    void onMetadataClientReadyRead();

private:
    QTcpSocket* serverSocket;
    QTcpSocket* metadataSocket;
    QTcpServer* metadataServer;
    QString metadataAnalyticsLeader;
    QString metadataIngestionLeader;
    QStringList nodeList;
    bool dataReady;

    void processDataAndSend(const QString& folderPath);
    void processBatch(QByteArray &batchData, QJsonArray &dataArray, int &rowIndex);
    void processNodeDiscovery(const QJsonObject& message);
    void sendDataToLeader();
    void sendToMetadataAnalyticsLeader(const QJsonDocument& doc);
};

#endif // INGESTION_MAIN_H
