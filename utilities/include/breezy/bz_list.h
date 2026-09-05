#ifndef BZ_LIST_H
#define BZ_LIST_H
// #################################################################################################


struct bz_node {
	void *data;
	struct bz_node *next;
	struct bz_node *prev;
};

struct bz_list {
	struct bz_node *head;
	struct bz_node *tail;
	int length;
};

#define bz_list_foreach(item, list) \
	for (struct bz_node *node = (list)->head; node != nullptr; node = node->next) \
		if (((item) = node->data), 0) {} else

#define bz_list_foreach_rev(item, list) \
	for (struct bz_node *node = (list)->tail; node != nullptr; node = node->prev) \
		if (((item) = node->data), 0) {} else

struct bz_list *bz_list_create();
int bz_list_append(struct bz_list *list, void *data);
int bz_list_insert(struct bz_list *list, void *data, void *after_data);
void *bz_list_shift(struct bz_list *list);
void *bz_list_pop(struct bz_list *list);
int bz_list_replace(struct bz_list *list, void *search_data, void *replacement, void (*free_data)(void *));
int bz_list_remove(struct bz_list *list, void *data, void (*free_data)(void *));
int bz_list_filter(struct bz_list *list, void *match_data, bool (*item_matches)(void *, void *), void (*free_data)(void *));
void bz_list_clear(struct bz_list *list, void (*free_data)(void *));
void bz_list_free(struct bz_list *list, void (*free_data)(void *));
void *bz_list_find(struct bz_list *list, void *match_data, bool (*item_matches)(void *, void *));
bool bz_list_contains(struct bz_list *list, void *match_data);
void *bz_list_get_neighbor(struct bz_list *list, void *item);
int bz_list_move_to_end(struct bz_list *list_src, struct bz_list *list_dest);
int bz_list_move_item_to_end(struct bz_list *list, void *data);
struct bz_list *bz_list_clone(struct bz_list *list, void *(*clone_data)(void *));


// #################################################################################################
#endif