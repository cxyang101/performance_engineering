#include <assert.h>
#include <cilk/cilk.h>
#include <cilk/reducer.h>
#include <cilk/reducer_opadd.h>
#include <intersection_detection.h>
#include <math.h>
#include <quadtree.h>
#include <stdio.h>
#include <stdlib.h>
#include "./collision_world.h"
#include "./intersection_event_list.h"
#include "./rect.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// create an array to store all of the nodes of the quadtree
static QuadTree full_tree[QUADTREE_SIZE];

// declare a type for an intersection event list reducer
typedef CILK_C_DECLARE_REDUCER(IntersectionEventList)
    IntersectionEventListReducer;

// create an instance of the reducer for tracking intersection events
IntersectionEventListReducer EVENT_LIST_REDUCER = CILK_C_INIT_REDUCER(
    IntersectionEventList, event_list_reduce, event_list_identity,
    event_list_destroy,
    (IntersectionEventList){.head = NULL, .tail = NULL, .size = 0});

static void detect_intersections(int index, CollisionWorld *collisionWorld,
                          IntersectionEventList *intersection_events);

// update the quadtree at position index with the given number of lines in the
// node, number of lines in the child, pointer to the start of the lines in the
// node, and pointer to the start of the lines in the child
static void QuadTree_new(int index, int num_lines, bool has_children, Line **lines) {
  full_tree[index].lines = lines;
  full_tree[index].num_lines = num_lines;
  full_tree[index].child_lines = NULL;
}

// determines which quadrant a line is completely contained in (0, 1, 2, 3), or
// returns 4 if not fully contained in any quadrant
static int get_quadrant_number(Line *line, double xmin, double xmax, double ymin,
                        double ymax) {
  // calculate the quadrant divisions
  double mid_x = (xmin + xmax) / 2;
  double mid_y = (ymin + ymax) / 2;

  // perform a check for the quadrant of the line
  if (line->rectangle.xmax < mid_x) { // left half
    if (line->rectangle.ymax < mid_y) // upper half
      return 0;
    else if (line->rectangle.ymin > mid_y) // lower half
      return 2;
    else
      return 4;
  } else if (line->rectangle.xmin > mid_x) { // right half
    if (line->rectangle.ymax < mid_y) // upper half
      return 1;
    else if (line->rectangle.ymin > mid_y) // lower half
      return 3;
    else
      return 4;
  } else {
    return 4;
  }
}

// sorts array of Line* by quadrant number from 0 to 4 in place
static void sort_lines_by_quadrant(Line **lines, int assignment[],
                            int children_sizes[], int num_lines) {
  // find indices in the quadrant currently being swapped
  int index_in_quadrant = 0;
  // find indices to swap to place quadrant correctly
  int index_to_swap = 0;
  // mark the start of the quadrant
  int quadrant_start = 0;
  // temporary variable for swaps
  Line *temp;

  // sort the first four quadrants (the last one falls into place automatically)
  for (int quadrant = 0; quadrant < 4; quadrant++) {
    // while we are within the quadrant size
    while (index_to_swap < children_sizes[quadrant]) {
      // if we have found something in this quadrant, swap it into the quadrant
      if (assignment[index_in_quadrant] == quadrant) {
        temp = lines[index_in_quadrant];
        lines[index_in_quadrant] = lines[quadrant_start + index_to_swap];
        lines[quadrant_start + index_to_swap] = temp;

        // swap the assignments as well
        assignment[index_in_quadrant] =
            assignment[quadrant_start + index_to_swap];
        assignment[quadrant_start + index_to_swap] = quadrant;

        // increment to look for the next line to swap
        index_to_swap++;
      }
      // increment to look for the next line in the quadrant
      index_in_quadrant++;
    }
    // move to the next quadrant to be swapped into place
    quadrant_start += children_sizes[quadrant];
    index_to_swap = 0;
    index_in_quadrant = quadrant_start;
  }
}

