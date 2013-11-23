#ifndef QUOTEBOT_H
#define QUOTEBOT_H

#include <QObject>
#include <QTimer>
#include "ircsocket.h"
#include "channelusermodequery.h"

class QuoteBot : public QObject {
		Q_OBJECT
	public:
		explicit QuoteBot(QObject *parent = 0);
		void init(IRCSocket *irc);
		void setQuoteFile(QString fileName);
	signals:
		
	public slots:
		void privateMessage(QString sender, QString channel, QString msg);
		void loadQuotes();
		void saveQuotes();
	private:
		struct Channel {
			QMap<QString, QString> mLastMessages;
			QStringList mQuotes;
		};

		void handleChannelCommand(const QString &channel, const QString &user, const QString &msg);
		void handleUserCommand(const QString &user, const QString &command); //query
		bool handleAdminCommand(const QString &replyChan, QString command);

		IRCSocket *mIRC;
		ChannelUserModeQuery mUserQuery;
		QStringList mQuotes;
		QMap<QString, Channel> mChannels;
		QString mQuoteFile;

		QTimer *mSaveTimer;
		bool mNewQuotes;
};

#endif // QUOTEBOT_H
