/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "utils/MachineId.hpp"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace pictorio::utils
{

namespace
{

const QString machineId = QStringLiteral("0123456789abcdef0123456789abcdef");
// printf pictorio-db | openssl dgst -sha256 -hmac 0123456789abcdef0123456789abcdef; the server's tests use the
// same pair, so app and server compute alike.
const QString expectedId = QStringLiteral("9d623a361e613d33fd9937381c885a2ef999fa19c152bbbcadbf789bee807420");

}  // namespace

class TestMachineId : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void computesWhatTheServerComputes();
    void readsTheMachineIdFromItsFile();
    void hasNoIdWithoutAMachineId();
};

void TestMachineId::computesWhatTheServerComputes()
{
    QCOMPARE(appSpecificMachineId(machineId), expectedId);
    QCOMPARE(appSpecificMachineId(machineId + QStringLiteral("\n")), expectedId);  // as the file ends
}

void TestMachineId::readsTheMachineIdFromItsFile()
{
    QTemporaryDir folder;
    QFile file(folder.filePath(QStringLiteral("machine-id")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(machineId.toLatin1() + "\n");
    file.close();

    QCOMPARE(thisComputersAppSpecificId(file.fileName()), expectedId);
}

void TestMachineId::hasNoIdWithoutAMachineId()
{
    QTemporaryDir folder;
    QFile empty(folder.filePath(QStringLiteral("empty")));
    QVERIFY(empty.open(QIODevice::WriteOnly));
    empty.close();

    QVERIFY(thisComputersAppSpecificId(folder.filePath(QStringLiteral("missing"))).isEmpty());
    QVERIFY(thisComputersAppSpecificId(empty.fileName()).isEmpty());
}

}  // namespace pictorio::utils

QTEST_GUILESS_MAIN(pictorio::utils::TestMachineId)

#include "TestMachineId.moc"
