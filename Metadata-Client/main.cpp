#include <QCoreApplication>
#include <QDebug>
#include "metadata_client.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    MetadataClient metadataClient;
    metadataClient.start();

    return a.exec();
}
