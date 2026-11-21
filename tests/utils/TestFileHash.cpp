/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "utils/FileHash.hpp"

#include <memory>

#include <QCryptographicHash>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace photobook::utils
{

class TestFileHash : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void hashesAsTheServerWrites_data();
    void hashesAsTheServerWrites();
    void hashesALargeFileInPieces();
    void givesNothingForAMissingFile();

private:
    QString fileWith(const QByteArray& content) const;

    std::unique_ptr<QTemporaryDir> m_folder;
};

void TestFileHash::init()
{
    m_folder = std::make_unique<QTemporaryDir>();
    QVERIFY(m_folder->isValid());
}

QString TestFileHash::fileWith(const QByteArray& content) const
{
    const QString path = m_folder->filePath(QStringLiteral("beach.jpg"));
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(content);
    }

    return path;
}

// The well-known SHA-256 values, in lowercase hex
void TestFileHash::hashesAsTheServerWrites_data()
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<QString>("hash");

    QTest::newRow("empty") << QByteArray()
                           << QStringLiteral("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    QTest::newRow("abc") << QByteArray("abc")
                         << QStringLiteral("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

void TestFileHash::hashesAsTheServerWrites()
{
    QFETCH(QByteArray, content);
    QFETCH(QString, hash);

    QCOMPARE(sha256Of(fileWith(content)), hash);
}

// Larger than one piece read at a time
void TestFileHash::hashesALargeFileInPieces()
{
    QByteArray content;
    for (int part = 0; part < 100000; ++part)
    {
        content += QByteArray::number(part);
    }

    const QString expected = QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());

    QCOMPARE(sha256Of(fileWith(content)), expected);
}

void TestFileHash::givesNothingForAMissingFile()
{
    QCOMPARE(sha256Of(m_folder->filePath(QStringLiteral("missing.jpg"))), std::nullopt);
}

}  // namespace photobook::utils

QTEST_GUILESS_MAIN(photobook::utils::TestFileHash)

#include "TestFileHash.moc"
