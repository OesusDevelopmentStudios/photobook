/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <QTest>

#include "hlp/KeychainErrors.hpp"

namespace photobook::token_store
{

// The keyring itself is not used here: a test must not write into the user's real KWallet. These tests
// cover what a failed keyring job means for the app.
class TestTokenStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tellsWhatAKeyringErrorMeans_data();
    void tellsWhatAKeyringErrorMeans();
    void keepsTheKeyringsMessage();
};

void TestTokenStore::tellsWhatAKeyringErrorMeans_data()
{
    QTest::addColumn<int>("keychainError");
    QTest::addColumn<int>("expectedKind");

    const auto unavailable = static_cast<int>(TokenStoreError::Kind::Unavailable);
    const auto accessDenied = static_cast<int>(TokenStoreError::Kind::AccessDenied);
    const auto failed = static_cast<int>(TokenStoreError::Kind::Failed);
    QTest::newRow("no keyring") << static_cast<int>(QKeychain::NoBackendAvailable) << unavailable;
    QTest::newRow("not on this system") << static_cast<int>(QKeychain::NotImplemented) << unavailable;
    QTest::newRow("user said no") << static_cast<int>(QKeychain::AccessDeniedByUser) << accessDenied;
    QTest::newRow("access denied") << static_cast<int>(QKeychain::AccessDenied) << accessDenied;
    QTest::newRow("could not delete") << static_cast<int>(QKeychain::CouldNotDeleteEntry) << failed;
    QTest::newRow("other") << static_cast<int>(QKeychain::OtherError) << failed;
}

void TestTokenStore::tellsWhatAKeyringErrorMeans()
{
    QFETCH(int, keychainError);
    QFETCH(int, expectedKind);

    const TokenStoreError error = errorFrom(static_cast<QKeychain::Error>(keychainError), QString());

    QCOMPARE(static_cast<int>(error.kind), expectedKind);
}

void TestTokenStore::keepsTheKeyringsMessage()
{
    const TokenStoreError error = errorFrom(QKeychain::OtherError, QStringLiteral("The wallet is closed"));

    QCOMPARE(error.message, QStringLiteral("The wallet is closed"));
}

}  // namespace photobook::token_store

QTEST_GUILESS_MAIN(photobook::token_store::TestTokenStore)
#include "TestTokenStore.moc"
