#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 80
#define HEIGHT 25

// Grid/Canvas represented by a 2D character array
char grid[HEIGHT][WIDTH];

// Enums to represent the types of shapes
typedef enum {
  SHAPE_RECTANGLE,
  SHAPE_LINE,
  SHAPE_CIRCLE,
  SHAPE_TRIANGLE
} ShapeType;

// Structures to store shape properties
typedef struct {
  int x;
  int y;
  int width;
  int height;
} Rectangle;

typedef struct {
  int x1, y1;
  int x2, y2;
} Line;

typedef struct {
  int cx, cy;
  int radius;
} Circle;

typedef struct {
  int x1, y1;
  int x2, y2;
  int x3, y3;
} Triangle;

// Unified Shape structure with an ID for add/delete/modify operations
typedef struct {
  int id;
  ShapeType type;
  union {
    Rectangle rect;
    Line line;
    Circle circle;
    Triangle triangle;
  } data;
} Shape;

// Global shape database
#define MAX_SHAPES 100
Shape shapes[MAX_SHAPES];
int shapeCount = 0;
int nextId = 1;

// --- Canvas Utility Functions ---

// Fills the entire grid with the empty space character '_'
void clearCanvas(char canvas[HEIGHT][WIDTH]) {
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      canvas[y][x] = '_';
    }
  }
}

// Prints the 2D grid to standard output
void displayCanvas(char canvas[HEIGHT][WIDTH]) {
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      putchar(canvas[y][x]);
    }
    putchar('\n');
  }
}

// --- Shape Drawing Algorithms ---

// Function to draw a rectangle on the canvas using '*'
void drawRectangle(char canvas[HEIGHT][WIDTH], int x, int y, int width,
                   int height) {
  if (width <= 0 || height <= 0) {
    return;
  }

  // Draw top and bottom horizontal lines
  for (int i = 0; i < width; i++) {
    int px = x + i;
    if (px >= 0 && px < WIDTH) {
      if (y >= 0 && y < HEIGHT) {
        canvas[y][px] = '*';
      }
      if (y + height - 1 >= 0 && y + height - 1 < HEIGHT) {
        canvas[y + height - 1][px] = '*';
      }
    }
  }

  // Draw left and right vertical lines
  for (int j = 0; j < height; j++) {
    int py = y + j;
    if (py >= 0 && py < HEIGHT) {
      if (x >= 0 && x < WIDTH) {
        canvas[py][x] = '*';
      }
      if (x + width - 1 >= 0 && x + width - 1 < WIDTH) {
        canvas[py][x + width - 1] = '*';
      }
    }
  }
}

// Function to draw a line using Bresenham's Line Algorithm
void drawLine(char canvas[HEIGHT][WIDTH], int x1, int y1, int x2, int y2) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;

  while (1) {
    if (x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT) {
      canvas[y1][x1] = '*';
    }
    if (x1 == x2 && y1 == y2) {
      break;
    }
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}

// Helper to plot symmetric circle points
static void drawCirclePoints(char canvas[HEIGHT][WIDTH], int cx, int cy, int x,
                             int y) {
  int px[] = {cx + x, cx - x, cx + x, cx - x, cx + y, cx - y, cx + y, cx - y};
  int py[] = {cy + y, cy + y, cy - y, cy - y, cy + x, cy + x, cy - x, cy - x};
  for (int i = 0; i < 8; i++) {
    if (px[i] >= 0 && px[i] < WIDTH && py[i] >= 0 && py[i] < HEIGHT) {
      canvas[py[i]][px[i]] = '*';
    }
  }
}

// Function to draw a circle using the Midpoint Circle Algorithm
void drawCircle(char canvas[HEIGHT][WIDTH], int cx, int cy, int radius) {
  if (radius < 0) {
    return;
  }
  int x = 0;
  int y = radius;
  int d = 3 - 2 * radius;

  drawCirclePoints(canvas, cx, cy, x, y);
  while (y >= x) {
    x++;
    if (d > 0) {
      y--;
      d = d + 4 * (x - y) + 10;
    } else {
      d = d + 4 * x + 6;
    }
    drawCirclePoints(canvas, cx, cy, x, y);
  }
}

