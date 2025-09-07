#include "list.h"
#include "stdio.h"
void vListInitialistItem(ListItem_t* const pxItem)
{
    pxItem->pxContainer = NULL;
}


/*
*************************根节点初始化函数*****************************
* @param pxList 根节点指针
* @author wanglianghua
*/
void vListInitialise(List_t* const pxList)
{
	/* 将链表索引指针指向最后一个节点 */
	pxList->pxIndex = (ListItem_t*)(&(pxList->xListEnd));

	/* 将链表最后一个节点的辅助排序的值设置为最大，确保该节点就是链表的最后节点 */
	pxList->xListEnd.xItemValue = 20;

	/* 将最后一个节点的pxNext 和pxPrevious 指针均指向节点自身，表示链表为空 */
	pxList->xListEnd.pxNext = (ListItem_t*)(&(pxList->xListEnd));
	pxList->xListEnd.pxPrev = (ListItem_t*)(&(pxList->xListEnd));

	pxList->uxNumberOfItems = 0U;
}


/*
****************************节点插入函数***********************************
* @param pxList 根节点指针
* @param pxNewListItem 新插入节点指针
* @author wanglianghua
*/
void vListInsertEnd(List_t* const pxList, ListItem_t* const pxNewListItem)
{
	ListItem_t* pxCurListItem = (ListItem_t*)(pxList->pxIndex);
	pxNewListItem->pxPrev = pxCurListItem->pxPrev;
	pxNewListItem->pxNext = pxCurListItem;
	pxCurListItem->pxPrev->pxNext = pxNewListItem;
	pxCurListItem->pxPrev = pxNewListItem;
	pxList->pxIndex = pxNewListItem;

	pxNewListItem->pxContainer = (void*)pxList;

	(pxList->uxNumberOfItems) ++;
}


/*
****************************节点升序排列插入***********************************
* @param pxList 根节点指针
* @param pxNewListItem 新插入节点指针
* @author wanglianghua
*/
void vListInsert(List_t* const pxList, ListItem_t* const pxNewListItem)
{
	ListItem_t* pxIterator;
	uint16_t curIndex = pxNewListItem->NodeTick;
	if (curIndex == 20)
	{
		pxIterator = pxList->xListEnd.pxPrev;
	}
	else
	{
		for (pxIterator = (ListItem_t*)( & (pxList->xListEnd));
			curIndex >= pxIterator->pxNext->NodeTick;
			pxIterator = pxIterator->pxNext)
		{

		}
	}

	pxNewListItem->pxNext = pxIterator->pxNext;
	pxNewListItem->pxPrev = pxIterator;
	pxIterator->pxNext->pxPrev = pxNewListItem;
	pxIterator->pxNext = pxNewListItem;

	pxNewListItem->pxContainer = (void*)pxList;
	(pxList->uxNumberOfItems)++;
}


/*
****************************将节点从链表删除***********************************
* @param pxList 根节点指针
* @param pxNewListItem 新插入节点指针
* @return 剩余节点个数
* @author wanglianghua
*/
uint16_t vListDelet(ListItem_t* const pxItemToDelet)
{
	List_t* const pxList = (List_t*)pxItemToDelet->pxContainer;
	pxItemToDelet->pxNext->pxPrev = pxItemToDelet->pxPrev;
	pxItemToDelet->pxPrev->pxNext = pxItemToDelet->pxNext;

	if (pxList->pxIndex == pxItemToDelet)
	{
		pxList->pxIndex = pxItemToDelet->pxPrev;
	}

	pxItemToDelet->pxContainer = NULL;

    pxList->uxNumberOfItems--;

	return pxList->uxNumberOfItems;
}
