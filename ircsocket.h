#ifndef IRCSOCKET_H
#define IRCSOCKET_H
#include <QTcpSocket>
#include <QStringList>
#include <QQueue>

class IRCSocket : public QObject{
		Q_OBJECT
	public:
		enum State {
			NotConnected,
			HandshakeInProgress,
			Connected
		};

		IRCSocket();
		void connectToServer(const QString &address, quint16 port, const QString &nick);
		bool sendData(const QString &data);
		bool sendPrivateMessage(const QString &channel, const QString &msg);
		void joinChannel(QString channel, QString password = QString());
		void leave(QString channel, QString message = QString());
		void quit(QString message = QString());
		int whoQuery(const QString &queryString);
		QString nickname() const { return mNickname; }
	signals:
		void error(QAbstractSocket::SocketError socketError);
		void connected();
		void handshakeComplete();
		void privateMessage(QString sender, QString channel, QString msg);
		void channelMode(QString sender, QString channel, QString flags, QString params);
		void userMode(QString sender, QString flags);
		void userJoin(QString user, QString channel);
		void userQuit(QString user, QString msg);
		void userLeave(QString user, QString channel, QString msg);
		void whoQueryResult(int id, QStringList result);
	private slots:
		void socketConnected();
		void readyRead();
		void socketError(QAbstractSocket::SocketError socketError);
	private:
		QString popResponse();
		void handleRawResponse(QString r);
		void handleMessage(QString r);
		void sendNickname();
		void sendUser();
		void sendPong();
		void handleServerMessage(QString r);
		void handlePrivateMessage(const QString &sender, QString r);
		void handleMode(const QString &sender, QString m);
		void handleJoin(const QString &sender, QString m);
		void handleQuit(const QString &sender, QString m);
		void handleLeave(const QString &sender, QString m);
		void handleWhoReply(const QString &reply);
		void handleEndOfWho();

		QTcpSocket mSocket;
		QStringList mResponseBuffer;
		State mState;
		QString mNickname;
		QString mUsername;
		QString mHostName;
		QString mServerName;
		QString mRealName;
		QString mIRCServerAddress;
		QQueue<int> mWhoQueryQueue;
		int mWhoQueryQueueIdCounter;
		QStringList mWhoQueryResult;

		int mHandshakeCounter;
};

#endif // IRCSOCKET_H