// Function to draw a triangle by drawing lines between vertices
void drawTriangle(char canvas[HEIGHT][WIDTH], int x1, int y1, int x2, int y2,
                  int x3, int y3) {
  drawLine(canvas, x1, y1, x2, y2);
  drawLine(canvas, x2, y2, x3, y3);
  drawLine(canvas, x3, y3, x1, y1);
}

// Clears grid and draws all current active shapes in order
void redrawCanvas(char canvas[HEIGHT][WIDTH], Shape list[], int count) {
  clearCanvas(canvas);
  for (int i = 0; i < count; i++) {
    switch (list[i].type) {
    case SHAPE_RECTANGLE:
      drawRectangle(canvas, list[i].data.rect.x, list[i].data.rect.y,
                    list[i].data.rect.width, list[i].data.rect.height);
      break;
    case SHAPE_LINE:
      drawLine(canvas, list[i].data.line.x1, list[i].data.line.y1,
               list[i].data.line.x2, list[i].data.line.y2);
      break;
    case SHAPE_CIRCLE:
      drawCircle(canvas, list[i].data.circle.cx, list[i].data.circle.cy,
                 list[i].data.circle.radius);
      break;
    case SHAPE_TRIANGLE:
      drawTriangle(canvas, list[i].data.triangle.x1, list[i].data.triangle.y1,
                   list[i].data.triangle.x2, list[i].data.triangle.y2,
                   list[i].data.triangle.x3, list[i].data.triangle.y3);
      break;
    }
  }
}

// --- Console I/O Helper Functions ---

// Clears any leftover characters from standard input stream
void clearInputBuffer() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}

// Prompt and safely read an integer, flushing input buffer afterwards
int getIntegerInput(const char *prompt, int *val) {
  printf("%s", prompt);
  int res = scanf("%d", val);
  clearInputBuffer();
  if (res != 1) {
    return 0;
  }
  return 1;
}

// Pause and wait for the user to press Enter
void waitEnter() {
  printf("\nPress Enter to continue...");
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}

// Displays all currently managed shapes in the command console
void printShapes(Shape list[], int count) {
  printf("--- Active Shapes ---\n");
  if (count == 0) {
    printf("(none)\n");
    return;
  }
  for (int i = 0; i < count; i++) {
    printf("[%d] ", list[i].id);
    switch (list[i].type) {
    case SHAPE_RECTANGLE:
      printf("Rectangle: top-left(%d, %d), size %dx%d\n", list[i].data.rect.x,
             list[i].data.rect.y, list[i].data.rect.width,
             list[i].data.rect.height);
      break;
    case SHAPE_LINE:
      printf("Line: start(%d, %d), end(%d, %d)\n", list[i].data.line.x1,
             list[i].data.line.y1, list[i].data.line.x2, list[i].data.line.y2);
      break;
    case SHAPE_CIRCLE:
      printf("Circle: center(%d, %d), radius %d\n", list[i].data.circle.cx,
             list[i].data.circle.cy, list[i].data.circle.radius);
      break;
    case SHAPE_TRIANGLE:
      printf("Triangle: A(%d, %d), B(%d, %d), C(%d, %d)\n",
             list[i].data.triangle.x1, list[i].data.triangle.y1,
             list[i].data.triangle.x2, list[i].data.triangle.y2,
             list[i].data.triangle.x3, list[i].data.triangle.y3);
      break;
    }
  }
  printf("---------------------\n");
}

