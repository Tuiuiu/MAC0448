#include <stdlib.h>
#include <string.h>

#include "users_list.h"

User *list_init() 
{
	int i;

	User *ini;
	ini = malloc(sizeof (User));
	ini->connfd = -1;
	for (i = 0; i < NUMBER_OF_CHANNELS; i++)
		ini->is_in_channel[i] = false;
	ini->id = -1;
	ini->next = NULL;
	return ini;
}

User *create(User *list, int usr_connfd, int usr_channel, int usr_id, nick usr_nickname)
{
	User *new_user;
	new_user = malloc(sizeof (User));
	new_user->connfd = usr_connfd;
	new_user->id = usr_id;
	strcpy(new_user->nickname, usr_nickname);
	new_user->next = list->next;
	list->next = new_user;
	return new_user;
}

void remove_user(User *previous_cel)
{
   User *to_be_removed;
   to_be_removed = previous_cel->next;
   previous_cel->next = to_be_removed->next;
   /* free(to_be_removed.nickname); */
   free(to_be_removed);
}