// assign lines to quadtree and make any new quadtrees as needed
static void QuadTree_buildQuadTree(int index, double xmin, double xmax, double ymin,
                            double ymax, int depth) { // QuadTree* qtree){
  // quadtree exceeds max_bin and should be split
  int num_lines = full_tree[index].num_lines;
  Line **lines = full_tree[index].lines;

  double x_mid = (xmin + xmax) / 2;
  double y_mid = (ymin + ymax) / 2;

  // determine to which quadrant each line belongs and count how many lines are
  // in each quadrant
  int assignment[full_tree[index].num_lines];
  int children_sizes[5] = {0, 0, 0, 0, 0};

  for (unsigned int i = 0; i < num_lines; i++) {
    int quadrant = get_quadrant_number(lines[i], xmin, xmax, ymin, ymax);
    assignment[i] = quadrant;
    children_sizes[quadrant]++;
  }

  // using these assignments, sort the lines by quadrant
  sort_lines_by_quadrant(lines, assignment, children_sizes, num_lines);

  // create pointers to the lines for each of the children and the node itself
  Line **line_q1 = lines;
  Line **line_q2 = line_q1 + children_sizes[0];
  Line **line_q3 = line_q2 + children_sizes[1];
  Line **line_q4 = line_q3 + children_sizes[2];
  Line **large_lines = line_q4 + children_sizes[3];

  // reset the properties of the current node for the lines that pass through
  // multiple quadrants
  full_tree[index].child_lines = lines;
  full_tree[index].lines = large_lines;

  // set pointer to lines of first child
  full_tree[index].child_num_lines =
      full_tree[index].num_lines - children_sizes[4];
  full_tree[index].num_lines = children_sizes[4];

  // create quadtrees for the children
  unsigned int new_depth = depth + 1;
  unsigned int child_base = 4 * index + 1;

  QuadTree_new(child_base, children_sizes[0], false, line_q1);
  QuadTree_new(child_base + 1, children_sizes[1], false, line_q2);
  QuadTree_new(child_base + 2, children_sizes[2], false, line_q3);
  QuadTree_new(child_base + 3, children_sizes[3], false, line_q4);

  // recurse on the children as appropriate
  if (new_depth < MAX_DEPTH) {
    if (children_sizes[0] > MAX_BIN) {
      cilk_spawn QuadTree_buildQuadTree(child_base, xmin, x_mid, ymin, y_mid,
                                        new_depth);
    }
    if (children_sizes[1] > MAX_BIN) {
      cilk_spawn QuadTree_buildQuadTree(child_base + 1, x_mid, xmax, ymin,
                                        y_mid, new_depth);
    }
    if (children_sizes[2] > MAX_BIN) {
      cilk_spawn QuadTree_buildQuadTree(child_base + 2, xmin, x_mid, y_mid,
                                        ymax, new_depth);
    }
    if (children_sizes[3] > MAX_BIN) {
      cilk_spawn QuadTree_buildQuadTree(child_base + 3, x_mid, xmax, y_mid,
                                        ymax, new_depth);
    }
    cilk_sync;
  }
  return;
}

// provide a fast path test for bounding box intersection
static inline bool bounding_box_intersect(Line *line1, Line *line2) {
  // tolerance for intersecting bounding boxes
  vec_dimension epsilon = 1e-4;

  // check if the lines are exclusive in the x- and y-dimensions
  bool line1_above_line2 =
      line1->rectangle.ymax - line2->rectangle.ymin < -epsilon;
  bool line2_above_line1 =
      line2->rectangle.ymax - line1->rectangle.ymin < -epsilon;
  bool line1_leftof_line2 =
      line1->rectangle.xmax - line2->rectangle.xmin < -epsilon;
  bool line2_leftof_line1 =
      line2->rectangle.xmax - line1->rectangle.xmin < -epsilon;

  bool nonintersecting_in_y = line1_above_line2 || line2_above_line1;
  bool nonintersecting_in_x = line1_leftof_line2 || line2_leftof_line1;

  // check if the bounding rectangles intersect
  bool nonintersecting_rectangles =
      nonintersecting_in_y && nonintersecting_in_x;

  // return the not of the boolean for the rectangles intersecting
  return !nonintersecting_rectangles;
}

