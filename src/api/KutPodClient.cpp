#include "KutPodClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDateTime>
#include <QRegularExpression>
#include <QTimer>

KutPodClient::KutPodClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void KutPodClient::configure(const QString &instanceUrl,
                                const QString &apiUser,
                                const QString &apiPassword)
{
    m_baseUrl     = instanceUrl.trimmed().remove(QRegularExpression("/$")) + "/api";
    m_apiUser     = apiUser;
    m_apiPassword = apiPassword;
    m_token.clear();
    m_configured  = true;
}

void KutPodClient::ensureAuth(std::function<void()> onSuccess)
{
    if (!m_token.isEmpty()) {
        onSuccess();
        return;
    }
    
    m_authQueue.append(onSuccess);
    if (m_authenticating) return;
    m_authenticating = true;
    
    QNetworkRequest request(QUrl(m_baseUrl + "/auth/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject body;
    body["user"] = m_apiUser;
    body["password"] = m_apiPassword;
    
    QNetworkReply *reply = m_nam->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_authenticating = false;
        
        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->errorString();
            int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            emit errorOccurred(QString("Auth error (%1): %2").arg(code).arg(err));
            m_authQueue.clear();
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject() && doc.object().contains("token")) {
            m_token = doc.object()["token"].toString();
            auto queue = m_authQueue;
            m_authQueue.clear();
            for (auto &cb : queue) cb();
        } else {
            emit errorOccurred("Login failed. No token received.");
            m_authQueue.clear();
        }
    });
}

// ──────────────────────────────────────────────────────────────
//  Podcasts
// ──────────────────────────────────────────────────────────────

void KutPodClient::fetchPodcasts()
{
    ensureAuth([this]() {
        auto req = buildRequest("/podcasts");
        handleReply(m_nam->get(req), [this](const QJsonDocument &doc) {
            QList<Podcast> list;
            for (const QJsonValue &v : doc.array()) {
                QJsonObject o = v.toObject();
                Podcast p;
                p.id = o["id"].toInt();
                p.handle = o["slug"].toString(); // Mapped slug to handle
                p.title = o["name"].toString();  // Mapped name to title
                p.description = o["description"].toString();
                const QJsonValue coverVal = o["cover"];
                if (coverVal.isObject())
                    p.coverUrl = coverVal.toObject()["url"].toString();
                else if (o.contains("cover_url"))
                    p.coverUrl = o["cover_url"].toString();
                else
                    p.coverUrl = coverVal.toString();

                if (!p.coverUrl.isEmpty() && !p.coverUrl.startsWith("http://") && !p.coverUrl.startsWith("https://")) {
                    QString rootUrl = m_baseUrl;
                    if (rootUrl.endsWith("/api")) rootUrl.chop(4);
                    if (!p.coverUrl.startsWith("/")) rootUrl += "/";
                    p.coverUrl = rootUrl + p.coverUrl;
                }
                list << p;
            }
            emit podcastsFetched(list);
        });
    });
}

void KutPodClient::fetchPodcastById(int podcastId)
{
    // KutPod doesn't have a single GET /podcasts/{id} without filtering manually in the API.
    // Instead we fetch all and find the right one.
    ensureAuth([this, podcastId]() {
        auto req = buildRequest("/podcasts");
        handleReply(m_nam->get(req), [this, podcastId](const QJsonDocument &doc) {
            for (const QJsonValue &v : doc.array()) {
                QJsonObject o = v.toObject();
                if (o["id"].toInt() == podcastId) {
                    Podcast p;
                    p.id = o["id"].toInt();
                    p.handle = o["slug"].toString();
                    p.title = o["name"].toString();
                    p.description = o["description"].toString();
                    const QJsonValue coverVal = o["cover"];
                    if (coverVal.isObject())
                        p.coverUrl = coverVal.toObject()["url"].toString();
                    else if (o.contains("cover_url"))
                        p.coverUrl = o["cover_url"].toString();
                    else
                        p.coverUrl = coverVal.toString();

                    if (!p.coverUrl.isEmpty() && !p.coverUrl.startsWith("http://") && !p.coverUrl.startsWith("https://")) {
                        QString rootUrl = m_baseUrl;
                        if (rootUrl.endsWith("/api")) rootUrl.chop(4);
                        if (!p.coverUrl.startsWith("/")) rootUrl += "/";
                        p.coverUrl = rootUrl + p.coverUrl;
                    }
                    emit podcastFetched(p);
                    return;
                }
            }
            emit errorOccurred("Podcast not found");
        });
    });
}

void KutPodClient::resolvePodcastHandle(const QString &handle)
{
    // Conectamos una sola vez para interceptar la respuesta y filtrar por handle (slug)
    connect(this, &KutPodClient::podcastsFetched,
            this, [this, handle](const QList<Podcast> &podcasts) {
        for (const Podcast &p : podcasts) {
            if (p.handle == handle) {
                emit podcastIdResolved(handle, p.id);
                return;
            }
        }
        emit errorOccurred(
            QString("No se encontró podcast con slug '%1'").arg(handle));
    }, Qt::SingleShotConnection);
    fetchPodcasts();
}

