#include <stdlib.h>

#include "friendList.h"
#include "serialF0.h"

#define INITIAL_FRIEND_LIST_LENGTH 8

uint8_t friendListLength = INITIAL_FRIEND_LIST_LENGTH;
uint8_t friendAmount = 0;
friend_t *friends;

static void removeFriend(friend_t *friend);


void initFriendList() {
    friends = (friend_t *) calloc(INITIAL_FRIEND_LIST_LENGTH, sizeof(friend_t));
}

uint8_t addFriend(friend_t friend) {

    // Check if we already know this friend by a shorter route.
    friend_t *friendInList = findFriend(friend.id);
    if(friendInList != NULL && friendInList->hops <= friend.hops) {
        if(friendInList->hops == friend.hops) friendInList->remainingTime = friend.remainingTime;
        return 0;
    }

    // If the list not long enough for our vast amount of friends.
    if(friendListLength == friendAmount) {
        // Resize the list to add 8 bytes.
        friends = (friend_t *) realloc(friends, friendListLength + INITIAL_FRIEND_LIST_LENGTH);
        friendListLength += 8;
    }

    // Add the friend to the nearest hole in the list.
    friend_t *friendPtr = friends;
    while(friendPtr->id != 0) friendPtr++;
    *friendPtr = friend;
    friendAmount++;
    return 1;
}

void removeFriend(friend_t *friend) {
    friend->id = 0;
    friendAmount--;
}

void printFriends() {
    if(friendAmount == 0) {
        printf("I don't have any friends :(\n");
        return;
    }

    printf("My %d friends :)\n", friendAmount);
    printf("\e[0;31m       ID  Time  Hops  Via\e[0m\n");
    friend_t *friend = friends;
    for(uint8_t i = 0; i < friendAmount;) {
        if(friend->id != 0) {
            printf("  %3d  \e[0;35m0x%02x\e[0m  %1d  %02d  0x%02x\n", friend - friends, friend->id, friend->remainingTime, friend->hops, friend->via);
            i++;
        }
        friend++;
    }
    printf("\n");
}

void friendTimeTick() {
    for(uint8_t i = 0; i < friendListLength; i++) {
        if(friends[i].id != 0) {
            friends[i].remainingTime--;
            if(friends[i].remainingTime == 0)
                removeFriend(friends + i);
        }
    }
}

void printDebugFriends(void) {

    for(uint8_t i = 0; i < friendListLength; i++) {
        printf("%02x %02x,   ", friends[i].id, friends[i].remainingTime);
    }
    printf("\n");

}

// Find a friend, and return a pointer to that friend.
friend_t *findFriend(uint8_t id) {
    for(uint8_t i = 0; i < friendListLength; i++) {
        if(friends[i].id == id) return friends + i;
    }
    // If the friend was not found :(
    return NULL;
}
