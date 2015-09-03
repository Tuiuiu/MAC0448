#ifndef _USERS_LIST_H
#define _USERS_LIST_H


#define MAX_NICK_SIZE 50

typedef char nick[MAX_NICK_SIZE];

typedef struct cel {
	int			connfd;
	int 		channel;
	int 		id;
	nick 		nickname;
	struct cel 	*next;
} User; 


User *list_init();

User *create(User *list, int usr_connfd, int usr_channel, int usr_id, nick usr_nickname);

void remove_user(User *previous_cel);


#endif /* _USERS_LIST_H */