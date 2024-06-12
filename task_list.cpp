#include "task_list.h"
#include "log.h"
#include <Arduino.h>

/**
 * basically setTimeout() lol
 * Im proud of this, even though i basically
 * stopped using it
 */

boolean TaskList::add(task_entry* entry) {
  if (this->size() >= TASK_LIST_SIZE) {
    logg(ERROR, "Failed to add task to task list (maximum size reached)");
    return false;
  }

  this->list[this->length] = entry;
  this->length++;

  logg(TRACE, "Task added to Task List");
  
  return true;
}

boolean TaskList::queueTask(void (*func)(void*), void* data, unsigned long timeout) {
  task_entry* entry = (task_entry*) malloc(sizeof(task_entry));
  entry->funca = func;
  entry->data = data;
  entry->timeout = millis() + timeout;

  add(entry);

  return true;
}

boolean TaskList::queueTask(void (*func)(), unsigned long timeout) {
  task_entry* entry = (task_entry*) malloc(sizeof(task_entry));
  entry->func = func;
  entry->data = NULL;
  entry->timeout = millis() + timeout;

  add(entry);

  return true;
}

boolean TaskList::remove(byte index) {
  if (index >= this->length) {
    return false;
  }
  
  task_entry* ref = this->list[index];

  ref->func = NULL;

  if (ref->data != NULL) {
    free(ref->data);
    ref->data = NULL;
  }

  free(ref);
  
  for (byte i = index + 1; i < this->length; i++) {
    this->list[i - 1] = this->list[i];
  }

  this->length--;
  this->list[this->length] = NULL;

  return true;
}

byte TaskList::size() {
  return this->length;
}

void TaskList::callTasks() {
  logg(TRACE, "Task List being called");
  for (byte i = 0; i < this->length; i++) {
    task_entry* curr = this->list[i];
    if (millis() >= curr->timeout) {
      if (curr->data != NULL) {
	curr->funca(curr->data);
      } else {
	curr->func();
      }
      this->remove(i);
      i--;
    }
  }
  logg(TRACE, "Task List calling completed");
}
