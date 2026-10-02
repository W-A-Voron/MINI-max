#pragma once
#include <QByteArray>
#include <QObject>
#include <QSslSocket>
#include <QTimer>

#include "config.h"

// MMProto client connection: TCP or TLS, handshake (HELLO), keepalive (PING) and auto-reconnect with backoff.
class Connection : public QObject {
    Q_OBJECT
public:
    enum class State { Disconnected, Connecting, Handshaking, Ready };
    Q_ENUM(State)

    explicit Connection(const ClientConfig &cfg, QObject *parent = nullptr);
    // Disconnect observers first: during parent teardown they may already be partially destroyed.
    ~Connection() override { disconnect(this, nullptr, nullptr, nullptr); stop(); }
    void start();
    void stop();
    quint32 send(quint16 type, const QByteArray &payload = {});

    State state() const { return m_state; }
    QString serverName() const { return m_serverName; }
    QString serverVersion() const { return m_serverVersion; }

signals:
    void stateChanged(Connection::State state);
    void frameReceived(quint16 type, quint32 requestId, const QByteArray &payload);

private:
    void connectNow();
    void setState(State s);
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onError();
    void scheduleReconnect();
    void handleFrame(quint16 type, quint32 reqId, const QByteArray &payload);

    ClientConfig m_cfg;
    QSslSocket m_sock;
    QTimer m_reconnect;
    QTimer m_keepalive;
    QByteArray m_in;
    State m_state = State::Disconnected;
    bool m_running = false;
    int m_backoffMs;
    quint32 m_nextReq = 1;
    quint32 m_maxFrame = 16u * 1024 * 1024;
    QString m_serverName, m_serverVersion;
};
