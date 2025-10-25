#include "rbtree.h"

#include <stdlib.h>
#include <stdio.h>

static void leftRotate(rbtree *t, node_t *z);
static void rightRotate(rbtree *t, node_t *z);
static void insert_fix(rbtree *t, node_t *z);
static void removeNode(rbtree* t, node_t *cur);
static void transplant(rbtree *t, node_t *u, node_t *v);
static void delete_fixup(rbtree *t, node_t *z);
static node_t *subtreeMin(const rbtree *t, node_t *z);
static void recursiveFairy(const rbtree *t , node_t *cur, key_t *arr, int *index);


rbtree *new_rbtree(void) {
  rbtree *t = calloc(1, sizeof(*t));
  if(!t){
    return NULL;
  }
  //초깃값 설정
  t->nil = calloc(1, sizeof(node_t));
  if(!t->nil){
    free(t);
    return NULL;
  }
  t->nil->color = RBTREE_BLACK;
  t->nil->left = t->nil->right = t->nil->parent = t->nil;
  t->root = t->nil;
  return t;
}

void delete_rbtree(rbtree *t) {
  removeNode(t, t->root);
  free(t->nil);
  free(t);
}

void removeNode(rbtree* t, node_t *x){
  if(x!=t->nil)
  {
    removeNode(t, x->left);
    removeNode(t, x->right);
    free(x);
  }
}

node_t *rbtree_insert(rbtree *t, const key_t key) {
  //초기값 설정
  node_t *cur = t->root;
  node_t *prev;

  // key값으로 새로운 노드 생성
  node_t *z = malloc(sizeof(node_t));
  if(!z){
    return NULL;
  }
  z->key = key;
  z->color = RBTREE_RED;
  z->left = t->nil;
  z->right = t->nil;
  z->parent = t->nil;

  // root가 없을 때
  if(cur == t->nil){
    t->root = z;
    z->color = RBTREE_BLACK;
    return z;
  }

  // tree의 root부터 key가 들어갈 자리 탐색
  while(cur != t->nil){
    prev = cur;
    if(z->key < cur->key){
      cur = cur->left;
    }else cur = cur->right;
  }
  
  // z가 부모를 선택!! 와!!!
  z->parent = prev;

  // 부모가 자식을 어떤 방에 넣을지 확인
  if(z->key < prev->key){
    prev->left = z;
  }else prev->right = z;

  insert_fix(t, z);

  return z;
}

