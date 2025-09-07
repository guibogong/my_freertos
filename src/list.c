#include "list.h"
#include "stdio.h"
#include "portmacro.h"
void vListInitialistItem(ListItem_t* const pxItem)
{
    pxItem->pxContainer = NULL;
}


/*
*************************���ڵ��ʼ������*****************************
* @param pxList ���ڵ�ָ��
* @author wanglianghua
*/
void vListInitialise(List_t* const pxList)
{
	/* ����������ָ��ָ�����һ���ڵ� */
	pxList->pxIndex = (ListItem_t*)(&(pxList->xListEnd));

	/* ���������һ���ڵ�ĸ��������ֵ����Ϊ���ȷ���ýڵ�������������ڵ� */
	pxList->xListEnd.xItemValue = portMAX_DELAY;

	/* �����һ���ڵ��pxNext ��pxPrevious ָ���ָ��ڵ���������ʾ����Ϊ�� */
	pxList->xListEnd.pxNext = (ListItem_t*)(&(pxList->xListEnd));
	pxList->xListEnd.pxPrev = (ListItem_t*)(&(pxList->xListEnd));

	pxList->uxNumberOfItems = 0U;
}


/*
****************************�ڵ���뺯��***********************************
* @param pxList ���ڵ�ָ��
* @param pxNewListItem �²���ڵ�ָ��
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
****************************�ڵ��������в���***********************************
* @param pxList ���ڵ�ָ��
* @param pxNewListItem �²���ڵ�ָ��
* @author wanglianghua
*/
void vListInsert(List_t* const pxList, ListItem_t* const pxNewListItem)
{
	ListItem_t* pxIterator;
	uint16_t curIndex = pxNewListItem->NodeTick;
	if (curIndex == portMAX_DELAY)
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
****************************���ڵ������ɾ��***********************************
* @param pxList ���ڵ�ָ��
* @param pxNewListItem �²���ڵ�ָ��
* @return ʣ��ڵ����
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

    (pxList->uxNumberOfItems)--;

	return pxList->uxNumberOfItems;
}
