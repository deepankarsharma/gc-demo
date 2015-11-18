#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>

#include "../common/mem.h"
#include "../common/node.h"
#include "allocate.h"
#include "copy.h"

/* Current allocation region: */
uintptr_t space_start, space_end;
/* Current allocation pointer: */
uintptr_t space_next;

struct node *allocate()
{
  if (space_next + sizeof(struct node_plus_header) > space_end)
    collect_garbage();

  struct node_plus_header *p = (struct node_plus_header *)space_next;
  space_next += sizeof(struct node_plus_header);

  p->forwarded = 0;

  return &p->n;
}

struct node **root_addrs[128];
int num_roots;

static void copy_or_forward(struct node **addr, struct node_plus_header **alloc_pos_ptr)
{
  if (is_allocated(*addr)) {
    struct node_plus_header *nh;
    nh = (struct node_plus_header *)((char *)*addr - offsetof(struct node_plus_header, n));
    
    if (nh->forwarded) {
      *addr = nh->forward_to;
    } else {
      struct node_plus_header *nh2;
      nh2 = *alloc_pos_ptr;
      (*alloc_pos_ptr)++;
      memcpy(nh2, nh, sizeof(struct node_plus_header));
      nh->forwarded = 1;
      nh->forward_to = &nh2->n;
      *addr = &nh2->n;

      copy_or_forward(&nh2->n.left, alloc_pos_ptr);
      copy_or_forward(&nh2->n.right, alloc_pos_ptr);
    }
  }
}

void copy_from_roots(struct node_plus_header **alloc_pos_ptr)
{
  for (int i = 0; i < num_roots; i++) {
    copy_or_forward(root_addrs[i], alloc_pos_ptr);
  }
}