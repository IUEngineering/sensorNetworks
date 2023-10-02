#include <stdlib.h>

#include "friendList.h"
#include "serialF0.h"

#define INITIAL_FRIEND_LIST_LENGTH 8

#define ACTIVATE_TRUST      7
#define DEACTIVATE_TRUST    5
#define MAX_TRUST           10
#define TRUST_ADDER         2   
#define TRUST_SUBTRACTOR    1

uint8_t friendListLength = INITIAL_FRIEND_LIST_LENGTH;
uint8_t friendAmount = 0;
friend_t *friends;

static void removeFriend(friend_t *friend);
static friend_t *newFriend(uint8_t id, uint8_t hops, uint8_t via);


void initFriendList() {
    friends = (friend_t *) calloc(INITIAL_FRIEND_LIST_LENGTH, sizeof(friend_t));
}


friend_t *updateFriend(uint8_t id, uint8_t hops, uint8_t via) {

    // Check if we already know this friend.
    friend_t *oldFriend = findFriend(id);

    // If we don't, create a new one.
    if(oldFriend == NULL) {
        return newFriend(id, hops, via);
    }
    
    // If it's a direct connection:
    if(hops == 0) {
        oldFriend->trust += TRUST_ADDER;

        // Make sure the friend doesn't get too trusted (we have trust issues).
        if(oldFriend->trust > MAX_TRUST) oldFriend->trust = MAX_TRUST;

        // Activate the friend when we trust it enough.
        if(oldFriend->trust > ACTIVATE_TRUST) oldFriend->active = 1;
    }
    // Replace old friend if it has more hops.
    else if(hops < oldFriend->hops) {
        oldFriend->via = via;
        oldFriend->hops = hops;
    }

    return oldFriend;
}

friend_t *newFriend(uint8_t id, uint8_t hops, uint8_t via) {
    friend_t *friendPtr;

    // If the list not long enough for our vast amount of friends:
    if(friendListLength == friendAmount) {
        // Resize the list to add 8 bytes.
        friends = (friend_t *) realloc(friends, friendListLength + INITIAL_FRIEND_LIST_LENGTH);
        friendListLength += INITIAL_FRIEND_LIST_LENGTH;
        friendPtr = friends + friendAmount;
        printf("Made the array bigger :)\n");
    }
    else {
        // Find the nearest hole in the list.
        friendPtr = friends;
        while(friendPtr->id != 0) friendPtr++;
    }

    friendPtr->id = id;
    friendPtr->via = via;
    friendPtr->hops = hops;
    friendPtr->active = 0;
    friendAmount++;

    // Make sure all directly connected friends get a trust boost.
    if(hops == 0) friendPtr->trust = TRUST_ADDER;
    else friendPtr->trust = 0;

    return friendPtr;
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
    printf("\t\e[0;31mID\tTrust\tActive\tHops\tVia\e[0m\n");

    for(uint8_t i = 0; i < friendListLength; i++) {
        if(friends[i].id != 0) {
            printf("%3d\t\e[0;35m0x%02x\e[0m\t%2d\t%1d\t%02d\t0x%02x\n", i, friends[i].id, friends[i].trust, friends[i].active, friends[i].hops, friends[i].via);
            i++;
        }
    }
    printf("\n");
}

void friendTimeTick() {
    for(uint8_t i = 0; i < friendListLength; i++) {
        if(friends[i].id != 0 && friends[i].trust > 0) {

            // Reduce trust.
            friends[i].trust -= TRUST_SUBTRACTOR;

            // Deactivate friend.
            if(friends[i].active && friends[i].trust < DEACTIVATE_TRUST) {
                friends[i].active = 0;

                // Remove all via references to this friend.
                removeViaReferences(friends[i].id);
            }

            // Remove friend if we have no way of reaching it anymore.
            if(friends[i].trust == 0  &&  friends[i].via == 0)
                removeFriend(friends + i);
        }
    }
}

void removeViaReferences(uint8_t id) {
    for(uint8_t i = 0; i < friendListLength; i++) {
        if(friends[i].id != 0  &&  friends[i].via == id) {

            // Only remove the friend if we have no direct connection to it.
            if(friends[i].trust > 0) {
                friends[i].hops = 0;
                friends[i].via = 0;
            }
            else removeFriend(friends + i);
        }
    }
}


friend_t *getFriendsList(uint8_t *listLength) {
    *listLength = friendListLength;
    return friends;
}

void getFriends(friend_t *buf) {
    uint8_t bufIndex = 0;
    for(uint8_t i = 0; bufIndex < friendAmount; i++) {
        if(friends[i].id != 0) {
            // Copy the friend into the buffer.
            buf[bufIndex] = friends[i];
            bufIndex++;
        }
    }
    buf[bufIndex].id = 0;
}


// Find a friend, and return a pointer to that friend.
friend_t *findFriend(uint8_t id) {
    // ID 0x00 is invalid.   
    if(id == 0x00) return NULL;

    for(uint8_t i = 0; i < friendListLength; i++) {
        if(friends[i].id == id) return friends + i;
    }
    // If the friend was not found :(
    return NULL;
}

