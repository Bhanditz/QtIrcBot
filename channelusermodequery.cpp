#include "channelusermodequery.h"

ChannelUserModeQuery::ChannelUserModeQuery(QObject *parent) :
	QObject(parent)
{
}

void ChannelUserModeQuery::init(IRCSocket *irc) {
	mIRC = irc;
	connect(irc, &IRCSocket::userJoin, this, &ChannelUserModeQuery::userJoin);
	connect(irc, &IRCSocket::channelMode, this, &ChannelUserModeQuery::channelMode);
	connect(irc, &IRCSocket::whoQueryResult, this, &ChannelUserModeQuery::whoQueryResult);
}


QString ChannelUserModeQuery::parseUsername(const QString &username) {
	QStringList parts = username.split('!');
	return parts.first();
}

bool ChannelUserModeQuery::isOp(QString channel, QString user) {
	return (query(channel, user) & Op) == Op;
}

bool ChannelUserModeQuery::hasVoice(QString channel, QString user) {
	UserMode um = query(channel, user);
	if (um == Voice) return true;
	if (um == Op) return true;
	return false;
}

ChannelUserModeQuery::UserMode ChannelUserModeQuery::query(QString channel, QString user) {
	if (channel.startsWith('#')) channel.remove(0, 1);
	auto it1 = mChannels.find(channel);
	if (it1 == mChannels.end()) return Unknown;
	QString userName = parseUsername(user);
	auto it2 = it1.value().mUsers.find(userName);
	if (it2 == it1.value().mUsers.end()) return Unknown;
	return (UserMode)it2.value();
}

void ChannelUserModeQuery::userJoin(QString user, QString channel) {
	if (channel.startsWith('#')) channel.remove(0, 1);
	if (parseUsername(user) == mIRC->nickname()) {
		mIRC->whoQuery("#" + channel);
	}
	mChannels[channel].mUsers[user] = Normal;
}

void ChannelUserModeQuery::channelMode(QString sender, QString channel, QString flags, QString params) {
	if (channel.startsWith('#')) channel.remove(0, 1);
	QString::ConstIterator it = flags.begin();
	QString::ConstIterator pi = params.begin();
	bool add = false;
	while (it != flags.end()) {
		if (*it == '+') {
			add = true;
		} else if (*it == '-') {
			add = false;
		} else if (*it == 'o') {
			if (add)
				mChannels[channel].mUsers[parseUsername(*pi)] |= Op;
			else
				mChannels[channel].mUsers[parseUsername(*pi)] &= !Op;
		} else if (*it == 'v') {
			if (add)
				mChannels[channel].mUsers[parseUsername(*pi)] |= Voice;
			else
				mChannels[channel].mUsers[parseUsername(*pi)] &= !Voice;
		}
	}
}


void ChannelUserModeQuery::whoQueryResult(int id, QStringList result) {
	for (QString r : result) {
		QStringList s = r.split(' ', QString::SkipEmptyParts);
		QString c = s[1].remove(0, 1); //remove #
		QString n = s[5];
		QString a = s[6];
		if (!a.startsWith('H')) {
			qDebug() << "Invalid WHO query result" << r;
			continue;
		}
		a.remove(0, 1);
		if (a == "+") {
			mChannels[c].mUsers[n] = Voice;
		} else if (a == "@") {
			mChannels[c].mUsers[n] = Op;
		}
	}
}
