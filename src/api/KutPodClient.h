#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include "ApiTypes.h"

class KutPodClient : public QObject
{
    Q_OBJECT

public:
    explicit KutPodClient(QObject *parent = nullptr);

    // Configura la conexión.
    void configure(const QString &instanceUrl,
                   const QString &apiUser,
                   const QString &apiPassword);

    bool isConfigured() const { return m_configured; }

    // ── Podcasts ──────────────────────────────────────────
    void fetchPodcasts();
    void fetchPodcastById(int podcastId);
    void resolvePodcastHandle(const QString &handle);   // → emite podcastIdResolved

    // ── Episodes ──────────────────────────────────────────
    void fetchRecentEpisodes(int podcastId, int limit = 20);
    void fetchAllEpisodes(int podcastId);

    // ── Publish ───────────────────────────────────────────
    void publishEpisode(const PublishRequest &req);
    void publishDraft(int podcastId, int episodeId);

signals:
    // ── Éxito ─────────────────────────────────────────────
    void podcastsFetched(const QList<Podcast> &podcasts);
    void podcastFetched(const Podcast &podcast);
    void podcastIdResolved(const QString &handle, int podcastId);
    void episodesFetched(int podcastId, const QList<Episode> &episodes);
    void episodePublished(const Episode &episode);
    void draftPublished(int episodeId);

    // ── Progreso (upload) ─────────────────────────────────
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

    // ── Error ─────────────────────────────────────────────
    void errorOccurred(const QString &message);

private:
    // Helpers internos
    void ensureAuth(std::function<void()> onSuccess);
    QNetworkRequest buildRequest(const QString &endpoint,
                                 const QUrlQuery &query = QUrlQuery()) const;
    void            handleReply(QNetworkReply *reply,
                                std::function<void(const QJsonDocument &)> onSuccess);

    QString m_baseUrl;
    QString m_apiUser;
    QString m_apiPassword;
    QString m_token;
    bool    m_configured = false;
    bool    m_authenticating = false;
    QList<std::function<void()>> m_authQueue;

    QNetworkAccessManager *m_nam;
};
