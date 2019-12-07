#ifndef QUADTREE_H_
#define QUADTREE_H_

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "./line.h"
#include "./collision_world.h"
#include "./intersection_event_list.h"
#include "./intersection_detection.h"

#define MAX_BIN 30
#define MAX_DEPTH 7
#define QUADTREE_SIZE 21845 // 4^0 + 4^1 + 4^2 + ... + 4^7 = (4^8 - 1)/(4 - 1)

typedef struct QuadTree QuadTree;

typedef struct Node Node;
struct Node{
  Line* line;
  Node* next;
};

struct QuadTree{
  Line** lines;
  Line** child_lines;
  int num_lines;
  int child_num_lines;
};

int detect_intersections_with_quadtree(CollisionWorld* collisionWorld, IntersectionEventList* intersection_events);

#endif
