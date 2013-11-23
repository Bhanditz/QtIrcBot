#include <QCoreApplication>
#include "ircsocket.h"
#include "quotebot.h"

int main(int argc, char *argv[]) {
	QCoreApplication a(argc, argv);
	
	IRCSocket irc;
	QuoteBot bot;
	bot.setQuoteFile("quotes.txt");
	bot.loadQuotes();
	bot.init(&irc);
	irc.connectToServer("irc.cc.tut.fi", 6667, "LQB");
	QObject::connect(&irc, &IRCSocket::handshakeComplete, [&irc]() {
		irc.joinChannel("#testatat");
	});
	return a.exec();
}
