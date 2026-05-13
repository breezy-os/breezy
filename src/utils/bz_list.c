
#include "./breezy/bz_list.h"

#include <stdlib.h>

#include "./breezy/bz_logger.h"


/**
 * Creates and returns a new linked list. The caller is expected to call bz_list_free() when they
 * are finished with the list. Returns a nullptr if the list creation fails for any reason, such as
 * a memory allocation failure.
 */
struct bz_list *bz_list_create()
{
	struct bz_list *list = malloc(sizeof(*list));
	if (!list) {
		bz_error(BZ_LOG_LIST, __FILE__, __LINE__, "Failed to allocate memory creating list.");
		return nullptr;
	}
	list->head = nullptr;
	list->tail = nullptr;
	list->length = 0;
	return list;
}

/**
 * Adds the provided data to the end of the list, returning 0 on success or a negative value on
 * failure.
 */
int bz_list_append(struct bz_list *list, void *data)
{
	if (list == nullptr) {
		bz_error(BZ_LOG_LIST, __FILE__, __LINE__, "Append failed: list was not initialized.");
		return -1;
	}
	struct bz_node *new_node = malloc(sizeof(*new_node));
	if (!new_node) {
		bz_error(BZ_LOG_LIST, __FILE__, __LINE__,
			"Append failed: new node could not be allocated.");
		return -2;
	}
	new_node->data = data;
	new_node->next = nullptr;

	if (list->head == nullptr) {
		list->head = new_node;
		list->tail = new_node;
	} else {
		list->tail->next = new_node;
		list->tail = new_node;
	}
	list->length++;

	return 0;
}

/**
 * Inserts an item after the provided "after_data" in the list. If "after_data" is null, then
 * the new item is inserted at the beginning. If "after_data" cannot be found in the list, the new
 * item is not inserted, and a negative return value is given.
 *
 * A return value of 0 indicates success, and a negative value indicates failure.
 *
 * Return values:
 *   0: Insertion was successful!
 *   -1: Provided list was not initialized
 *   -2: Failed to find "after_data" in the list
 *   -3,-4: Not enough memory to allocate new list node
 */
int bz_list_insert(struct bz_list *list, void *data, void *after_data)
{
	if (list == nullptr) {
		bz_error(BZ_LOG_LIST, __FILE__, __LINE__, "Insertion failed: list was not initialized.");
		return -1;
	}

	// Insert at the beginning if "after_data" is null
	if (after_data == nullptr) {
		// Allocate a new node
		struct bz_node *new_node = malloc(sizeof(*new_node));
		if (!new_node) {
			bz_error(BZ_LOG_LIST, __FILE__, __LINE__,
				"Insertion failed: new node could not be allocated.");
			return -3;
		}
		new_node->data = data;
		new_node->next = nullptr;

		// Update our pointers
		if (list->head == nullptr) {
			// It's an empty list
			list->head = new_node;
			list->tail = new_node;
		} else {
			// There's at least one item already in the list
			new_node->next = list->head;
			list->head = new_node;
		}

		list->length++;
		return 0; // Success!
	}

	// Iterate over the list, looking for "after_data"
	struct bz_node *curr = list->head;
	while (curr != nullptr) {
		// Not the node we're looking for. Move along.
		if (curr->data != after_data) {
			curr = curr->next;
			continue;
		}

		// Allocate a new node
		struct bz_node *new_node = malloc(sizeof(*new_node));
		if (!new_node) {
			bz_error(BZ_LOG_LIST, __FILE__, __LINE__,
				"Insertion failed: new node could not be allocated.");
			return -4;
		}
		new_node->data = data;
		new_node->next = nullptr;

		// Update our pointers
		new_node->next = curr->next;
		curr->next = new_node;
		if (list->tail == curr) {
			list->tail = new_node;
		}

		list->length++;
		return 0; // Success!
	}

	// Failed to find after_data in the list.
	return -2;
}

/**
 * Replaces the first instance of "search_data" in the provided list with "replacement", returning
 * 0 on success or a negative value on failure. If a function is provided for "free_data", then the
 * replaced data will be freed via that function if the replacement occurs.
 *
 * Return values:
 *   0: Replacement was successful!
 *   -1: Provided list was not initialized.
 *   -2: Provided search_data was not found.
 *   -3: Not enough memory to allocate new node.
 */
