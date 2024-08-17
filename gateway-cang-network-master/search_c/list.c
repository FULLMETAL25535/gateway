#include <stdio.h>
#include <string.h>
#include "list.h"

struct student
{
	int age;
	char name[64];
	struct list_head list;
};

void main()
{
	struct list_head head;
	INIT_LIST_HEAD(&head);

	struct student stu1;
	strcpy(stu1.name, "zhangsan");
	stu1.age = 1;

	struct student stu2;
	strcpy(stu2.name, "lisi");
	stu2.age = 2;

	struct student stu3;
	strcpy(stu3.name, "wangwu");
	stu3.age = 3;


	list_add(&stu3.list, &head);
	list_add(&stu2.list, &head);
	list_add(&stu1.list, &head);

	struct list_head *pos;
	struct student *tmp;

	printf("init list\n");
	list_for_each(pos, &head)
	{
		tmp = list_entry(pos, struct student, list);
		printf("name = %s, age = %d\n", tmp->name, tmp->age);
	}
	printf("\n");

	pos = get_first(&head);
	tmp = list_entry(pos, struct student, list);
	printf("first is %s\n\n", tmp->name);

	pos = get_last(&head);
	tmp = list_entry(pos, struct student, list);
	printf("last is %s\n\n", tmp->name);

	puts("del last");
	list_del(pos);

	printf("after del:");
	list_for_each(pos, &head)
	{
		tmp = list_entry(pos, struct student, list);
		printf("%d ", tmp->age);
	}
	puts("\n");
}