// ──────────────────────────────────────────────────────────────
//  Episodes
// ──────────────────────────────────────────────────────────────

void KutPodClient::fetchRecentEpisodes(int podcastId, int limit)
{
    ensureAuth([this, podcastId, limit]() {
        QUrlQuery q;
        q.addQueryItem("per_page", QString::number(limit));
        q.addQueryItem("page", "1");

        handleReply(m_nam->get(buildRequest(QString("/podcasts/%1/episodes").arg(podcastId), q)), 
                    [this, podcastId](const QJsonDocument &doc) {
            QList<Episode> list;
            QJsonArray arr = doc.object()["items"].toArray();
            for (const QJsonValue &v : arr) {
                QJsonObject o = v.toObject();
                Episode e = Episode::fromJson(o);
                if (e.episodeNumber <= 0 && o.contains("episode")) {
                    e.episodeNumber = o["episode"].toInt();
                }
                if (o.contains("status")) {
                    e.isDraft = (o["status"].toString() == "draft");
                }
                if (!e.audioUrl.isEmpty() && !e.audioUrl.startsWith("http://") && !e.audioUrl.startsWith("https://")) {
                    QString rootUrl = m_baseUrl;
                    if (rootUrl.endsWith("/api")) rootUrl.chop(4);
                    if (!e.audioUrl.startsWith("/")) rootUrl += "/";
                    e.audioUrl = rootUrl + e.audioUrl;
                }
                list << e;
            }
            emit episodesFetched(podcastId, list);
        });
    });
}

void KutPodClient::fetchAllEpisodes(int podcastId)
{
    constexpr int pageSize = 100;

    struct State {
        QList<Episode> all;
        int page = 1;
    };
    auto state = std::make_shared<State>();

    auto fetchPage = std::make_shared<std::function<void()>>();

    *fetchPage = [this, state, podcastId, fetchPage]() mutable {
        ensureAuth([this, state, podcastId, fetchPage]() {
            QUrlQuery q;
            q.addQueryItem("per_page", QString::number(pageSize));
            q.addQueryItem("page",     QString::number(state->page));

            handleReply(m_nam->get(buildRequest(QString("/podcasts/%1/episodes").arg(podcastId), q)),
                        [this, state, podcastId, fetchPage](const QJsonDocument &doc) mutable {
                            QJsonArray arr = doc.object()["items"].toArray();
                            if (arr.isEmpty()) {
                                emit episodesFetched(podcastId, state->all);
                                return;
                            }

                            for (const QJsonValue &v : arr) {
                                QJsonObject o = v.toObject();
                                Episode e = Episode::fromJson(o);
                                if (e.episodeNumber <= 0 && o.contains("episode")) {
                                    e.episodeNumber = o["episode"].toInt();
                                }
                                if (o.contains("status")) {
                                    e.isDraft = (o["status"].toString() == "draft");
                                }
                                if (!e.audioUrl.isEmpty() && !e.audioUrl.startsWith("http://") && !e.audioUrl.startsWith("https://")) {
                                    QString rootUrl = m_baseUrl;
                                    if (rootUrl.endsWith("/api")) rootUrl.chop(4);
                                    if (!e.audioUrl.startsWith("/")) rootUrl += "/";
                                    e.audioUrl = rootUrl + e.audioUrl;
                                }
                                state->all << e;
                            }

                            int totalPages = doc.object()["pages"].toInt();
                            if (state->page < totalPages) {
                                state->page++;
                                QTimer::singleShot(0, *fetchPage);
                            } else {
                                emit episodesFetched(podcastId, state->all);
                            }
                        });
        });
    };

    // Start first page
    QTimer::singleShot(0, *fetchPage);
}

// ──────────────────────────────────────────────────────────────
//  Publish
// ──────────────────────────────────────────────────────────────

