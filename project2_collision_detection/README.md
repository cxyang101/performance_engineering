# 6172 Project 2: 
This program implements efficient collision detection of line segments for physics engine. The naive approach for detecting collisions is to check all pairs of line segments for collisions on every timestep, but this is an expensive operation.  
Therefore, we implemented a quadtree, which recursively separates the environment into quadrants, such that lines in entirely different quadrants need not be compared. 
We further optimized by creating a fast pass in our collision-detection check using the bounding boxes of line segment, and through the parallelization of our algorithm. 
We also implemented a reducer for collecting intersection events to parallelize our code, reducing the runtime of our code from 13.3 seconds for 4000 timesteps of mit.in to approximately 4.5 seconds.
