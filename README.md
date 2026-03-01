# 3d viewer

* Load edges and edge-to-face adjacency list from file
* offscreen buffer and backface culling
* The culling test is 12 cross products (cheap: 2 multiplies + 1 subtract each), edge popping just inherent to the integer precision at this scale — the faces on the far side project to very small areas where the cross product naturally fluctuates around zero.
* Keyboard and Kempston joystick rotational controls. Fire button 1 / 'Q' reverses rotation direction on the X axis, Fire button 2/ 'A' reverses direction on the Y axis. Kempston Up or 'O' reverses direction on the Z
* Every 10th frame, all 768 attribute bytes get set to a random ink colour (1-7, skipping black so the wireframe stays visible) on black paper. Uses a simple 8-bit LFSR for the random number.
* Renders optional text string, positioned at row 21, col 4; 2 characters from the bottom and 2 from the right edge.
