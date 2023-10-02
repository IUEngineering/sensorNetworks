#ifndef _FRIENDLIST_H_
#define _FRIENDLIST_H_

#include <avr/io.h>

#define FORGET_FRIEND_TIME 8

//* Define neighbor datatype
//* Holds al the necessairy information about a friend
typedef struct {
    uint8_t id;
    uint8_t hops;
    uint8_t via;
    uint8_t trust;
    uint8_t active;
}friend_t;


//* Initialise the friendlist
void initFriendList(void);


//* Add a friend to the friendlist or increase our trust in it.
//* id:    The friend id
//* hops:  The number of nodes between this node and the friend node
//* via:   The next node to reach the friend node
//*
//* Returns the updated friend.
friend_t *updateFriend(uint8_t id, uint8_t hops, uint8_t via);


//* Print a list of all friends to the terminal (serialF0)
void printFriends(void);


//* Lower the trust of all the friends with 1
//* Remove friends if their trust == 0
//* Deactivate friends if they are not trustworthy
void friendTimeTick(void);

//* Get a list of the direct friends
//* 
//* *dFriends: array to store n direct friends. If there are not enough direct friends a 0x00 will be added
//* length: Gives the length of the dFriends array
void getDirectFriends(uint8_t *dFriends, uint8_t length);
friend_t *getFriendsList(uint8_t *listLength);  // TODO: change with getDirectFriends(uint8_t *dFriends, uint8_t length);

//* Return a pointer to the requested friend id. Returns Null incase friend doesn't exist
friend_t *findFriend(uint8_t id);


#endif // _FRIENDLIST_H_