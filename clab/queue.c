/* 
 * Code for basic C skills diagnostic.
 * Developed for courses 15-213/18-213/15-513 by R. E. Bryant, 2017
 * Modified to store strings, 2018
 */

/*
 * This program implements a queue supporting both FIFO and LIFO
 * operations.
 *
 * It uses a singly-linked list to represent the set of queue elements
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/*
  Create empty queue.
  Return NULL if could not allocate space.
*/
queue_t *q_new()
{
    queue_t *q = malloc(sizeof(queue_t));
    /* What if malloc returned NULL? */
    if (q == NULL)  // handle NULL
        return q;
    q->head = NULL;
    q->len = 0;
    return q;
}

/* Free all storage used by queue */
void q_free(queue_t *q)
{
    /* How about freeing the list elements and the strings? */
    /* Free queue structure */
    if (q == NULL)
        return;
    list_ele_t *tmp;
    while (q->head != NULL) {
        tmp = q->head;
        q->head = q->head->next;
        free(tmp->value);
        free(tmp);
    }
    free(q);
}

/*
  Attempt to insert element at head of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(queue_t *q, char *s)
{
    list_ele_t *newh;
    char *news;
    /* What should you do if the q is NULL? */
    if (q == NULL)
        return false;
    /* Don't forget to allocate space for the string and copy it */
    /* What if either call to malloc returns NULL? */        
    if ((newh = malloc(sizeof(list_ele_t))) == NULL)
        return false;
    // Note must +1!
    if ((news = malloc(strlen(s) + 1)) == NULL){
        free(newh);
        return false;
    }
    if (strcpy(news, s) == NULL)
        return false;
    newh->value = news;
    newh->next = q->head;
    q->head = newh;
    (q->len)++;
    if (q->head->next == NULL)
        q->tail = q->head;
    return true;
}


/*
  Attempt to insert element at tail of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(queue_t *q, char *s)
{
    /* You need to write the complete code for this function */
    /* Remember: It should operate in O(1) time */
    if (q == NULL)
        return false;
    list_ele_t *newt;
    char *news;
    if ((newt = malloc(sizeof(list_ele_t))) == NULL)
        return false;
    // Note must +1!
    if ((news = malloc(strlen(s) + 1)) == NULL) {
        free(newt);
        return false;
    }
    if (strcpy(news, s) == NULL)
        return false;
    newt->value = news;
    newt->next = NULL;
    if (q->tail == NULL) {
        q->tail = newt;
        q->head = newt;
    }
    else {
        q->tail->next = newt;
        q->tail = newt;
    }
    (q->len)++;
    return true;
}

/*
  Attempt to remove element from head of queue.
  Return true if successful.
  Return false if queue is NULL or empty.
  If sp is non-NULL and an element is removed, copy the removed string to *sp
  (up to a maximum of bufsize-1 characters, plus a null terminator.)
  The space used by the list element and the string should be freed.
*/
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
    /* You need to fix up this code. */
    if (q == NULL || q->head == NULL)
        return false;
    list_ele_t *tmp = q->head;
    q->head = q->head->next;

    if (sp != NULL){
        strncpy(sp, tmp->value, bufsize - 1);
        *(sp + bufsize - 1) = '\0';
    }
    free(tmp->value);
    free(tmp);
    (q->len)--; 
    if (q->head == NULL)
        q->tail = NULL;
    return true;
}

/*
  Return number of elements in queue.
  Return 0 if q is NULL or empty
 */
int q_size(queue_t *q)
{
    /* You need to write the code for this function */
    /* Remember: It should operate in O(1) time */
    return q != NULL ? q->len : 0;
}

/*
  Reverse elements in queue
  No effect if q is NULL or empty
  This function should not allocate or free any list elements
  (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
  It should rearrange the existing ones.
 */
void q_reverse(queue_t *q)
{
    /* You need to write the code for this function */
    if (q == NULL || q->head == NULL)
        return;
    q->tail = q->head;
    while (q->tail->next != NULL) {
        list_ele_t *tmp = q->tail->next;
        q->tail->next = tmp->next;
        tmp->next = q->head;
        q->head = tmp;
    }
}

