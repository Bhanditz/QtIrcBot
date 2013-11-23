#ifndef CHANNELUSERMODEQUERY_H
#define CHANNELUSERMODEQUERY_H

#include <QObject>
#include <QMap>
#include "ircsocket.h"

class ChannelUserModeQuery : public QObject {
		Q_OBJECT
	public:
		enum UserMode {
			Normal = 1,
			Voice = 2,
			Op = 4,
			Unknown = 0
		};

		explicit ChannelUserModeQuery(QObject *parent = 0);
		void init(IRCSocket *irc);
		static QString parseUsername(const QString &username);

		bool isOp(QString channel, QString user);
		bool hasVoice(QString channel, QString user);
		UserMode query(QString channel, QString user);

	private slots:
		void userJoin(QString user, QString channel);
		void channelMode(QString sender, QString channel, QString flags, QString params);
		void whoQueryResult(int id, QStringList result);


	private:
		struct Channel {
			QMap<QString,int> mUsers;
		};

		IRCSocket *mIRC;
		QMap<QString, Channel> mChannels;
};

#endif // CHANNELUSERMODEQUERY_H
