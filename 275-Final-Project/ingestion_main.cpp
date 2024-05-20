#include "ingestion_main.h"
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QTextStream>
#include <omp.h>

IngestionClient::IngestionClient(QObject* parent)
    : QObject(parent),
    serverSocket(new QTcpSocket(this)),
    metadataSocket(nullptr),
    metadataServer(new QTcpServer(this)),
    dataReady(false) {  // Initialize metadataServer and dataReady flag
    connect(serverSocket, &QTcpSocket::connected, this, &IngestionClient::onServerConnected);
    connect(serverSocket, &QTcpSocket::readyRead, this, &IngestionClient::onServerReadyRead);
    connect(metadataServer, &QTcpServer::newConnection, this, &IngestionClient::onNewMetadataClientConnection);
}

void IngestionClient::start() {
    serverSocket->connectToHost("192.168.1.102", 12351);  // Update IP and port to the central server
}

void IngestionClient::startListeningForMetadataClient(quint16 port) {
    if (!metadataServer->listen(QHostAddress::Any, port)) {
        qDebug() << "Failed to start metadata server:" << metadataServer->errorString();
    } else {
        qDebug() << "Metadata server started on port" << port;
    }
}

void IngestionClient::onServerConnected() {
    sendRegistrationRequest();
}

void IngestionClient::onNewMetadataClientConnection() {
    metadataSocket = metadataServer->nextPendingConnection();
    connect(metadataSocket, &QTcpSocket::readyRead, this, &IngestionClient::onMetadataClientReadyRead);
    qDebug() << "Metadata Client connected.";
}

void IngestionClient::sendRegistrationRequest() {
    QJsonObject request;
    request["requestType"] = "registering";
    request["Ip"] = serverSocket->localAddress().toString();
    request["nodeType"] = "ingestion";
    request["computingCapacity"] = 0.8;  // Example value, adjust as needed

    QJsonDocument doc(request);
    serverSocket->write(doc.toJson(QJsonDocument::Indented));
    qDebug() << "Sent ingestion registration request:\n" << doc.toJson(QJsonDocument::Indented);
}

void IngestionClient::onServerReadyRead() {
    QByteArray responseData = serverSocket->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    QJsonObject responseObj = responseDoc.object();
    qDebug() << "Received from server:\n" << responseDoc.toJson(QJsonDocument::Indented);

    if (responseObj.contains("requestType") && responseObj["requestType"].toString() == "Node Discovery") {
        processNodeDiscovery(responseObj);
    }
}

void IngestionClient::onMetadataClientReadyRead() {
    QByteArray responseData = metadataSocket->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    QJsonObject responseObj = responseDoc.object();
    qDebug() << "Received from Metadata Client:\n" << responseDoc.toJson(QJsonDocument::Indented);

    if (responseObj.contains("command") && responseObj["command"].toString() == "load_data") {
        QString folderPath = responseObj["folderPath"].toString();
        processDataAndSend(folderPath);
    }
}


void IngestionClient::processDataAndSend(const QString& folderName) {
    QString basePath = "D:/SJSU - Total Coursework/2 - Sem/CMPE-275/Project/275-Final-Project/data/";  // Base path for the data folder

    QString folderPath = basePath + folderName;  // Construct the full path

    QDir dir(folderPath);
    QStringList files = dir.entryList(QStringList() << "*.csv", QDir::Files);

    QJsonArray dataArray;
    int rowIndex = 0;
    bool folderOpenedSuccessfully = false;

    #pragma omp parallel for
    for (int i = 0; i < files.size(); ++i) {
        QString filePath = folderPath + "/" + files[i];
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            #pragma omp critical
            {
                qDebug() << "Failed to open file:" << filePath;
            }
            continue;
        }

        folderOpenedSuccessfully = true;
        int batchSize = 1000;  // Process in batches of 1000 lines
        QByteArray batchData;
        int batchCount = 0;

        while (!file.atEnd()) {
            batchData.append(file.readLine().trimmed() + "\n");
            batchCount++;

            if (batchCount == batchSize || file.atEnd()) {
                #pragma omp critical
                {
                    processBatch(batchData, dataArray, rowIndex);
                }
                batchData.clear();
                batchCount = 0;
            }
        }
    }

    if (folderOpenedSuccessfully && !dataArray.isEmpty()) {
        dataReady = true;  // Set data ready flag to true
    }

    if (dataReady && !metadataAnalyticsLeader.isEmpty()) {
        QJsonObject request;
        request["requestType"] = "ingestion";
        request["data"] = dataArray;

        QJsonDocument doc(request);
        sendToMetadataAnalyticsLeader(doc);  // Send data to the metadata analytics leader
        qDebug() << "Sent processed data to leader:\n" << doc.toJson(QJsonDocument::Indented);
    }
}

