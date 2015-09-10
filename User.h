#ifndef _USER_DATA_H
#define _USER_DATA_H

/* Macros */
#define MAX_NICK_SIZE 50

/* Forward declaration */
struct channel_list;

/* Typedefs */
typedef struct user *User;

/* Structs */
struct user {
	int				     connfd;
	int 			     id;
	char 			     nickname[1 + MAX_NICK_SIZE];
	struct channel_list *channels;
};

User new_user(int usr_connfd,
              int usr_id,
              char usr_nickname[1 + MAX_NICK_SIZE]);

/* void remove_user(User *previous_cel); */

#endif /* _USER_DATA_H */