/* Round Robin Scheduler */

#include <stdio.h>
#include "lwp.h"
#include <assert.h>

Node* head = NULL;
Node* tail = NULL;

void rr_init(void){
    /* Initilializeds the scheduler */
    head = tail = NULL;
}

void rr_shutdown(void){
    /* Frees all nodes in the queue */
    while (head != NULL){
        Node* temp = head;
        head = head->next;
        free(temp);
    }
    tail = NULL;
}

void rr_admit(thread new_thread){
    /* adds new thread to end of queue */
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Unable to allocate memory for new thread");
        return;
    }
    new_node->data = new_thread;
    new_node->next = NULL;

    if (tail == NULL) {
        head = tail = new_node;
    } else {
        tail->next = new_node;
        tail = new_node;
    }
}

void rr_remove_thread(thread victim){
    /* removes vicitm thread from the queue */
    Node* temp = head;
    Node* prev = NULL;

    while (temp != NULL && temp->data != victim) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) return; /* Didn't find the victim */

    if (temp == head){
        head = head->next;
        if (head == NULL) tail = NULL;
    } else {
        prev->next = temp->next;
        if (temp == tail) tail = prev;
    }

    free(temp);

}

thread rr_next(){
    /* retrieves next thread and the moves it to the end of the queue*/
    if (head == NULL) return -1; /* Indicate that no thread is available */

    thread next_thread = head->data;
    /* Move the head to the end of the queue to achieve round-robin behavior */
    Node* temp = head;
    head = head->next;
    if (head == NULL) {
        tail = NULL;
    } else {
        tail->next = temp;
        tail = temp;
        tail->next = NULL;
    }

    return next_thread;
}

int rr_qlen(){
    /* returns num threads in queue */
    int length = 0;
    Node* current = head;
    while (current != NULL) {
        length++;
        current = current->next;
    }
    return length;
}

/*
void test_init_shutdown() {
    rr_init();
    assert(head == NULL && tail == NULL);
    rr_shutdown();
    assert(head == NULL && tail == NULL);
}

void test_admit_remove() {
    rr_init();
    rr_admit(1);
    rr_admit(2);
    assert(rr_qlen() == 2);
    rr_remove_thread(1);
    assert(rr_qlen() == 1);
    rr_remove_thread(2);
    assert(rr_qlen() == 0);
    rr_shutdown();
}

void test_next() {
    rr_init();
    rr_admit(1);
    rr_admit(2);
    assert(rr_next() == 1);
    assert(rr_next() == 2);
    assert(rr_next() == 1); // Should cycle back to the start
    rr_shutdown();
}

void test_qlen() {
    rr_init();
    assert(rr_qlen() == 0);
    rr_admit(1);
    rr_admit(2);
    assert(rr_qlen() == 2);
    rr_shutdown();
}

int main() {
    test_init_shutdown();
    test_admit_remove();
    test_next();
    test_qlen();

    printf("All tests passed.\n");
    return 0;
}
*/