//값이 같을 경우 처리
// 4 2 3 케이스 오류  
void insert_fix(rbtree* t, node_t *z){
  while(z->parent->color == RBTREE_RED){
    if(z->parent == z->parent->parent->left){ 
      if(z->parent->parent->right->color == RBTREE_RED){
        z->parent->parent->color = RBTREE_RED;
        z->parent->color = RBTREE_BLACK;
        z->parent->parent->right->color = RBTREE_BLACK;
        z = z->parent->parent;
      }else {
        if(z == z->parent->right){
        z = z->parent;
        leftRotate(t, z);
        }
        z->parent->color = RBTREE_BLACK;
        z->parent->parent->color = RBTREE_RED;
        rightRotate(t, z->parent->parent);
      }
    }else{
      node_t *uncleR = z->parent->parent->left;
      if(uncleR->color == RBTREE_RED){
        z->parent->parent->color = RBTREE_RED;
        z->parent->color = RBTREE_BLACK;
        uncleR->color = RBTREE_BLACK;
        z = z->parent->parent;
      }else{
      if(z == z->parent->left){
        z = z->parent;
        rightRotate(t, z->parent);
      }
        z->parent->color = RBTREE_BLACK;
        z->parent->parent->color = RBTREE_RED;
        leftRotate(t, z->parent->parent);
      }
    }
  }
  t->root->color = RBTREE_BLACK;
}
void insert_fix(rbtree *t, node_t *inserted) {
  while (inserted->parent->color == RBTREE_RED) {
    node_t *parent = inserted->parent;
    node_t *grand = parent->parent;
    if (parent == grand->left) {
      node_t *uncle = grand->right;
      if (uncle->color == RBTREE_RED) {
        parent->color = RBTREE_BLACK;
        uncle->color = RBTREE_BLACK;
        grand->color = RBTREE_RED;
        inserted = grand;
      } 
      // Case 2 + 3: 삼촌이 BLACK
      else {
        // Case 2: 부모가 왼쪽인데, 새 노드가 오른쪽이면 회전
        if (inserted == parent->right) {
          inserted = parent;
          leftRotate(t, inserted);
          parent = inserted->parent;
          grand = parent->parent;
        }
        // Case 3: 부모를 BLACK으로, 할아버지를 RED로 바꾸고 회전
        parent->color = RBTREE_BLACK;
        grand->color = RBTREE_RED;
        rightRotate(t, grand);
      }

    // 대칭 Case: 부모가 오른쪽 자식일 때
    } else {
      node_t *uncle = grand->left;

      // Case 1 대칭: 삼촌이 RED
      if (uncle->color == RBTREE_RED) {
        parent->color = RBTREE_BLACK;
        uncle->color = RBTREE_BLACK;
        grand->color = RBTREE_RED;
        inserted = grand;
      } 
      // Case 2 + 3 대칭: 삼촌이 BLACK
      else {
        if (inserted == parent->left) {
          inserted = parent;
          rightRotate(t, inserted);
          parent = inserted->parent;
          grand = parent->parent;
        }
        parent->color = RBTREE_BLACK;
        grand->color = RBTREE_RED;
        leftRotate(t, grand);
      }
    }
  }
  t->root->color = RBTREE_BLACK;
}

void leftRotate(rbtree* t, node_t *z){
  if (z->right == t->nil) return;
  node_t *rightChd = z->right;
  // 왼쪽 서브트리가 z의 오른쪽으로 붙는다.
  z->right = rightChd->left;

  // 왼쪽에 서브트리가 있을 경우 이동
  if(rightChd->left != t->nil){
    rightChd->left->parent = z;
  }
  // rigthchd를 z의 부모의 밑으로 옮긴다.
  rightChd->parent = z->parent;
  if(z == t->root){
    t->root = rightChd;
    t->root->parent = t->nil;
  }else if(z == z->parent->left){
    z->parent->left = rightChd;
  }else{
    z->parent->right = rightChd;
  }

  rightChd->left = z;
  z->parent = rightChd;
}
void rightRotate(rbtree* t, node_t *z){
  if (z->left == t->nil) return;
  node_t *leftChd = z->left;
  z->left = leftChd->right;

  if(leftChd->right != t->nil){
    leftChd->right->parent = z;
  }

  leftChd->parent = z->parent;
  if(z == t->root){
    t->root = leftChd;
    t->root->parent = t->nil;
  }else if(z == z->parent->right){
    z->parent->right = leftChd;
  }else{
    z->parent->left = leftChd;
  }

  leftChd->right = z;
  z->parent = leftChd;
}

node_t *rbtree_find(const rbtree *t, const key_t key) {
  node_t *cur = t->root;
  while(cur != t->nil)
  {
    if(key < cur->key)
    {
      cur = cur->left;
      continue;
    }else if(key > cur->key)
    {
      cur = cur->right;
      continue;
    }else if(key == cur->key)
    {
      return cur;
    }
  }
  return NULL;
}

node_t *rbtree_min(const rbtree *t) {
  node_t *z = t->root;
  if(z == t->nil) return t->nil;
  while(z->left != t->nil) z = z->left;
  return z;
}
node_t *rbtree_max(const rbtree *t) {
  node_t *z = t->root;
  if(z == t->nil) return t->nil;
  while(z->right != t->nil) z = z->right;
  return z;
}

