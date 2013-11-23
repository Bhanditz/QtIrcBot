#include "ircsocket.h"
#include <QString>
#include <QDebug>
#include <QDateTime>

IRCSocket::IRCSocket() :
	mState(NotConnected),
	mWhoQueryQueueIdCounter(0) {
	connect(&mSocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &IRCSocket::socketError);
	connect(&mSocket, &QAbstractSocket::connected, this, &IRCSocket::socketConnected);
	connect(&mSocket, &QIODevice::readyRead, this, &IRCSocket::readyRead);
}

void IRCSocket::connectToServer(const QString &address, quint16 port, const QString &nick) {
	mSocket.connectToHost(address, port);
	mIRCServerAddress = address;
	mNickname = nick;
	mUsername = "IRCSocket" + QString::number(qHash(mNickname));
	mHostName = "IRCSocketDefaultHostName";
	mServerName = "IRCSocketDefaultServerName";
	mRealName = "Qt IRCSocket";
}

bool IRCSocket::sendData(const QString &data) {
	qDebug() << " > " << data;
	QByteArray msg = data.toUtf8() + "\r\n";
	if (msg.length() != mSocket.write(msg)) {
		qDebug() << "Can't send data!!!";
		return false;
	}
	return true;
}

bool IRCSocket::sendPrivateMessage(const QString &channel, const QString &msg) {
	return sendData(" PRIVMSG " + channel + " :" + msg);
}

void IRCSocket::joinChannel(QString channel, QString password) {
	if (!channel.startsWith('#')) channel = '#' + channel;
	sendData("JOIN " + channel + ((password.isEmpty())? QString() : (" " + password)));
}

void IRCSocket::leave(QString channel, QString message) {
	if (!channel.startsWith('#')) channel = '#' + channel;
	sendData("PART " + channel + " :" + message);
}


void IRCSocket::socketConnected() {
	mState = HandshakeInProgress;
	mHandshakeCounter = 0;
	emit connected();
}

void IRCSocket::readyRead() {
	QByteArray data = mSocket.readAll();
	qDebug() << " <RAW> " << data;
	QStringList newResponses = QString::fromUtf8(data).split("\r\n", QString::SkipEmptyParts);
	for (auto r : newResponses) handleRawResponse(r);
}

void IRCSocket::socketError(QAbstractSocket::SocketError socketError) {
	qDebug() << "Socket error " << socketError;
	emit error(socketError);
}

QString IRCSocket::popResponse() {
	if (mResponseBuffer.empty()) return QString();
	QString last = mResponseBuffer.last();
	mResponseBuffer.removeLast();
	return last;
}

void IRCSocket::handleRawResponse(QString r) {
	//qDebug() << " < " << r;
	if (r.startsWith("PING")) {
		sendPong();
		return;
	}
	switch(mState) {
		case HandshakeInProgress:
			mHandshakeCounter++;
			if (mHandshakeCounter == 1) {
				sendNickname();
				sendUser();
			}
			if (r.contains(" 437")) {
				mNickname += "_";
				sendNickname();
			}

			if (r.contains(" 376")) {
				mState = Connected;
				emit handshakeComplete();
			}
			return;
		case Connected:
			if (r.startsWith(':')) {
				handleMessage(r.remove(0, 1));
			}
			break;
		default:
			qDebug() << "WTF";
	}
}

void IRCSocket::handleMessage(QString r) {
	int senderEndIndex = r.indexOf(' ');
	if (senderEndIndex == -1) { qDebug() << "Invalid message"; return; }
	QString sender = r.left(senderEndIndex);

	r.remove(0, senderEndIndex + 1);

	if (r.startsWith("PRIVMSG")) {
		handlePrivateMessage(sender, r.remove(0, 8)); return;
	}
	if (r.startsWith("MODE")) {
		handleMode(sender, r.remove(0, 5)); return;
	}
	if (r.startsWith("JOIN")) {
		handleJoin(sender, r.remove(0, 5)); return;
	}
	if (r.startsWith("QUIT")) {
		handleQuit(sender, r.remove(0, 5)); return;
	}
	if (r.startsWith("PART")) {
		handleLeave(sender, r.remove(0, 5)); return;
	}

	if (!sender.contains('!')) {
		handleServerMessage(r);
		return;
	}

}

void IRCSocket::sendNickname() {
	sendData("NICK " + mNickname);
}

void IRCSocket::sendUser() {
	sendData("USER " + mUsername + ' ' + mHostName + ' ' + mServerName + " :" + mRealName);
}

void IRCSocket::sendPong() {
	sendData("PONG " + QString::number(QDateTime::currentDateTime().toTime_t()));
}

void IRCSocket::handleServerMessage(QString r) {
	if (r[0].isDigit()) {
		if (r.startsWith("352")) {
			r.remove(0, 4);
			handleWhoReply(r);
		}
		if (r.startsWith("315")) {
			r.remove(0, 4);
			handleEndOfWho();
		}
	}
}

void IRCSocket::handlePrivateMessage(const QString &sender, QString r) {
	int channelEndIndex = r.indexOf(':');
	if (channelEndIndex == -1) { qDebug() << "Invalid PRIVMSG"; return; }
	QString channel = r.left(channelEndIndex).trimmed();
	r.remove(0, channelEndIndex + 1);

	emit privateMessage(sender, channel, r);
}

void IRCSocket::handleMode(const QString &sender, QString m) {
	if (!m.startsWith('#')) {
		emit userMode(sender, m);
		return;
	}

	int targetEndIndex = m.indexOf(' ');
	if (targetEndIndex == -1) { qDebug() << "Invalid MODE"; return; }
	QString target = m.left(targetEndIndex);
	m.remove(0, targetEndIndex + 1);

	int flagsEndIndex = m.indexOf(' ');
	if (flagsEndIndex == -1) { qDebug() << "Invalid MODE"; return; }
	QString flags = m.left(flagsEndIndex);
	m.remove(0, flagsEndIndex + 1);

	emit channelMode(sender, target, flags, m);
}

void IRCSocket::handleJoin(const QString &sender, QString m) {
	int channelStart = m.indexOf(':');
	if (channelStart == -1) { qDebug() << "Invalid join"; return; }
	m.remove(0, channelStart + 1);

	emit userJoin(sender, m);
}

void IRCSocket::handleQuit(const QString &sender, QString m) {
	int messageStart = m.indexOf(':');
	if (messageStart == -1) { qDebug() << "Invalid join"; return; }
	m.remove(0, messageStart + 1);

	emit userQuit(sender, m);
}

void IRCSocket::handleLeave(const QString &sender, QString m) {
	int channelEnd = m.indexOf(':');
	if (channelEnd == -1) { qDebug() << "Invalid join"; return; }

	QString channel = m.left(channelEnd).trimmed();
	m.remove(0, channelEnd + 1);

	emit userLeave(sender, channel, m);
}

void IRCSocket::handleWhoReply(const QString &reply) {
	mWhoQueryResult.append(reply);
}

void IRCSocket::handleEndOfWho() {
	emit whoQueryResult(mWhoQueryQueue.first(), mWhoQueryResult);
	mWhoQueryResult.clear();
	mWhoQueryQueue.removeFirst();
}


int IRCSocket::whoQuery(const QString &queryString) {
	mWhoQueryQueue.append(++mWhoQueryQueueIdCounter);
	sendData("WHO " + queryString);
	return mWhoQueryQueueIdCounter;
}


void IRCSocket::quit(QString message) {
	sendData("QUIT :" + message);
	mSocket.close();
	mState = NotConnected;
}




