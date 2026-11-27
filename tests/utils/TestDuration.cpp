/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <QTest>

#include "utils/Duration.hpp"

namespace photobook::utils
{

class TestDuration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void showsMinutesAndSeconds_data();
    void showsMinutesAndSeconds();
};

void TestDuration::showsMinutesAndSeconds_data()
{
    QTest::addColumn<qint64>("milliseconds");
    QTest::addColumn<QString>("text");

    QTest::newRow("under a second") << qint64(600) << QStringLiteral("0:00");
    QTest::newRow("seconds") << qint64(7999) << QStringLiteral("0:07");  // rounded down, as players count
    QTest::newRow("minutes") << qint64(222400) << QStringLiteral("3:42");
    QTest::newRow("just under an hour") << qint64(3599000) << QStringLiteral("59:59");
    QTest::newRow("an hour") << qint64(3600000) << QStringLiteral("1:00:00");
    QTest::newRow("hours") << qint64(3725000) << QStringLiteral("1:02:05");
    QTest::newRow("longer than a day") << qint64(90061000) << QStringLiteral("25:01:01");
    QTest::newRow("unknown") << qint64(-1) << QStringLiteral("0:00");
}

void TestDuration::showsMinutesAndSeconds()
{
    QFETCH(qint64, milliseconds);
    QFETCH(QString, text);

    QCOMPARE(durationText(milliseconds), text);
}

}  // namespace photobook::utils

QTEST_GUILESS_MAIN(photobook::utils::TestDuration)
#include "TestDuration.moc"
