/* Standard headers */
#include <stdlib.h>
#include <string.h>

/* Internal headers */
#include "User.h"
#include "Channel_list.h"

User new_user(int usr_connfd,
              int usr_id,
              char usr_nickname[MAX_NICK_SIZE])
{
	User new_user;
	new_user = malloc(sizeof (*new_user));
	new_user->connfd = usr_connfd;
	new_user->id = usr_id;
	new_user->channels = chn_list_init();
	strcpy(new_user->nickname, usr_nickname);
	return new_user;
}

/*
void remove_user(User *user)
{
	free(user);
}
*/
