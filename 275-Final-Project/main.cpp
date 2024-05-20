#include <QCoreApplication>
#include <QDebug>
#include "ingestion_main.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);


    // Start the ingestion client
    IngestionClient ingestionClient;
    ingestionClient.start();
    ingestionClient.startListeningForMetadataClient(12346);  // Listen for MetadataClient connections

    return a.exec();
}

