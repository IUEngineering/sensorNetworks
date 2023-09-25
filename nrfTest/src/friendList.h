#ifndef _FRIENDLIST_H_
#define _FRIENDLIST_H_

#include <avr/io.h>

#define FORGET_FRIEND_TIME 8

// Define neighbor datatype
typedef struct {
    uint8_t id;
    uint8_t remainingTime;
    uint8_t hops;
    uint8_t via;
} friend_t;

void initFriendList(void);
uint8_t addFriend(uint8_t id, uint8_t hops, uint8_t via);
void printFriends(void);
void friendTimeTick(void);
void printDebugFriends(void);
friend_t *findFriend(uint8_t id);
friend_t *getFriendsList(uint8_t *listLength);


#endif // _FRIENDLIST_H_