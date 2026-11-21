/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <functional>
#include <memory>

#include <QHash>
#include <QPromise>
#include <QSet>

#include "HttpTransport.hpp"

namespace pictorio::fakes
{

inline server_client::HttpResponse response(int status, const QByteArray& body)
{
    server_client::HttpResponse answer;
    answer.status = status;
    answer.body = body;
    return answer;
}

inline server_client::HttpResponse problem(int status, const QByteArray& code)
{
    return response(status,
                    R"({"type":"t","title":"Problem","status":)" + QByteArray::number(status) + R"(,"code":)" +
                        ('"' + code + '"') + "}");
}

inline QByteArray authorizationOf(const server_client::HttpRequest& request)
{
    for (const auto& [name, value] : request.headers)
    {
        if (name == "Authorization")
        {
            return value;
        }
    }

    return {};
}

// Answers by method and address (without the query), with canned responses or a function. Addresses can
// be made unreachable, as if the server were off; answers can be held back, as if the network were slow.
// Records every request.
class FakeServer : public server_client::HttpTransport
{
public:
    using Handler = std::function<server_client::HttpResponse(const server_client::HttpRequest&)>;

    QFuture<server_client::HttpResponse> send(const server_client::HttpRequest& request) override
    {
        requests.append(request);
        if (unreachableOrigins.contains(request.url.adjusted(QUrl::RemovePath | QUrl::RemoveQuery).toString()))
        {
            server_client::HttpResponse noAnswer;
            noAnswer.networkError = QStringLiteral("Connection refused");
            return QtFuture::makeReadyValueFuture(noAnswer);
        }

        const QString key = keyOf(request.method, request.url);
        const server_client::HttpResponse answer =
            m_handlers.contains(key) ? m_handlers.value(key)(request) : m_answers.value(key, problem(404, "not_found"));
        if (!holdAnswers)
        {
            return QtFuture::makeReadyValueFuture(answer);
        }

        auto promise = std::make_shared<QPromise<server_client::HttpResponse>>();
        m_heldAnswers.append({promise, answer});
        return promise->future();
    }

    // Sends the answers held back so far, oldest first.
    void releaseAnswers()
    {
        const auto heldAnswers = std::exchange(m_heldAnswers, {});
        for (const auto& [promise, answer] : heldAnswers)
        {
            promise->start();
            promise->addResult(answer);
            promise->finish();
        }
    }

    // Sends one held answer, e.g. the first of two to arrive last.
    void releaseAnswer(qsizetype index)
    {
        const auto [promise, answer] = m_heldAnswers.takeAt(index);
        promise->start();
        promise->addResult(answer);
        promise->finish();
    }

    void answer(const char* method, const QString& url, const server_client::HttpResponse& answer)
    {
        m_answers.insert(keyOf(method, QUrl(url)), answer);
    }

    void answerWith(const char* method, const QString& url, Handler handler)
    {
        m_handlers.insert(keyOf(method, QUrl(url)), std::move(handler));
    }

    QList<server_client::HttpRequest> requestsTo(const char* method, const QString& url) const
    {
        QList<server_client::HttpRequest> found;
        for (const server_client::HttpRequest& request : requests)
        {
            if (keyOf(request.method, request.url) == keyOf(method, QUrl(url)))
            {
                found.append(request);
            }
        }

        return found;
    }

    QSet<QString> unreachableOrigins;  // e.g. "https://raspberrypi.local:8737"
    bool holdAnswers = false;          // until releaseAnswers()
    QList<server_client::HttpRequest> requests;

private:
    static QString keyOf(const QByteArray& method, const QUrl& url)
    {
        return QString::fromLatin1(method) + QLatin1Char(' ') + url.adjusted(QUrl::RemoveQuery).toString();
    }

    QHash<QString, server_client::HttpResponse> m_answers;
    QHash<QString, Handler> m_handlers;
    QList<QPair<std::shared_ptr<QPromise<server_client::HttpResponse>>, server_client::HttpResponse>> m_heldAnswers;
};

}  // namespace pictorio::fakes
