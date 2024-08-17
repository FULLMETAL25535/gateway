/***********************************************************************************
Copy right:	Coffee Tech.
Author:			jiaoyue
Version:		V1.0
Date:			2018-12
Description:	从linux内核抽出来链表，可以做通用链表使用，可保留
***********************************************************************************/

#ifndef _LIST_H
#define _LIST_H

//定义核心链表结构
struct list_head
{
	struct list_head *next, *prev;
};

//链表初始化
static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

//插入结点
static inline void __list_add(struct list_head *new_list,
							  struct list_head *prev, struct list_head *next)
{
	next->prev = new_list;
	new_list->next = next;
	new_list->prev = prev;
	prev->next = new_list;
}

//在链表头部插入
static inline void list_add(struct list_head *new_list, struct list_head *head)
{
	__list_add(new_list, head, head->next);
}

//尾部插入结点
static inline void list_add_tail(struct list_head *new_list, struct list_head *head)
{
	__list_add(new_list, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

//删除任意结点
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

//是否为空
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

//得到第一个结点
static inline struct list_head *get_first(const struct list_head *head)
{
	return head->next;
}

//得到最后一个结点
static inline struct list_head *get_last(const struct list_head *head)
{
	return head->prev;
}

static inline void __list_splice(const struct list_head *list,
								 struct list_head *prev,
								 struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(const struct list_head *list,
							   struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice_tail(struct list_head *list,
									struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head->prev, head);
}

//后序（指针向后走）遍历链表
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

//前序（指针向前走）遍历链表
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define offsetof_list(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)

#define list_entry(ptr, type, member) ({			\
    const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
    (type *)( (char *)__mptr - offsetof_list(type,member) ); })

#endif