// Menu dialogue to add a shape
void addShapeMenu() {
  printf("\n--- Add a New Shape ---\n");
  printf("1. Rectangle\n");
  printf("2. Line\n");
  printf("3. Circle\n");
  printf("4. Triangle\n");
  printf("5. Back to Main Menu\n");

  int choice;
  if (!getIntegerInput("Enter your choice (1-5): ", &choice)) {
    printf("Invalid input.\n");
    return;
  }

  if (choice < 1 || choice > 4) {
    if (choice != 5) {
      printf("Invalid choice.\n");
    }
    return;
  }

  if (shapeCount >= MAX_SHAPES) {
    printf("Shape storage is full!\n");
    return;
  }

  Shape newShape;
  newShape.id = nextId++;

  if (choice == 1) {
    newShape.type = SHAPE_RECTANGLE;
    int x, y, w, h;
    if (!getIntegerInput("Enter top-left X (0-79): ", &x) ||
        !getIntegerInput("Enter top-left Y (0-24): ", &y) ||
        !getIntegerInput("Enter Width: ", &w) ||
        !getIntegerInput("Enter Height: ", &h)) {
      printf("Failed to read values.\n");
      return;
    }
    newShape.data.rect.x = x;
    newShape.data.rect.y = y;
    newShape.data.rect.width = w;
    newShape.data.rect.height = h;
  } else if (choice == 2) {
    newShape.type = SHAPE_LINE;
    int x1, y1, x2, y2;
    if (!getIntegerInput("Enter Start X1 (0-79): ", &x1) ||
        !getIntegerInput("Enter Start Y1 (0-24): ", &y1) ||
        !getIntegerInput("Enter End X2 (0-79): ", &x2) ||
        !getIntegerInput("Enter End Y2 (0-24): ", &y2)) {
      printf("Failed to read values.\n");
      return;
    }
    newShape.data.line.x1 = x1;
    newShape.data.line.y1 = y1;
    newShape.data.line.x2 = x2;
    newShape.data.line.y2 = y2;
  } else if (choice == 3) {
    newShape.type = SHAPE_CIRCLE;
    int cx, cy, r;
    if (!getIntegerInput("Enter Center X (0-79): ", &cx) ||
        !getIntegerInput("Enter Center Y (0-24): ", &cy) ||
        !getIntegerInput("Enter Radius: ", &r)) {
      printf("Failed to read values.\n");
      return;
    }
    newShape.data.circle.cx = cx;
    newShape.data.circle.cy = cy;
    newShape.data.circle.radius = r;
  } else if (choice == 4) {
    newShape.type = SHAPE_TRIANGLE;
    int x1, y1, x2, y2, x3, y3;
    if (!getIntegerInput("Enter X1 (0-79): ", &x1) ||
        !getIntegerInput("Enter Y1 (0-24): ", &y1) ||
        !getIntegerInput("Enter X2 (0-79): ", &x2) ||
        !getIntegerInput("Enter Y2 (0-24): ", &y2) ||
        !getIntegerInput("Enter X3 (0-79): ", &x3) ||
        !getIntegerInput("Enter Y3 (0-24): ", &y3)) {
      printf("Failed to read values.\n");
      return;
    }
    newShape.data.triangle.x1 = x1;
    newShape.data.triangle.y1 = y1;
    newShape.data.triangle.x2 = x2;
    newShape.data.triangle.y2 = y2;
    newShape.data.triangle.x3 = x3;
    newShape.data.triangle.y3 = y3;
  }

  shapes[shapeCount++] = newShape;
  printf("Shape added successfully with ID %d!\n", newShape.id);
}

// Menu dialogue to delete a shape
void deleteShapeMenu() {
  printf("\n--- Delete a Shape ---\n");
  int id;
  if (!getIntegerInput("Enter the ID of the shape to delete: ", &id)) {
    printf("Invalid input.\n");
    return;
  }

  int foundIndex = -1;
  for (int i = 0; i < shapeCount; i++) {
    if (shapes[i].id == id) {
      foundIndex = i;
      break;
    }
  }

  if (foundIndex == -1) {
    printf("Shape with ID %d not found.\n", id);
    return;
  }

  // Shift subsequent shapes left
  for (int i = foundIndex; i < shapeCount - 1; i++) {
    shapes[i] = shapes[i + 1];
  }
  shapeCount--;
  printf("Shape %d deleted successfully.\n", id);
}

