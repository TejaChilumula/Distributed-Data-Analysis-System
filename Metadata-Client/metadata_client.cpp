#include "metadata_client.h"
#include <QDebug>
#include <QCryptographicHash>
#include <omp.h>

MetadataClient::MetadataClient(QObject* parent)
    : QObject(parent),
    socket(new QTcpSocket(this)),
    electionTimer(new QTimer(this)),
    currentNumber(0),  // Set your node's unique number here
    ipAddress("YOUR_IP_ADDRESS"),  // Set your node's IP address here
    isLeader(false) {
    connect(socket, &QTcpSocket::connected, this, &MetadataClient::onServerConnected);
    connect(socket, &QTcpSocket::readyRead, this, &MetadataClient::onReadyRead);
    connect(electionTimer, &QTimer::timeout, this, &MetadataClient::onElectionTimeout);
}

void MetadataClient::start() {
    socket->connectToHost("CENTRAL_SERVER_IP", CENTRAL_SERVER_PORT);  // Update with actual IP and port
}

void MetadataClient::sendRegistrationRequest() {
    QJsonObject request;
    request["requestType"] = "registering";
    request["Ip"] = ipAddress;
    request["nodeType"] = "metadata";
    request["computingCapacity"] = 0.8;  // Example value, adjust as needed

    QJsonDocument doc(request);
    socket->write(doc.toJson(QJsonDocument::Indented));
    qDebug() << "Sent metadata registration request:\n" << doc.toJson(QJsonDocument::Indented);
}

void MetadataClient::onServerConnected() {
    sendRegistrationRequest();
}

void MetadataClient::onReadyRead() {
    QByteArray responseData = socket->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    QJsonObject responseObj = responseDoc.object();
    qDebug() << "Received from server:\n" << responseDoc.toJson(QJsonDocument::Indented);

    if (responseObj.contains("requestType")) {
        QString requestType = responseObj["requestType"].toString();
        if (requestType == "Node Discovery") {
            processNodeDiscovery(responseObj);
        } else if (requestType == "Election Message") {
            processElectionMessage(responseObj);
        } else if (requestType == "Leader Announcement") {
            processLeaderAnnouncement(responseObj);
        }
    }
}

void MetadataClient::processNodeDiscovery(const QJsonObject& message) {
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

    // Start election timer if necessary
    electionTimer->start(30000);  // 30 seconds delay before starting election
}

void MetadataClient::processElectionMessage(const QJsonObject& message) {
    int receivedNumber = message["Current Number"].toInt();
    QString receivedIp = message["IP"].toString();

    if (receivedNumber > currentNumber) {
        // Forward the election message with the higher number
        QJsonObject electionMessage;
        electionMessage["requestType"] = "Election Message";
        electionMessage["Current Number"] = receivedNumber;
        electionMessage["IP"] = receivedIp;

        QJsonDocument doc(electionMessage);
        socket->write(doc.toJson(QJsonDocument::Indented));
    } else if (receivedNumber < currentNumber) {
        // Send our number
        participateInElection();
    } else if (receivedNumber == currentNumber && receivedIp == ipAddress) {
        // We are the leader
        isLeader = true;
        announceLeadership();
    }
}

void MetadataClient::processLeaderAnnouncement(const QJsonObject& message) {
    QString leaderIp = message["leaderIP"].toString();
    QString nodeType = message["nodeType"].toString();

    if (nodeType == "metadata" && leaderIp != ipAddress) {
        isLeader = false;
        qDebug() << "Received leader announcement. Leader is:" << leaderIp;
    }
}

void MetadataClient::participateInElection() {
    QJsonObject electionMessage;
    electionMessage["requestType"] = "Election Message";
    electionMessage["Current Number"] = currentNumber;
    electionMessage["IP"] = ipAddress;

    QJsonDocument doc(electionMessage);
    socket->write(doc.toJson(QJsonDocument::Indented));
    qDebug() << "Sent election message:\n" << doc.toJson(QJsonDocument::Indented);
}

void MetadataClient::onElectionTimeout() {
    participateInElection();
    electionTimer->stop();  // Stop the timer once we participate in the election
}

void MetadataClient::announceLeadership() {
    QJsonObject leaderAnnouncement;
    leaderAnnouncement["requestType"] = "Leader Announcement";
    leaderAnnouncement["leaderIP"] = ipAddress;
    leaderAnnouncement["nodeType"] = "metadata";

    QJsonDocument doc(leaderAnnouncement);
    socket->write(doc.toJson(QJsonDocument::Indented));
    qDebug() << "Announced leadership:\n" << doc.toJson(QJsonDocument::Indented);

    // Once leadership is announced, send folder names to ingestion nodes
    if (isLeader) {
        sendHashedFolderNames();
    }
}

void MetadataClient::sendHashedFolderNames() {
    QHash<QString, QString> hashedFolderNames;

    // Simple hash to distribute folder names to ingestion nodes
    int folderIndex = 0;
    int nodeListSize = nodeList.size();

    #pragma omp parallel for
    for (int i = 0; i < nodeListSize; ++i) {
        const QString& nodeInfo = nodeList[i];
        if (nodeInfo.contains("ingestion")) {
            QString nodeIp = nodeInfo.split(", ")[1].split(": ")[1];  // Extract IP from nodeInfo
            QString folderName = folderNames[folderIndex % folderNames.size()];
            #pragma omp critical
            {
                hashedFolderNames.insert(nodeIp, folderName);
            }
            folderIndex++;
        }
    }

    for (auto it = hashedFolderNames.begin(); it != hashedFolderNames.end(); ++it) {
        QJsonObject message;
        message["requestType"] = "Assign Folder";
        message["folderName"] = it.value();

        QJsonDocument doc(message);
        socket->write(doc.toJson(QJsonDocument::Indented));
        qDebug() << "Sent folder assignment to" << it.key() << ":\n" << doc.toJson(QJsonDocument::Indented);
    }
}
