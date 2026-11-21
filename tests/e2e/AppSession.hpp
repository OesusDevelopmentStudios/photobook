/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>
#include <optional>
#include <utility>

#include <QDir>
#include <QDirIterator>
#include <QImage>
#include <QList>
#include <QRandomGenerator>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "ApiTypes.hpp"
#include "HiddenFilterEditor.hpp"
#include "Library.hpp"
#include "LibraryFactory.hpp"
#include "TagEditor.hpp"
#include "TagEditorFactory.hpp"
#include "UploadTagEditor.hpp"
#include "Uploader.hpp"
#include "e2e/EndToEndServer.hpp"
#include "fakes/FakeTrash.hpp"

namespace pictorio::e2e
{

// A small image of random pixels, so that every image is new to the server, which refuses a file it has. Its type
// comes from the file's suffix, e.g. PNG.
inline bool writeSampleImage(const QString& filePath, QSize size)
{
    QImage image(size, QImage::Format_RGB32);
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            image.setPixel(x, y, QRandomGenerator::global()->generate() | 0xff000000);
        }
    }

    return image.save(filePath);
}

// The image files the server keeps for an uploaded file, by their path in its library folder, e.g.
// "photo/sample_artist/2026/beach_3f2a….png". The server names a file after the original and its hash.
inline QStringList storedFiles(const EndToEndServer& server, const QString& originalStem)
{
    const QDir library(server.libraryDir);
    QStringList files;
    QDirIterator found(
        server.libraryDir, {originalStem + QStringLiteral("_*")}, QDir::Files, QDirIterator::Subdirectories);
    while (found.hasNext())
    {
        files.append(library.relativeFilePath(found.next()));
    }

    return files;
}

// The app's services as AppManager wires them, for one user. Originals go to a fake trash, which only notes them.
class AppSession
{
public:
    // A tag as typed into a namespace's row, e.g. {"author", "Sample Artist"}
    using TypedTag = std::pair<QString, QString>;

    AppSession()
        : m_connection(m_folder.filePath(QStringLiteral("pictoriorc")), std::make_shared<fakes::Keyring>())
        , m_uploadTagEditor(tag_editor::createUploadTagEditor(&*m_connection))
        , m_uploader(std::make_unique<uploader::Uploader>(
              &*m_connection, m_uploadTagEditor.get(), std::make_unique<fakes::FakeTrash>(m_trashCan)))
        , m_library(library::createLibrary(&*m_connection))
        , m_tagEditor(tag_editor::createTagEditor(&*m_connection))
        , m_hiddenFilterEditor(tag_editor::createHiddenFilterEditor(&*m_connection))
    {}

    bool logIn(const EndToEndServer& server, const QString& username)
    {
        return m_connection.logIn(server, username, server.password);
    }

    connection::Connection& connection()
    {
        return *m_connection;
    }

    uploader::Uploader& uploader()
    {
        return *m_uploader;
    }

    tag_editor::UploadTagEditor& uploadTagEditor()
    {
        return *m_uploadTagEditor;
    }

    const fakes::TrashCan& trashCan() const
    {
        return *m_trashCan;
    }

    // A new sample image in this session's folder, e.g. "beach" for beach.png; empty if it could not be written
    QString newSampleImage(const QString& stem, const QString& suffix = QStringLiteral("png"), QSize size = {16, 16})
    {
        const QString filePath = m_folder.filePath(stem + QLatin1Char('.') + suffix);
        return writeSampleImage(filePath, size) ? filePath : QString();
    }

    // As on the upload page: the tags typed into their rows, the file added, Start. How the file went.
    std::pair<uploader::Uploader::FileState, QString> upload(const QString& filePath, const QList<TypedTag>& tags)
    {
        m_uploader->clear();
        m_uploadTagEditor->clear();
        m_uploadTagEditor->load();
        for (const auto& [namespaceName, typedName] : tags)
        {
            if (!m_uploadTagEditor->addTag(namespaceName, typedName))
            {
                return {uploader::Uploader::FileState::Failed, m_uploadTagEditor->errorMessage()};
            }
        }

        m_uploader->addFiles({QUrl::fromLocalFile(filePath)});
        if (!QTest::qWaitFor([this]() { return m_uploader->waitingCount() == 1; }, answerTimeout))
        {
            return {uploader::Uploader::FileState::Failed, QStringLiteral("The file was not added")};
        }

        m_uploader->start();
        if (!QTest::qWaitFor([this]() { return !m_uploader->isRunning(); }, answerTimeout))
        {
            return {uploader::Uploader::FileState::Failed, QStringLiteral("The upload did not end")};
        }

        const QModelIndex row = m_uploader->index(0);
        return {row.data(uploader::Uploader::StateRole).value<uploader::Uploader::FileState>(),
                row.data(uploader::Uploader::MessageRole).toString()};
    }