void IngestionClient::processBatch(QByteArray &batchData, QJsonArray &dataArray, int &rowIndex) {
    QTextStream stream(&batchData);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList values = line.split(',');

        // Preprocessing and cleaning
        values.removeAll("-999");
        values.removeAll("");

        // Adding ID
        if (!values.isEmpty()) {
            values.prepend(values[2] + "@" + QString::number(rowIndex));
            rowIndex++;
        }

        QJsonArray jsonArray;
        for (const QString& value : values) {
            jsonArray.append(value);
        }
        dataArray.append(jsonArray);
    }
}


void IngestionClient::sendQueryRequest(int param) {
    QJsonObject queryRequest;
    queryRequest["requestType"] = "query";
    queryRequest["param"] = QJsonArray::fromVariantList(QVariantList() << param);

    QJsonDocument doc(queryRequest);
    sendToMetadataAnalyticsLeader(doc);  // Send query to the metadata analytics leader
    qDebug() << "Sent query request to leader:\n" << doc.toJson(QJsonDocument::Indented);
}

void IngestionClient::processNodeDiscovery(const QJsonObject& message) {
    if (message.contains("nodes") && message["nodes"].isArray()) {
        nodeList.clear();
        QJsonArray nodes = message["nodes"].toArray();
        for (const QJsonValue &node : nodes) {
            if (node.isObject()) {
                QJsonObject nodeObj = node.toObject();
                QString nodeInfo = QString("Type: %1, IP: %2, Capacity: %3")
                                       .arg(nodeObj["nodeType"].toString())
                                       .arg(nodeObj["IP"].toString())
                                       .arg(nodeObj["computingCapacity"].toDouble());
                nodeList.append(nodeInfo);
            }
        }
    }

    if (message.contains("metadataAnalyticsLeader")) {
        metadataAnalyticsLeader = message["metadataAnalyticsLeader"].toString();
    }

    if (message.contains("metadataIngestionLeader")) {
        metadataIngestionLeader = message["metadataIngestionLeader"].toString();
    }

    qDebug() << "Node Discovery Processed:";
    qDebug() << "Nodes:" << nodeList;
    qDebug() << "Metadata Analytics Leader:" << metadataAnalyticsLeader;
    qDebug() << "Metadata Ingestion Leader:" << metadataIngestionLeader;
}

void IngestionClient::sendDataToLeader() {
    if (!metadataAnalyticsLeader.isEmpty() && dataReady) {
        QJsonObject request;
        request["requestType"] = "send_data_to_leader";
        request["data"] = "Your data payload here";  // Adjust as needed

        QJsonDocument doc(request);
        sendToMetadataAnalyticsLeader(doc);  // Connect to the leader's IP
        qDebug() << "Sent data to leader:\n" << doc.toJson(QJsonDocument::Indented);
    }
}

void IngestionClient::sendToMetadataAnalyticsLeader(const QJsonDocument& doc) {
    QTcpSocket* leaderSocket = new QTcpSocket(this);
    connect(leaderSocket, &QTcpSocket::connected, [leaderSocket, doc]() {
        leaderSocket->write(doc.toJson(QJsonDocument::Indented));
    });
    connect(leaderSocket, &QTcpSocket::disconnected, leaderSocket, &QTcpSocket::deleteLater);
    leaderSocket->connectToHost(metadataAnalyticsLeader, 12351);
}
