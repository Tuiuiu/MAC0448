#include <stdbool.h>

#define MAX_NICK_SIZE 50

#define NUMBER_OF_CHANNELS 2

typedef char nick[MAX_NICK_SIZE];

typedef struct cel {
	int			connfd;
	bool		is_in_channel[NUMBER_OF_CHANNELS]; /* is_in_channel[i] é true se o usuário estiver no canal i */
	int 		id;
	nick 		nickname;
	struct cel 	*next;
} User; 


User *list_init();

User *create(User *list, int usr_connfd, int usr_channel, int usr_id, nick usr_nickname);

void remove_user(User *previous_cel);
