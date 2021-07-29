# Group-Messaging-System-using-Multicasting

The group messaging system uses the IP multicasting and broadcasting. It allows users in a UNIX
system to create groups, search groups, join groups , send message to groups, receive message
from groups . A user can also ask and search for filenames from other users in the same group. A
user can also create a poll using a special message in a group.

DESIGN

• There is no central server. All the communication is managed by the clients(nodes).

• The client manages all the groups in the LAN as well as the groups he/she is added in.

• For the system to work properly all the users should login before creating any groups.

• Maximum group limit is taken to be 100.

• All the users are connected by a global group. They are added when they login for the first
time.

• Maximum file size shared is 1000 characters.

• There are two threads running : one for receiving messages and another for main loop.

• The messages are send and decoded in this manner:
1. $type1:this means the message is of type1.
2. Followed by the group name.
3. Followed by ampersand sign and then the message.
4. Followed by ‘#’ sign and the sender’s name.
5. For example a message can be : $type3friends@hello#shrey
6. The message is decoded in the same order.

• The polling time is taken to be 20sec. The user should wait for 20 sec before giving any
commands.

• Before polling the users who didn’t initiate the poll have to press 0 to enable polling.

• At the end of 20 sec the polls are not recorded and hence not accounted.

• Users can receive file with name “received.txt” in the current working directory from other
users.