int rbtree_erase(rbtree *t, node_t *p) {
  node_t *x;
  // node_t *y = p;
  color_t yCol = p->color;

  // 처음 조건 분기들은 대체할 노드를 찾는 역할
  if(p->left == t->nil)
  {
    x = p->right;
    transplant(t, p, p->right);
  }else if(p->right == t->nil)
  {
    x = p->left;
    transplant(t, p, p->left);
  }else
  {
    // 노드를 삭제하고, tree 포인터 변경
    node_t *y = subtreeMin(t, p->right); //대체 할 노드
    yCol = y->color; //이 노드의 색이 삭제된 노드의 색으로 바뀌니 미리 저장
    x = y->right;

    // y가 p의 바로 오른쪽 자식이 아닐 때
    if(y != p->right)
    {
      // y의 빼기 전 오른쪽 서브트리를 연결해줌
      transplant(t, y, y->right);
      // p에 연결된 서브 트리 y에 연결
      y->right = p->right;
      y->right->parent = y;
    }else //y가 p의 바로 오른쪽 자식일 때
    {
      x->parent = y;
    }
    // y와 p의 부모 연결
    transplant(t,p,y);
    y->left = p->left;
    y->left->parent = y;
    y->color = p->color;
  }

  free(p);
  if(yCol == RBTREE_BLACK)
  {
    delete_fixup(t,x);
  }
  return 0;
}

node_t *subtreeMin(const rbtree *t, node_t *z){
  while(z->left != t->nil) z = z ->left;
  return z;
}

// 대체 노드에 삭제 노드의 부모 포인터 연결
void transplant(rbtree *t, node_t *u, node_t *v)
{
  if(u->parent == t->nil){
    t->root = v;
  }else if(u == u->parent->left)
  {
    u->parent->left = v;
  }else{
    u->parent->right = v;
  }
  v->parent = u->parent;
}

void delete_fixup(rbtree *t, node_t *z){
  while(t->root && z->color == RBTREE_BLACK)
  {
    if(z == z->parent->left)
    {
      node_t *w = z->parent->right;
      if(w->color == RBTREE_RED)
      {
        w->color = RBTREE_BLACK;
        z->parent->color = RBTREE_RED;
        leftRotate(t, z->parent);
        w = z->parent->right;
      }
      if(w->left->color == RBTREE_BLACK && w->right->color == RBTREE_BLACK)
      {
        w->color =RBTREE_RED;
        z = z->parent;
      }
      else
      {
        if(w->right->color == RBTREE_BLACK)
        {
          w->left->color = RBTREE_BLACK;
          w->color = RBTREE_RED;
          rightRotate(t, w);
          w= z->parent->right;
        }
        w->color = z->parent->color;
        z->parent->color = RBTREE_BLACK;
        w->right->color = RBTREE_BLACK;
        leftRotate(t, z->parent);
        z = t->root;
      }
    }else
    {
      node_t *w = z->parent->left;
      if(w->color == RBTREE_RED)
      {
        w->color = RBTREE_BLACK;
        z->parent->color = RBTREE_RED;
        rightRotate(t, z->parent);
        w = z->parent->left;
      }
      if(w->right->color == RBTREE_BLACK && w->left->color == RBTREE_BLACK)
      {
        w->color =RBTREE_RED;
        z = z->parent;
      }
      else
      {
        if(w->left->color == RBTREE_BLACK)
        {
          w->right->color = RBTREE_BLACK;
          w->color = RBTREE_RED;
          leftRotate(t, w);
          w= z->parent->left;
        }
        w->color = z->parent->color;
        z->parent->color = RBTREE_BLACK;
        w->left->color = RBTREE_BLACK;
        rightRotate(t, z->parent);
        z = t->root;
      }
    }
  }
}

int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n) {  
  int index = 0;
  recursiveFairy(t, t->root, arr, &index);
  return *arr;
}

void recursiveFairy(const rbtree *t , node_t *cur, key_t *arr, int *index) {
  if (cur == t->nil) {
    return;
  }
  recursiveFairy(t, cur->left, arr, index);
  arr[(*index)++] = cur->key;
  recursiveFairy(t, cur->right, arr, index);
}