// check for line intersection
static void check_line_intersect(Line *line1, Line *line2,
                          CollisionWorld *collisionWorld,
                          IntersectionEventList *intersection_events) {
  if (!bounding_box_intersect(line1, line2)) {
    return;
  }

  if (compareLines(line1, line2) >= 0) {
    Line *temp = line1;
    line1 = line2;
    line2 = temp;
  }
  IntersectionType iType = intersect(line1, line2, collisionWorld->timeStep);
  if (iType != NO_INTERSECTION) {
    IntersectionEventList_appendNode(&REDUCER_VIEW(EVENT_LIST_REDUCER), line1,
                                     line2, iType);
  }
}

// check for all pairwise intersections within a quadtree at given index
static inline void
check_within_quadtree(int index, CollisionWorld *collisionWorld,
                      IntersectionEventList *intersection_events) {
  Line *line1;
  Line *line2;
#pragma cilk grainsize 600
  cilk_for(int i = 0; i < full_tree[index].num_lines; i++) {
    line1 = full_tree[index].lines[i];
    for (int j = i + 1; j < full_tree[index].num_lines; j++) {
      line2 = full_tree[index].lines[j];
      check_line_intersect(line1, line2, collisionWorld, intersection_events);
    }
  }
}

static void check_with_children(int index, CollisionWorld *collisionWorld,
                         IntersectionEventList *intersection_events) {
  Line *line1;
  Line *line2;
  Line **child_lines = full_tree[index].child_lines;
  cilk_for(int i = 0; i < full_tree[index].child_num_lines; i++) {
    line1 = child_lines[i];
    for (int j = 0; j < full_tree[index].num_lines; j++) {
      line2 = full_tree[index].lines[j];
      check_line_intersect(line1, line2, collisionWorld, intersection_events);
    }
  }
}

static void recurse_children(int index, CollisionWorld *collisionWorld,
                      IntersectionEventList *intersection_events) {
  int child_base = 4 * index + 1;
  cilk_for(int i = 0; i < 4; i++) {
    detect_intersections(child_base + i, collisionWorld, intersection_events);
  }
}

// detect intersections in the quadtree
void detect_intersections(int index, CollisionWorld *collisionWorld,
                          IntersectionEventList *intersection_events) {

  // If not a leaf, then we need to complete three steps in parallel
  if (full_tree[index].child_lines != NULL) {
    // Check for intersections between all pairs within quadtree
    cilk_spawn check_within_quadtree(index, collisionWorld,
                                     intersection_events);
    // Check for intersection against all lines in the subtree rooted at this
    // node
    cilk_spawn check_with_children(index, collisionWorld, intersection_events);
    // Recursively call detect_intersections on all children
    cilk_spawn recurse_children(index, collisionWorld, intersection_events);
    cilk_sync;
  } else { // If leaf, only need to check all pairs in quadtree
    check_within_quadtree(index, collisionWorld, intersection_events);
  }
}

// detect intersections with reducer and parallelism
int detect_intersections_with_quadtree(
    CollisionWorld *collisionWorld,
    IntersectionEventList *intersection_events) {
  QuadTree_new(0, collisionWorld->numOfLines, false, collisionWorld->lines);
  QuadTree_buildQuadTree(0, BOX_XMIN, BOX_XMAX, BOX_YMIN, BOX_YMAX, 0);
  detect_intersections(0, collisionWorld, intersection_events);
  int num_collisions = REDUCER_VIEW(EVENT_LIST_REDUCER).size;
  IntersectionEventList_mergeNodes(intersection_events,
                                   &REDUCER_VIEW(EVENT_LIST_REDUCER));
  CILK_C_UNREGISTER_REDUCER(EVENT_LIST_REDUCER);
  return num_collisions;
}
