#include "quotebot.h"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
QuoteBot::QuoteBot(QObject *parent) :
	QObject(parent) {
	mSaveTimer = new QTimer();
	connect(mSaveTimer, &QTimer::timeout, this, &QuoteBot::saveQuotes);
	mSaveTimer->start(1000 * 60 * 5); //5min
}

void QuoteBot::init(IRCSocket *irc) {
	mIRC = irc;
	connect(irc, &IRCSocket::privateMessage, this, &QuoteBot::privateMessage);
	mUserQuery.init(irc);
}

void QuoteBot::setQuoteFile(QString fileName) {
	mQuoteFile = fileName;
	mNewQuotes = true;
}

void QuoteBot::loadQuotes() {
	QFile file(mQuoteFile);
	if (!file.open(QFile::ReadOnly)) {
		qCritical("Can't open load file");
		return;
	}
	QTextStream in(&file);
	QString channel;
	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (line.startsWith('[') && line.endsWith(']')) {
			channel = line.mid(1, line.length() - 2);
			mChannels[channel].mQuotes.clear();
		}
		else {
			mChannels[channel].mQuotes.append(line);
		}
	}
	mNewQuotes = false;
}

void QuoteBot::saveQuotes() {
	if (!mNewQuotes) return;
	QFile file(mQuoteFile);
	if (!file.open(QFile::WriteOnly)) {
		qCritical("Can't open save file");
		return;
	}
	QTextStream out(&file);
	for (auto i = mChannels.cbegin(); i != mChannels.cend(); i++) {
		out << '[' << i.key() << "]\n";
		for (auto i2 = i.value().mQuotes.cbegin(); i2 != i.value().mQuotes.cend(); i2++) {
			out << *i2 << '\n';
		}
	}
	mNewQuotes = false;
	qDebug() << "All quotes saved";
}

void QuoteBot::privateMessage(QString sender, QString channel, QString msg) {
	if (msg.startsWith('!')) {
		msg.remove(0, 1);
		if (channel.startsWith("#")) { //channel
			handleChannelCommand(channel, sender, msg);
		}
		else {
			handleUserCommand(sender, msg);
		}
	}
	else {
		mChannels[channel].mLastMessages[ChannelUserModeQuery::parseUsername(sender)] = msg;
	}
}


void QuoteBot::handleChannelCommand(const QString &channel, const QString &user, const QString &msg) {
	QStringList parts = msg.split(' ', QString::SkipEmptyParts);
	QString command = parts.first();
	if (command == "grab") {
		if (!mUserQuery.hasVoice(channel, user) && !user.endsWith("!latexi95@kapsi.fi")) {
			mIRC->sendPrivateMessage(channel, ChannelUserModeQuery::parseUsername(user) + ": You need +v to access this command");
			return;
		}
		if (parts.size() != 2) {
			mIRC->sendPrivateMessage(channel, ChannelUserModeQuery::parseUsername(user) + ": Usage:  !grab nick");
			return;
		}

		auto it = mChannels[channel].mLastMessages.find(parts[1]);
		if (it == mChannels[channel].mLastMessages.end()) {
			mIRC->sendPrivateMessage(channel, ChannelUserModeQuery::parseUsername(user) + ": No messages of \"" + parts[1] + "\" (case-sensitive) in the logs of this channel");
			return;
		}
		else {
			mChannels[channel].mQuotes.append("<" + it.key() + "> " + it.value());
			mIRC->sendPrivateMessage(channel, ChannelUserModeQuery::parseUsername(user) + ": A quote from \"" + parts[1] + "\" saved");
			mNewQuotes = true;
		}
	}
	else if (command == "lastquote" || command == "lq") {
		if (mChannels[channel].mQuotes.empty()) {
			mIRC->sendPrivateMessage(channel, ChannelUserModeQuery::parseUsername(user) + ": No quotes");
			return;
		}
		mIRC->sendPrivateMessage(channel, "\"" + mChannels[channel].mQuotes.last() + "\"");
	} else if (command == "random" || command == "q" || command == "quote") {
		int quoteCount = mChannels[channel].mQuotes.size();
		if (quoteCount == 0) {
			mIRC->sendPrivateMessage(channel, ChannelUserModeQuery::parseUsername(user) + ": No quotes");
			return;
		}
		int quoteIndex = abs((qrand() * qrand() * qrand() - qrand()) % quoteCount);
		QString quote = mChannels[channel].mQuotes[quoteIndex];
		mIRC->sendPrivateMessage(channel, "\"" + quote + "\"");
	}
	else if (command == "leave") {
		if (!mUserQuery.isOp(channel, user) && !user.endsWith("!latexi95@kapsi.fi")) {
			mIRC->sendPrivateMessage(channel, ChannelUserModeQuery::parseUsername(user) + ": You need +o to access this command");
			return;
		}
		mIRC->leave(channel, "Leave :(");
	}
	else if (user.endsWith("!latexi95@kapsi.fi")) { //Special admin commands
		if (!handleAdminCommand(channel, msg)) {
			mIRC->sendPrivateMessage(channel, "Invalid command");
		}
	} else {
		mIRC->sendPrivateMessage(channel, "Invalid command");
	}
}

void QuoteBot::handleUserCommand(const QString &user, const QString &command) {
	if (user.endsWith("!latexi95@kapsi.fi") && handleAdminCommand(user, command)) {
	} else {
		mIRC->sendPrivateMessage(user, "Invalid command");
	}

}

bool QuoteBot::handleAdminCommand(const QString &replyChan, QString command) {
	if (command == "quit") {
		mIRC->quit("Quit :(");
		saveQuotes();
		return true;
	} else if (command == "save") {
		saveQuotes();
		mIRC->sendPrivateMessage(replyChan, "All quotes saved");
		return true;
	} else if (command == "load") {
		loadQuotes();
		mIRC->sendPrivateMessage(replyChan, "All quotes loaded");
		return true;
	} else if (command.startsWith("join ")) {
		command.remove(0, 5);
		mIRC->joinChannel(command);
		return true;
	}
	else if (command.startsWith("leave ")) {
		command.remove(0, 6);
		mIRC->leave(command);
		return true;
	}
	return false;
}

