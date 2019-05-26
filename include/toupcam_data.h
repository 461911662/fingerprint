#ifndef _TOUPCAM_DATA_H_
#define _TOUPCAM_DATA_H_

#define HASHTABLESIZE 255

typedef struct directory{
    char *pcname;
    char *pcval;
    struct directory *pdirectory;
}DIRECTORY_S;

typedef struct HashTable
{
    unsigned iHashVal;
    struct directory *Table[HASHTABLESIZE];
}HASHTABLE_S;

HASHTABLE_S* createHash();       /* 创建 */
DIRECTORY_S* createnode(char *key, char*val);   /* 创建node */
void destorynode(DIRECTORY_S *pnode);           /* 删除node */
DIRECTORY_S* findnnode(HASHTABLE_S *ptable, DIRECTORY_S *pnode);        /* 查 */
void findbykey();        /* 查1 */  
void findbyvalue();      /* 查2 */
HASHTABLE_S* insertnode(HASHTABLE_S *ptable, DIRECTORY_S *pnode);       /* 增 */
HASHTABLE_S* deletenode(HASHTABLE_S *ptable, DIRECTORY_S *pnode);       /* 删 */
HASHTABLE_S* updatenode(HASHTABLE_S *ptable, DIRECTORY_S *pnode);       /* 改 */
void destoryHash(HASHTABLE_S *ptable);      /* 销毁 */

#endif