// Menu dialogue to modify a shape
void modifyShapeMenu() {
  printf("\n--- Modify a Shape ---\n");
  int id;
  if (!getIntegerInput("Enter the ID of the shape to modify: ", &id)) {
    printf("Invalid input.\n");
    return;
  }

  int foundIndex = -1;
  for (int i = 0; i < shapeCount; i++) {
    if (shapes[i].id == id) {
      foundIndex = i;
      break;
    }
  }

  if (foundIndex == -1) {
    printf("Shape with ID %d not found.\n", id);
    return;
  }

  Shape *s = &shapes[foundIndex];
  printf("Modifying ");
  switch (s->type) {
  case SHAPE_RECTANGLE:
    printf("Rectangle [%d]\n", s->id);
    int x, y, w, h;
    if (!getIntegerInput("Enter new top-left X (0-79): ", &x) ||
        !getIntegerInput("Enter new top-left Y (0-24): ", &y) ||
        !getIntegerInput("Enter new Width: ", &w) ||
        !getIntegerInput("Enter new Height: ", &h)) {
      printf("Failed to read values.\n");
      return;
    }
    s->data.rect.x = x;
    s->data.rect.y = y;
    s->data.rect.width = w;
    s->data.rect.height = h;
    break;
  case SHAPE_LINE:
    printf("Line [%d]\n", s->id);
    int x1, y1, x2, y2;
    if (!getIntegerInput("Enter new Start X1 (0-79): ", &x1) ||
        !getIntegerInput("Enter new Start Y1 (0-24): ", &y1) ||
        !getIntegerInput("Enter new End X2 (0-79): ", &x2) ||
        !getIntegerInput("Enter new End Y2 (0-24): ", &y2)) {
      printf("Failed to read values.\n");
      return;
    }
    s->data.line.x1 = x1;
    s->data.line.y1 = y1;
    s->data.line.x2 = x2;
    s->data.line.y2 = y2;
    break;
  case SHAPE_CIRCLE:
    printf("Circle [%d]\n", s->id);
    int cx, cy, r;
    if (!getIntegerInput("Enter new Center X (0-79): ", &cx) ||
        !getIntegerInput("Enter new Center Y (0-24): ", &cy) ||
        !getIntegerInput("Enter new Radius: ", &r)) {
      printf("Failed to read values.\n");
      return;
    }
    s->data.circle.cx = cx;
    s->data.circle.cy = cy;
    s->data.circle.radius = r;
    break;
  case SHAPE_TRIANGLE:
    printf("Triangle [%d]\n", s->id);
    int tx1, ty1, tx2, ty2, tx3, ty3;
    if (!getIntegerInput("Enter new X1 (0-79): ", &tx1) ||
        !getIntegerInput("Enter new Y1 (0-24): ", &ty1) ||
        !getIntegerInput("Enter new X2 (0-79): ", &tx2) ||
        !getIntegerInput("Enter new Y2 (0-24): ", &ty2) ||
        !getIntegerInput("Enter new X3 (0-79): ", &tx3) ||
        !getIntegerInput("Enter new Y3 (0-24): ", &ty3)) {
      printf("Failed to read values.\n");
      return;
    }
    s->data.triangle.x1 = tx1;
    s->data.triangle.y1 = ty1;
    s->data.triangle.x2 = tx2;
    s->data.triangle.y2 = ty2;
    s->data.triangle.x3 = tx3;
    s->data.triangle.y3 = ty3;
    break;
  }
  printf("Shape %d modified successfully.\n", id);
}

// --- Main Application Loop ---

int main() {
  while (1) {
    // 1. Redraw all shapes onto the canvas
    redrawCanvas(grid, shapes, shapeCount);

    // 2. Clear terminal and print current view
    printf("\033[H\033[J");
    printf("================== C 2D Vector Graphics Canvas (%dx%d) "
           "==================\n",
           WIDTH, HEIGHT);
    displayCanvas(grid);
    printf("==================================================================="
           "======\n");

    // 3. Print active shape registry
    printShapes(shapes, shapeCount);

    // 4. Menu operations
    printf("Options:\n");
    printf("1. Add Shape\n");
    printf("2. Delete Shape\n");
    printf("3. Modify Shape\n");
    printf("4. Exit\n");

    int option;
    if (!getIntegerInput("Select an option (1-4): ", &option)) {
      printf("Invalid selection.\n");
      waitEnter();
      continue;
    }

    if (option == 1) {
      addShapeMenu();
      waitEnter();
    } else if (option == 2) {
      deleteShapeMenu();
      waitEnter();
    } else if (option == 3) {
      modifyShapeMenu();
      waitEnter();
    } else if (option == 4) {
      printf("Exiting program. Goodbye!\n");
      break;
    } else {
      printf("Unknown option.\n");
      waitEnter();
    }
  }
  return 0;
}