#include "net/connection.h"

#include <QFile>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <algorithm>

#include "minimax/mmproto.h"

Connection::Connection(const ClientConfig &cfg, QObject *parent)
    : QObject(parent), m_cfg(cfg), m_sock(this), m_backoffMs(cfg.reconnectMinMs) {
    m_reconnect.setSingleShot(true);
    connect(&m_reconnect, &QTimer::timeout, this, &Connection::connectNow);
    m_keepalive.setInterval(cfg.keepaliveSeconds * 1000);
    connect(&m_keepalive, &QTimer::timeout, this, [this] { send(MM_MSG_PING); });

    connect(&m_sock, &QSslSocket::connected, this, [this] { if (!m_cfg.tlsEnabled) onConnected(); });
    connect(&m_sock, &QSslSocket::encrypted, this, &Connection::onConnected);
    connect(&m_sock, &QSslSocket::readyRead, this, &Connection::onReadyRead);
    connect(&m_sock, &QSslSocket::disconnected, this, &Connection::onDisconnected);
    connect(&m_sock, &QAbstractSocket::errorOccurred, this, &Connection::onError);
    connect(&m_sock, &QSslSocket::sslErrors, this, [this](const QList<QSslError> &errs) {
        if (!m_cfg.tlsVerifyPeer) m_sock.ignoreSslErrors(errs);
    });

    if (cfg.tlsEnabled) {
        QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
        if (!cfg.tlsCaFile.isEmpty()) {
            QList<QSslCertificate> cas = QSslCertificate::fromPath(cfg.tlsCaFile);
            ssl.addCaCertificates(cas);
        }
        if (!cfg.tlsVerifyPeer) ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
        m_sock.setSslConfiguration(ssl);
    }
}

void Connection::start() {
    m_running = true;
    connectNow();
}

void Connection::stop() {
    m_running = false;
    m_reconnect.stop();
    m_keepalive.stop();
    m_sock.abort();
    setState(State::Disconnected);
}

void Connection::setState(State s) {
    if (s == m_state) return;
    m_state = s;
    emit stateChanged(s);
}

void Connection::connectNow() {
    if (!m_running) return;
    m_in.clear();
    setState(State::Connecting);
    if (m_cfg.tlsEnabled) m_sock.connectToHostEncrypted(m_cfg.host, m_cfg.port);
    else m_sock.connectToHost(m_cfg.host, m_cfg.port);
}

void Connection::onConnected() {
    setState(State::Handshaking);
    QByteArray tlv;
    mm_buf p;
    mm_buf_init(&p);
    mm_tlv_put_str(&p, MM_TAG_CLIENT_NAME, "MINI max Desktop");
    send(MM_MSG_HELLO, QByteArray(reinterpret_cast<const char *>(p.data), static_cast<int>(p.len)));
    mm_buf_free(&p);
}

quint32 Connection::send(quint16 type, const QByteArray &payload) {
    if (m_sock.state() != QAbstractSocket::ConnectedState) return 0;
    quint32 id = m_nextReq++;
    mm_buf out;
    mm_buf_init(&out);
    if (mm_frame_encode(&out, type, 0, id, payload.constData(), static_cast<size_t>(payload.size()), m_maxFrame) == MM_OK)
        m_sock.write(reinterpret_cast<const char *>(out.data), static_cast<qint64>(out.len));
    else
        id = 0;
    mm_buf_free(&out);
    return id;
}

void Connection::onReadyRead() {
    m_in += m_sock.readAll();
    for (;;) {
        mm_frame f;
        size_t used = 0;
        mm_status st = mm_frame_decode(reinterpret_cast<const uint8_t *>(m_in.constData()),
                                       static_cast<size_t>(m_in.size()), m_maxFrame, &f, &used);
        if (st == MM_NEED_MORE) return;
        if (st != MM_OK) { m_sock.abort(); return; }
        QByteArray payload(reinterpret_cast<const char *>(f.payload), static_cast<int>(f.payload_len));
        quint16 type = f.type;
        quint32 req = f.request_id;
        m_in.remove(0, static_cast<int>(used));
        handleFrame(type, req, payload);
    }
}

void Connection::handleFrame(quint16 type, quint32 reqId, const QByteArray &payload) {
    if (type == MM_MSG_HELLO) {
        auto str = [&](uint16_t tag) {
            const uint8_t *v;
            size_t l;
            if (mm_tlv_find(reinterpret_cast<const uint8_t *>(payload.constData()), static_cast<size_t>(payload.size()), tag, &v, &l))
                return QString();
            return QString::fromUtf8(reinterpret_cast<const char *>(v), static_cast<int>(l));
        };
        m_serverName = str(MM_TAG_SERVER_NAME);
        m_serverVersion = str(MM_TAG_SERVER_VERSION);
        uint32_t mf;
        if (!mm_tlv_get_u32(reinterpret_cast<const uint8_t *>(payload.constData()), static_cast<size_t>(payload.size()), MM_TAG_MAX_FRAME, &mf))
            m_maxFrame = mf;
        m_backoffMs = m_cfg.reconnectMinMs;
        m_keepalive.start();
        setState(State::Ready);
    }
    emit frameReceived(type, reqId, payload);
}

void Connection::onDisconnected() {
    m_keepalive.stop();
    setState(State::Disconnected);
    scheduleReconnect();
}

void Connection::onError() {
    if (m_sock.state() == QAbstractSocket::UnconnectedState) {
        setState(State::Disconnected);
        scheduleReconnect();
    }
}

void Connection::scheduleReconnect() {
    if (!m_running || m_reconnect.isActive()) return;
    m_reconnect.start(m_backoffMs);
    m_backoffMs = std::min(m_backoffMs * 2, m_cfg.reconnectMaxMs);
}
