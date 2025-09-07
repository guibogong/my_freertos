#ifndef _H_LIST_H
#define _H_LIST_H

typedef unsigned char      uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int       uint32_t;
typedef signed char        int8_t;
typedef short int          int16_t;
typedef int                int32_t;  

struct xList_ITEM
{
      uint16_t NodeTick;
	  struct xList_ITEM* pxPrev;
	  struct xList_ITEM* pxNext;
	  void* pxOwer;
	  void* pxContainer;
};

typedef struct xList_ITEM ListItem_t;

struct xMin_LIST_ITEM
{
	uint16_t xItemValue;
	ListItem_t* pxNext;
	ListItem_t* pxPrev;
};

typedef struct xMin_LIST_ITEM MinListItem_t;

typedef struct xLIST
{
	uint16_t uxNumberOfItems;
	ListItem_t* pxIndex;
	MinListItem_t xListEnd;
}List_t;


/*
************************************************************************
*                                �궨��
************************************************************************
*/
/* ��ʼ���ڵ��ӵ���� */
#define listSET_LIST_ITEM_OWNER( pxListItem, pxOwner )		( ( pxListItem )->pxOwer = ( void * ) ( pxOwner ) )
/* ��ȡ�ڵ�ӵ���� */
#define listGET_LIST_ITEM_OWNER( pxListItem )	( ( pxListItem )->pxOwer )

/* ��ʼ���ڵ�������ֵ */
#define listSET_LIST_ITEM_VALUE( pxListItem, xValue )	( ( pxListItem )->NodeTick = ( xValue ) )

/* ��ȡ�ڵ�������ֵ */
#define listGET_LIST_ITEM_VALUE( pxListItem )	( ( pxListItem )->NodeTick )

/* ��ȡ������ڵ�Ľڵ��������ֵ */
#define listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxList )	( ( ( pxList )->xListEnd ).pxNext->NodeTick )

/* ��ȡ�������ڽڵ� */
#define listGET_HEAD_ENTRY( pxList )	( ( ( pxList )->xListEnd ).pxNext )

/* ��ȡ����ĵ�һ���ڵ� */
#define listGET_NEXT( pxListItem )	( ( pxListItem )->pxNext )

/* ��ȡ��������һ���ڵ� */
#define listGET_END_MARKER( pxList )	( ( ListItem_t const * ) ( &( ( pxList )->xListEnd ) ) )

/* �ж������Ƿ�Ϊ�� */
#define listLIST_IS_EMPTY( pxList )	( ( BaseType_t ) ( ( pxList )->uxNumberOfItems == ( UBaseType_t ) 0 ) )

/* ��ȡ����Ľڵ��� */
#define listCURRENT_LIST_LENGTH( pxList )	( ( pxList )->uxNumberOfItems )

/* ��ȡ����ڵ��OWNER����TCB */
#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )										\
{																							\
	List_t * const pxConstList = ( pxList );											    \
	/* �ڵ�����ָ�������һ���ڵ�����ڵ�����ָ�룬ָ����һ���ڵ㣬
    �����ǰ������N���ڵ㣬����N�ε��øú���ʱ��pxInedex��ָ���N���ڵ� */\
	( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;							\
	/* ��ǰ����Ϊ�� */                                                                       \
	if( ( void * ) ( pxConstList )->pxIndex == ( void * ) &( ( pxConstList )->xListEnd ) )	\
	{																						\
		( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;						\
	}																						\
	/* ��ȡ�ڵ��OWNER����TCB */                                                             \
	( pxTCB ) = ( pxConstList )->pxIndex->pxOwer;											 \
}

#define listGET_OWNER_OF_HEAD_ENTRY( pxList )  ( (&( ( pxList )->xListEnd ))->pxNext->pxOwer )



void vListInitialistItem(ListItem_t* const pxItem);

void vListInitialise(List_t* const pxList);

void vListInsertEnd(List_t* const pxList, ListItem_t* const pxNewListItem);

void vListInsert(List_t* const pxList, ListItem_t* const pxNewListItem);

uint16_t vListDelet(ListItem_t* const pxItemToDelet);

#endif