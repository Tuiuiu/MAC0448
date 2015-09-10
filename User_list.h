#ifndef _USER_LIST_H
#define _USER_LIST_H

/* Internal header */
#include "User.h"

/* Macros */
#define MAX_CHANNEL_NAME_SIZE 20

/* Typedefs */
typedef struct user_list *User_list;

struct user_list {
	User      user;
	User_list next;
}; 

/* Prototypes */
User_list list_init();

User_list insert_user(User_list list, User usr);

int number_of_users (User_list list);

void users_list_to_string (User_list list, char* string); /* string (que precisa já estar alocada) terá os nicks dos usuários separados por espaços */
void remove_user(User_list list, char user_nick[MAX_NICK_SIZE]);

#endif /* _USER_LIST_H */