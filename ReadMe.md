#summary ReadMe for pidgin-privacy-please
#labels Featured,Phase-Deploy

pidgin privacy please is a pidgin plugin to stop spammers from annoying you.

It offers the following features:

  * Block individual users
  * Auto-reply to blocked messages
  * Block messages from people who are not on your contact list (with an optional auto-reply)
  * Block messages using regular expressions, either against the message sender, the message content, or both
  * Suppress repeated/all authorization requests
  * Suppress OSCAR (ICQ/AIM) authorization requests
  * Automatically show user info on authorization requests
  * Block jabber headline messages (eg. alerts from the MSN transport)
  * Block AOL system messages
  * Challenge-response bot-check (see below)

## Auto-reply on blocked messages ##

You can have an auto-reply message sent to people whose messages have been
blocked to prevent accidentally blocked messages from disappearing unnoticed.


## Messages from unknown people ##

You can block any messages from people who are not on your contact list and
optionally have an auto-reply sent, telling them to request your authorization
first. Just check the corresponding options in the configuration dialog.


## Authorization requests ##

For protocols where the user name does not tell you anything about the actual
person behind it (eg. ICQ) there is also an option to have the user information
pop-up automatically shown whenever somebody asks for your authorization.

Or, if you already have all the friends you need, you can also have all
authorization requests rejected automatically (note that, depending on the
protocol, you might still be asked for a reason for rejecting).

## Bot check ##


To block spam-bots, you can have pidgin-privacy-please ask a simple question whenever somebody starts a conversation. If they send the right answer, they will be allowed to talk to you. Note that the right answer may be surrounded by other text, so if the right answer is "foo", and they write "i guess it's foo or bar" that's fine, so don't make the answer too short (eg. a simple character like 'a' is a bad choice).

Also, be aware that using this feature might cause all kinds of strange and devastating side-effects if you already use another plugin with similar functionality such as Bot Sentry.