int bz_list_replace(
	struct bz_list *list,
	void *search_data,
	void *replacement,
	void (*free_data)(void *)
) {
	if (list == nullptr) {
		bz_error(BZ_LOG_LIST, __FILE__, __LINE__, "Remove failed: list was not initialized.");
		return -1;
	}

	// Iterate over the list, looking for "item"
	struct bz_node *prev = nullptr;
	struct bz_node *curr = list->head;
	while (curr != nullptr) {
		// Not the node we're looking for. Move along.
		if (curr->data != search_data) {
			prev = curr;
			curr = curr->next;
			continue;
		}

		// Allocate space for the new node
		struct bz_node *new_node = malloc(sizeof(*new_node));
		if (!new_node) {
			bz_error(BZ_LOG_LIST, __FILE__, __LINE__,
				"Replacement failed: new node could not be allocated.");
			return -3;
		}
		new_node->data = replacement;
		new_node->next = nullptr;

		// Do the replacement
		new_node->next = curr->next;
		if (curr == list->tail) list->tail = new_node;
		if (curr == list->head) list->head = new_node;
		else                    prev->next = new_node;

		// Clean up the old node
		if (free_data != nullptr) {
			free_data(curr->data);
		}
		free(curr);

		return 0;
	}

	return -2; // "item" not found.
}

/**
 * Removes first item with matching data. To remove multiple items, use bz_list_filter(). If a
 * value for "free_data" is given, then it will be called and provided with the removed data, which
 * can be useful for freeing up memory.
 *
 * Returns "1" if the item was found and removed, "0" if it wasn't found but there were no failures,
 * or a negative value on failure.
 */
int bz_list_remove(struct bz_list *list, void *data, void (*free_data)(void *))
{
	if (list == nullptr) {
		bz_error(BZ_LOG_LIST, __FILE__, __LINE__, "Remove failed: list was not initialized.");
		return -1;
	}

	// Empty list.
	if (list->head == nullptr) {
		return 0;
	}
	// One item...
	if (list->head->next == nullptr) {
		// ...and it's a match!
		if (list->head->data == data) {
			if (free_data != nullptr) {
				free_data(list->head->data);
			}
			free(list->head);
			list->head = nullptr;
			list->tail = nullptr;
			list->length--;
			return 1;
		}
		return 0;
	}
	// First item matches...
	if (list->head->data == data) {
		struct bz_node *next = list->head->next;

		if (free_data != nullptr) {
			free_data(list->head->data);
		}
		free(list->head);

		list->head = next;
		list->length--;
		return 1;
	}

	// Loop through list, removing matches as needed.
	struct bz_node *prev = list->head;
	struct bz_node *curr = prev->next;
	while (curr != nullptr) {
		if (curr->data != data) {
			// Advance the pointers
			prev = curr;
			curr = curr->next;
			continue;
		}

		// Remove current from the list
		list->length--;
		prev->next = curr->next;
		// ...and update the list's tail if needed.
		if (list->tail == curr) {
			list->tail = prev;
			prev->next = nullptr;
		}

		// Free current's data
		if (free_data != nullptr) {
			free_data(curr->data);
		}
		free(curr);

		return 1;
	}

	return 0;
}

/**
 * Removes items from the provided list that don't pass the "item_matches" function. Each item, as
 * well as "match_data", are provided to the matching function. If a value is given for "free_data",
 * then it will be called with the data for each removed item.
 *
 * Returns an integer representing the number of removed items, or a negative value if there were
 * errors.
 */
int bz_list_filter(
	struct bz_list *list,
	void *match_data,
	bool (*item_matches)(void *, void *),
	void (*free_data)(void *)
) {
	if (list == nullptr) {
		bz_error(BZ_LOG_LIST, __FILE__, __LINE__, "Filter failed: list was not initialized.");
		return -1;
	}

	int removed_items = 0;

	struct bz_node *prev = nullptr;
	struct bz_node *curr = list->head;
	while (curr != nullptr) {
		struct bz_node *next = curr->next;

		if (item_matches(curr->data, match_data)) {
			// "prev" only stores the last *non-removed* result
			prev = curr;
		} else {
			// Item didn't pass the test, so remove it.
			list->length--;
			removed_items++;

			// Update pointers
			if (list->head == curr) { list->head = next; }
			if (list->tail == curr) { list->tail = prev; }
			if (prev != nullptr)    { prev->next = next; }

			// Free up memory
			if (free_data != nullptr) {
				free_data(curr->data);
			}
			free(curr);
		}

		curr = next;
	}

	return removed_items;
}

