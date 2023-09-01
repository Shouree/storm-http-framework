Usage:

The following commands are recognized:
- <number> + "i": Insert a new node at the location of the mouse cursor.
  Example: 12i to insert a node with label "12"
- "d": Remove the node at the location of the mouse cursor.
- "c": Clear both edges from the node at the location of the mouse cursor.
- "q": Clear all nodes.
- "l" or "j": Add or replace the left child of the node at the location of the mouse cursor.
  Then click the node where the edge shall end.
- "r" or "k": Add or replace the right child of the node at the location of the mouse cursor.
  Then click the node where the edge shall end.
- "esc": Cancel the current operation (mainly creating new edges).
- "<" (on Swedish keyboards) or "z": Insert a | node.
- <number> + "e": Load an example tree. This removes any nodes currently present.
  For example: 1e loads the file "1.tree", 2e loads the file "2.tree" and so on.
  Examples are numbered as in the student handout (exercise 1 is 1e and so on),
  the ones in the teacher's handouts are numbered (11 and 12, i.e. 11e and 12e).
- Shift+"l" or Shift+"j": Rotate the node at the location of the mouse cursor with its left child.
- Shift+"r" or Shift+"k": Rotate the node at the location of the mouse cursor with its right child.
- Shift+"c": Clone the tree starting at the location of the mouse cursor and place it below.
- Ctrl+"z": Undo (node movements are not tracked). Keep in mind that loading a new graph
  clears the undo stack.
- Click and drag a node: Move the node.
- Click and drag the background: Move all contents of the window.
- "a": Perform automatic layout in the subtree that starts in the node under the mouse cursor.