void KutPodClient::publishEpisode(const PublishRequest &req)
{
    // Paso 1: resolver slug → podcast_id, luego crear/publicar
    connect(this, &KutPodClient::podcastIdResolved,
            this, [this, req](const QString &handle, int podcastId) {
        if (handle == req.podcastHandle) {
            ensureAuth([this, podcastId, req]() {
                auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

                auto addField = [&](const QString &name, const QString &value) {
                    QHttpPart part;
                    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                                   QString("form-data; name=\"%1\"").arg(name));
                    part.setBody(value.toUtf8());
                    multiPart->append(part);
                };

                addField("title", req.title);
                
                if (!req.description.isEmpty())
                    addField("description", req.description);
                if (req.episodeNumber > 0)
                    addField("episode", QString::number(req.episodeNumber));
                if (req.seasonNumber > 0)
                    addField("season", QString::number(req.seasonNumber));
                if (req.isExplicit)
                    addField("explicit", "1");
                if (!req.type.isEmpty())
                    addField("episode_type", req.type);

                if (!req.publishedAt.isEmpty()) {
                    QDateTime dt = QDateTime::fromString(req.publishedAt, Qt::ISODate);
                    if (dt.isValid() && dt > QDateTime::currentDateTime().addSecs(60))
                        addField("publish_at", dt.toString("yyyy-MM-dd HH:mm:ss"));
                }

                if (!req.audioFilePath.isEmpty()) {
                    QFile *audioFile = new QFile(req.audioFilePath, multiPart);
                    if (audioFile->open(QIODevice::ReadOnly)) {
                        QHttpPart audioPart;
                        audioPart.setHeader(QNetworkRequest::ContentTypeHeader, "audio/mpeg");
                        audioPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                            QString("form-data; name=\"audio\"; filename=\"%1\"")
                                                .arg(QFileInfo(req.audioFilePath).fileName()));
                        audioPart.setBodyDevice(audioFile);
                        multiPart->append(audioPart);
                    }
                } else if (!req.audioUrl.isEmpty()) {
                    emit errorOccurred("KutPod no soporta audioUrls, requiere subir archivo local.");
                    multiPart->deleteLater();
                    return;
                }

                if (!req.coverFilePath.isEmpty()) {
                    QFile *coverFile = new QFile(req.coverFilePath, multiPart);
                    if (coverFile->open(QIODevice::ReadOnly)) {
                        QMimeDatabase db;
                        QString mime = db.mimeTypeForFile(req.coverFilePath).name();
                        QHttpPart coverPart;
                        coverPart.setHeader(QNetworkRequest::ContentTypeHeader, mime);
                        coverPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                            QString("form-data; name=\"cover\"; filename=\"%1\"")
                                                .arg(QFileInfo(req.coverFilePath).fileName()));
                        coverPart.setBodyDevice(coverFile);
                        multiPart->append(coverPart);
                    }
                }

                QNetworkRequest request = buildRequest(QString("/podcasts/%1/episodes").arg(podcastId));
                QNetworkReply *reply = m_nam->post(request, multiPart);
                multiPart->setParent(reply);

                connect(reply, &QNetworkReply::uploadProgress, this, &KutPodClient::uploadProgress);

                connect(reply, &QNetworkReply::finished, this,
                        [this, reply]() {
                    reply->deleteLater();

                    if (reply->error() != QNetworkReply::NoError) {
                        emit errorOccurred(QString("Error de conexión: %1").arg(reply->errorString()));
                        return;
                    }

                    int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    QByteArray raw = reply->readAll();
                    QJsonDocument doc = QJsonDocument::fromJson(raw);

                    if (httpCode >= 400) {
                        QString msg;
                        if (!doc.isNull() && doc.isObject()) {
                            auto o = doc.object();
                            msg = o["messages"].toObject()["error"].toString(
                                  o["message"].toString(
                                  o["error"].toString()));
                        }
                        if (msg.isEmpty())
                            msg = QString::fromUtf8(raw.left(400));
                        emit errorOccurred(QString("KutPod HTTP %1: %2").arg(httpCode).arg(msg));
                        return;
                    }

                    int episodeId = doc.object()["id"].toInt();
                    if (!episodeId) {
                        emit errorOccurred("KutPod no devolvió el ID del episodio.");
                        return;
                    }

                    Episode e;
                    e.id = episodeId;
                    e.title = doc.object()["title"].toString();
                    emit episodePublished(e);
                });
            });
        }
    }, Qt::SingleShotConnection);
    resolvePodcastHandle(req.podcastHandle);
}

void KutPodClient::publishDraft(int podcastId, int episodeId)
{
    ensureAuth([this, podcastId, episodeId]() {
        auto req = buildRequest(QString("/podcasts/%1/episodes/%2").arg(podcastId).arg(episodeId));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject body;
        body["status"] = "published";
        body["published_at"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

        handleReply(m_nam->put(req, QJsonDocument(body).toJson()), [this, episodeId](const QJsonDocument &/*doc*/) {
            emit draftPublished(episodeId);
        });
    });
}

// ──────────────────────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────────────────────

QNetworkRequest KutPodClient::buildRequest(const QString &endpoint,
                                              const QUrlQuery &query) const
{
    QUrl url(m_baseUrl + endpoint);
    if (!query.isEmpty())
        url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer " + m_token).toUtf8());
    request.setRawHeader("Accept",        "application/json");
    request.setTransferTimeout(600'000);  // 10 min para uploads grandes
    return request;
}

void KutPodClient::handleReply(QNetworkReply *reply,
                                  std::function<void(const QJsonDocument &)> onSuccess)
{
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QString("Error de conexión: %1").arg(reply->errorString()));
            return;
        }

        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray raw = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(raw);

        if (httpCode >= 400) {
            QString msg;
            if (!doc.isNull() && doc.isObject()) {
                auto o = doc.object();
                msg = o["messages"].toObject()["error"].toString(
                      o["message"].toString(
                      o["error"].toString()));
            }
            if (msg.isEmpty())
                msg = QString::fromUtf8(raw.left(400));
            emit errorOccurred(QString("KutPod HTTP %1: %2").arg(httpCode).arg(msg));
            return;
        }

        onSuccess(doc);
    });
}