/**
 * Removes all data from the provided list, and resets its length to 0. If a value is given for
 * "free_data", then it will be called with the data for each removed item.
 */
void bz_list_clear(struct bz_list *list, void (*free_data)(void *))
{
	if (list == nullptr) {
		bz_warn(BZ_LOG_LIST, __FILE__, __LINE__, "List clearing failed: list was not initialized.");
		return;
	}
	struct bz_node *next = list->head;
	while (next != nullptr) {
		struct bz_node *current = next;
		next = current->next;
		if (free_data != nullptr) {
			free_data(current->data);
		}
		free(current);
	}
	list->length = 0;
}

/**
 * Clears and then frees the provided list. If a value is given for "free_data", then it will be
 * called with the data for each removed item.
 *
 * The provided list should not be used after calling this function.
 */
void bz_list_free(struct bz_list *list, void (*free_data)(void *))
{
	if (list == nullptr) {
		bz_warn(BZ_LOG_LIST, __FILE__, __LINE__, "List freeing failed: list was not initialized.");
		return;
	}
	bz_list_clear(list, free_data);
	free(list);
}

/**
 * Finds and returns the data for the first item in the list which matches "match_data", as
 * determined by "item_matches". The match function takes a pointer to an item as the first
 * parameter, and "match_data" as the second.
 *
 * Returns nullptr if the list is either not initialized, or if no matching nodes were found.
 */
void *bz_list_find(struct bz_list *list, void *match_data, bool (*item_matches)(void *, void *))
{
	if (list == nullptr) {
		bz_warn(BZ_LOG_LIST, __FILE__, __LINE__, "Find failed: list was not initialized.");
		return nullptr;
	}

	struct bz_node *node = list->head;
	while (node != nullptr) {
		if (item_matches(node->data, match_data)) {
			return node->data;
		}
		node = node->next;
	}

	// Match not found
	return nullptr;
}

/**
 * Returns a neighbor of item, prioritized in the following order:
 *   1. The node that comes BEFORE the item (if present)
 *   2. The node that comes AFTER the item (if present)
 *   3. nullptr
 */
void *bz_list_get_neighbor(struct bz_list *list, void *item)
{
	if (list == nullptr) {
		bz_warn(BZ_LOG_LIST, __FILE__, __LINE__, "Get neighbor failed: list was not initialized.");
		return nullptr;
	}

	// "item" is first in the list
	if (list->head->data == item) {
		return list->head->next->data;
	}

	// Search for "item", keeping track of the previous node to return.
	struct bz_node *prev = list->head;
	struct bz_node *curr = prev->next;
	while (curr != nullptr) {
		if (curr->data == item) {
			return prev->data;
		}

		// Advance the loop
		prev = curr;
		curr = curr->next;
	}

	// Match not found.
	return nullptr;
}

/**
 * Creates a duplicate of the provided list. The data for each element is provided to "clone_data",
 * and its returned value is used as the data for the new list. The original list is unchanged.
 */
struct bz_list *bz_list_clone(struct bz_list *list, void *(*clone_data)(void *))
{
	// Base case - source list is null.
	if (list == nullptr) { return nullptr; }

	// Create the new list
	struct bz_list *new_list = bz_list_create();
	if (new_list == nullptr) {
		bz_error(BZ_LOG_LIST, __FILE__, __LINE__, "Failed to allocate memory duplicating list.");
		return nullptr;
	}

	// Clone each item from the source list into the new list.
	struct bz_node *current = list->head;
	while (current != nullptr) {
		void *cloned_data = clone_data(current->data);
		if (bz_list_append(new_list, cloned_data) != 0) {
			goto append_failure;
		}
		current = current->next;
	}

	return new_list;

append_failure:
	bz_error(BZ_LOG_LIST, __FILE__, __LINE__, "Failed to duplicate items into target list.");
	bz_list_free(new_list, free);
	return nullptr;
}