    // As typed into the library's search field: every image found, in the library's order
    QList<server_client::Image> search(const QString& text)
    {
        m_library->search(text);
        QList<server_client::Image> images;
        if (!QTest::qWaitFor([this]() { return !m_library->isLoading(); }, answerTimeout))
        {
            return images;
        }

        for (int row = 0; row < m_library->count(); ++row)
        {
            images.append(*m_library->imageAt(row));
        }

        return images;
    }

    QString libraryError() const
    {
        return m_library->errorMessage();
    }

    // The last search as the server ran it, with the hidden filters it added
    QString effectiveQuery() const
    {
        return m_library->effectiveQuery();
    }

    // As in the viewer's footer: Edit Tags, then the change, then Save. The image as the server saved it.
    template <typename Change>
    std::optional<server_client::Image> editTags(const server_client::Image& image, Change change)
    {
        std::optional<server_client::Image> saved;
        const QMetaObject::Connection onSaved =
            QObject::connect(m_tagEditor.get(),
                             &tag_editor::TagEditor::saved,
                             [&saved](const server_client::Image& answer) { saved = answer; });
        m_tagEditor->setImage(image);
        m_tagEditor->startEditing();
        if (change(*m_tagEditor))
        {
            m_tagEditor->save();
            if (!QTest::qWaitFor([&saved, this]() { return saved.has_value() || !m_tagEditor->isSaving(); },
                                 answerTimeout))
            {
                m_tagEditor->cancel();  // no answer: nothing saved
            }
        }

        QObject::disconnect(onSaved);
        return saved;
    }

    QString tagEditorError() const
    {
        return m_tagEditor->errorMessage();
    }

    // As on the settings page: the saved filters loaded, these instead, Save. True once the server has them.
    bool saveHiddenFilters(const QStringList& filters)
    {
        tag_editor::HiddenFilterEditor& editor = *m_hiddenFilterEditor;
        editor.load();
        if (!QTest::qWaitFor([&editor]() { return !editor.isLoading(); }, answerTimeout) ||
            !editor.errorMessage().isEmpty())
        {
            return false;
        }

        for (const QString& filter : editor.filtersAfterChanges())
        {
            editor.removeTag(filter);
        }

        for (const QString& filter : filters)
        {
            if (!editor.addTag({}, filter))
            {
                return false;
            }
        }

        if (!editor.hasChanges())
        {
            return true;  // already as wanted
        }

        bool saved = false;
        const QMetaObject::Connection onSaved =
            QObject::connect(&editor, &tag_editor::HiddenFilterEditor::saved, [&saved]() { saved = true; });
        editor.save();
        const bool answered =
            QTest::qWaitFor([&saved, &editor]() { return saved || !editor.isSaving(); }, answerTimeout);
        QObject::disconnect(onSaved);
        return answered && saved;
    }

    QString hiddenFilterError() const
    {
        return m_hiddenFilterEditor->errorMessage();
    }

private:
    QTemporaryDir m_folder;  // the settings and the sample images
    AppConnection m_connection;
    std::shared_ptr<fakes::TrashCan> m_trashCan = std::make_shared<fakes::TrashCan>();
    std::unique_ptr<tag_editor::UploadTagEditor> m_uploadTagEditor;
    std::unique_ptr<uploader::Uploader> m_uploader;
    std::unique_ptr<library::Library> m_library;
    std::unique_ptr<tag_editor::TagEditor> m_tagEditor;
    std::unique_ptr<tag_editor::HiddenFilterEditor> m_hiddenFilterEditor;
};

}  // namespace pictorio::e2e
