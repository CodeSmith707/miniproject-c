#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ncurses.h>
#include <string.h>

#define WIDTH 80
#define HEIGHT 25
#define MAX_SHAPES 100

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
Shape shapes[MAX_SHAPES];
int shapeCount = 0;
int nextId = 1;

// Ncurses sub-window objects
WINDOW *canvas_win = NULL;
WINDOW *sidebar_win = NULL;
WINDOW *input_win = NULL;

// --- Canvas Utility Functions ---

// Fills the entire grid with the empty space character '_'
void clearCanvas(char canvas[HEIGHT][WIDTH]) {
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      canvas[y][x] = '_';
    }
  }
}

// --- Shape Drawing Algorithms ---

// Function to draw a rectangle on the canvas using '*'
void drawRectangle(char canvas[HEIGHT][WIDTH], int x, int y, int width, int height) {
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
static void drawCirclePoints(char canvas[HEIGHT][WIDTH], int cx, int cy, int x, int y) {
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
void drawTriangle(char canvas[HEIGHT][WIDTH], int x1, int y1, int x2, int y2, int x3, int y3) {
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

// --- Ncurses Window Styling Utilities ---

// Draws borders with optional title and color formatting
void drawWinBorder(WINDOW *win, const char *title, int color_pair) {
  wattron(win, COLOR_PAIR(color_pair));
  box(win, 0, 0);
  if (title != NULL) {
    mvwprintw(win, 0, 2, " %s ", title);
  }
  wattroff(win, COLOR_PAIR(color_pair));
  wrefresh(win);
}

// Renders the 2D grid pixels inside canvas window using custom color pairs
void renderCanvasWin(WINDOW *win, char canvas[HEIGHT][WIDTH]) {
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      char ch = canvas[y][x];
      if (ch == '*') {
        wattron(win, COLOR_PAIR(1) | A_BOLD); // Highlight actual shape pixels
        mvwaddch(win, y + 1, x + 1, ch);
        wattroff(win, COLOR_PAIR(1) | A_BOLD);
      } else {
        wattron(win, COLOR_PAIR(3)); // Dim blue for empty background cells
        mvwaddch(win, y + 1, x + 1, ch);
        wattroff(win, COLOR_PAIR(3));
      }
    }
  }
  wrefresh(win);
}

// Renders the hotkey lists and active shape descriptions in the sidebar window
void renderSidebarWin(WINDOW *win, Shape list[], int count) {
  werase(win);
  drawWinBorder(win, "REGISTRY & CONTROLS", 2);
  
  wattron(win, COLOR_PAIR(4) | A_BOLD);
  mvwprintw(win, 2, 2, "KEYBOARD SHORTCUTS:");
  wattroff(win, COLOR_PAIR(4) | A_BOLD);
  
  mvwprintw(win, 3, 2, "[a] Add Vector Shape");
  mvwprintw(win, 4, 2, "[d] Delete Shape by ID");
  mvwprintw(win, 5, 2, "[m] Modify Shape by ID");
  mvwprintw(win, 6, 2, "[c] Clear All Shapes");
  mvwprintw(win, 7, 2, "[q] Quit Graphics Editor");
  
  wattron(win, COLOR_PAIR(4) | A_BOLD);
  mvwprintw(win, 9, 2, "ACTIVE SHAPES (%d/%d):", count, MAX_SHAPES);
  wattroff(win, COLOR_PAIR(4) | A_BOLD);
  
  int start_row = 11;
  int max_rows = 25;
  for (int i = 0; i < count && (start_row + i) < max_rows; i++) {
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win, start_row + i, 2, "[%d]", list[i].id);
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
    
    switch (list[i].type) {
      case SHAPE_RECTANGLE:
        mvwprintw(win, start_row + i, 7, " Rect (%d,%d) %dx%d",
                  list[i].data.rect.x, list[i].data.rect.y,
                  list[i].data.rect.width, list[i].data.rect.height);
        break;
      case SHAPE_LINE:
        mvwprintw(win, start_row + i, 7, " Line (%d,%d)-(%d,%d)",
                  list[i].data.line.x1, list[i].data.line.y1,
                  list[i].data.line.x2, list[i].data.line.y2);
        break;
      case SHAPE_CIRCLE:
        mvwprintw(win, start_row + i, 7, " Circ (%d,%d) r=%d",
                  list[i].data.circle.cx, list[i].data.circle.cy,
                  list[i].data.circle.radius);
        break;
      case SHAPE_TRIANGLE:
        mvwprintw(win, start_row + i, 7, " Tri A(%d,%d) B(%d,%d)",
                  list[i].data.triangle.x1, list[i].data.triangle.y1,
                  list[i].data.triangle.x2, list[i].data.triangle.y2);
        break;
    }
  }
  wrefresh(win);
}

// --- TUI Form Dialogues ---

// Prompt and safely grab an integer value inside bottom bar panel
int getTUIInteger(WINDOW *win, const char *prompt, int *val) {
  werase(win);
  drawWinBorder(win, "PROMPT & INPUT BAR", 2);
  
  wattron(win, COLOR_PAIR(4) | A_BOLD);
  mvwprintw(win, 1, 2, "%s", prompt);
  wattroff(win, COLOR_PAIR(4) | A_BOLD);
  
  wrefresh(win);
  
  // Enable terminal echo and text cursor
  echo();
  curs_set(1);
  
  int res = mvwscanw(win, 1, 2 + strlen(prompt), "%d", val);
  
  noecho();
  curs_set(0);
  
  // Refresh standard layout state
  werase(win);
  drawWinBorder(win, "PROMPT & INPUT BAR", 2);
  wrefresh(win);
  
  return (res == 1);
}

// Interactive wizard to insert a vector shape
void addShapeTUI() {
  werase(input_win);
  drawWinBorder(input_win, "ADD VECTOR SHAPE", 2);
  
  wattron(input_win, COLOR_PAIR(4) | A_BOLD);
  mvwprintw(input_win, 1, 2, "Choose Type: 1=Rect, 2=Line, 3=Circle, 4=Triangle, 5=Cancel: ");
  wattroff(input_win, COLOR_PAIR(4) | A_BOLD);
  wrefresh(input_win);
  
  echo();
  curs_set(1);
  int choice = 0;
  mvwscanw(input_win, 1, 62, "%d", &choice);
  noecho();
  curs_set(0);
  
  werase(input_win);
  drawWinBorder(input_win, "PROMPT & INPUT BAR", 2);
  wrefresh(input_win);
  
  if (choice < 1 || choice > 4) {
    return;
  }
  
  if (shapeCount >= MAX_SHAPES) {
    werase(input_win);
    drawWinBorder(input_win, "ERROR", 4);
    mvwprintw(input_win, 1, 2, "Storage limit reached! Press any key...");
    wrefresh(input_win);
    wgetch(input_win);
    return;
  }
  
  Shape newShape;
  newShape.id = nextId++;
  
  if (choice == 1) {
    newShape.type = SHAPE_RECTANGLE;
    int x = 0, y = 0, w = 0, h = 0;
    if (!getTUIInteger(input_win, "Top-Left X (0-79): ", &x) ||
        !getTUIInteger(input_win, "Top-Left Y (0-24): ", &y) ||
        !getTUIInteger(input_win, "Rectangle Width: ", &w) ||
        !getTUIInteger(input_win, "Rectangle Height: ", &h)) {
      return;
    }
    newShape.data.rect.x = x;
    newShape.data.rect.y = y;
    newShape.data.rect.width = w;
    newShape.data.rect.height = h;
  } else if (choice == 2) {
    newShape.type = SHAPE_LINE;
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    if (!getTUIInteger(input_win, "Start Coordinate X1 (0-79): ", &x1) ||
        !getTUIInteger(input_win, "Start Coordinate Y1 (0-24): ", &y1) ||
        !getTUIInteger(input_win, "End Coordinate X2 (0-79): ", &x2) ||
        !getTUIInteger(input_win, "End Coordinate Y2 (0-24): ", &y2)) {
      return;
    }
    newShape.data.line.x1 = x1;
    newShape.data.line.y1 = y1;
    newShape.data.line.x2 = x2;
    newShape.data.line.y2 = y2;
  } else if (choice == 3) {
    newShape.type = SHAPE_CIRCLE;
    int cx = 0, cy = 0, r = 0;
    if (!getTUIInteger(input_win, "Center Coordinate CX (0-79): ", &cx) ||
        !getTUIInteger(input_win, "Center Coordinate CY (0-24): ", &cy) ||
        !getTUIInteger(input_win, "Circle Radius (R): ", &r)) {
      return;
    }
    newShape.data.circle.cx = cx;
    newShape.data.circle.cy = cy;
    newShape.data.circle.radius = r;
  } else if (choice == 4) {
    newShape.type = SHAPE_TRIANGLE;
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
    if (!getTUIInteger(input_win, "Vertex 1 X1 (0-79): ", &x1) ||
        !getTUIInteger(input_win, "Vertex 1 Y1 (0-24): ", &y1) ||
        !getTUIInteger(input_win, "Vertex 2 X2 (0-79): ", &x2) ||
        !getTUIInteger(input_win, "Vertex 2 Y2 (0-24): ", &y2) ||
        !getTUIInteger(input_win, "Vertex 3 X3 (0-79): ", &x3) ||
        !getTUIInteger(input_win, "Vertex 3 Y3 (0-24): ", &y3)) {
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
}

// Dynamic prompt to erase a shape by unique ID
void deleteShapeTUI() {
  int id = 0;
  if (!getTUIInteger(input_win, "Enter Vector ID to erase: ", &id)) {
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
    werase(input_win);
    drawWinBorder(input_win, "ERROR", 4);
    mvwprintw(input_win, 1, 2, "ID not found! Press any key...");
    wrefresh(input_win);
    wgetch(input_win);
    return;
  }
  
  for (int i = foundIndex; i < shapeCount - 1; i++) {
    shapes[i] = shapes[i + 1];
  }
  shapeCount--;
}

// Dialogue to adjust properties of an existing vector shape
void modifyShapeTUI() {
  int id = 0;
  if (!getTUIInteger(input_win, "Enter Vector ID to modify: ", &id)) {
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
    werase(input_win);
    drawWinBorder(input_win, "ERROR", 4);
    mvwprintw(input_win, 1, 2, "ID not found! Press any key...");
    wrefresh(input_win);
    wgetch(input_win);
    return;
  }
  
  Shape *s = &shapes[foundIndex];
  if (s->type == SHAPE_RECTANGLE) {
    int x = 0, y = 0, w = 0, h = 0;
    if (!getTUIInteger(input_win, "New Top-Left X (0-79): ", &x) ||
        !getTUIInteger(input_win, "New Top-Left Y (0-24): ", &y) ||
        !getTUIInteger(input_win, "New Width: ", &w) ||
        !getTUIInteger(input_win, "New Height: ", &h)) {
      return;
    }
    s->data.rect.x = x;
    s->data.rect.y = y;
    s->data.rect.width = w;
    s->data.rect.height = h;
  } else if (s->type == SHAPE_LINE) {
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    if (!getTUIInteger(input_win, "New Start X1 (0-79): ", &x1) ||
        !getTUIInteger(input_win, "New Start Y1 (0-24): ", &y1) ||
        !getTUIInteger(input_win, "New End X2 (0-79): ", &x2) ||
        !getTUIInteger(input_win, "New End Y2 (0-24): ", &y2)) {
      return;
    }
    s->data.line.x1 = x1;
    s->data.line.y1 = y1;
    s->data.line.x2 = x2;
    s->data.line.y2 = y2;
  } else if (s->type == SHAPE_CIRCLE) {
    int cx = 0, cy = 0, r = 0;
    if (!getTUIInteger(input_win, "New Center CX (0-79): ", &cx) ||
        !getTUIInteger(input_win, "New Center CY (0-24): ", &cy) ||
        !getTUIInteger(input_win, "New Radius (R): ", &r)) {
      return;
    }
    s->data.circle.cx = cx;
    s->data.circle.cy = cy;
    s->data.circle.radius = r;
  } else if (s->type == SHAPE_TRIANGLE) {
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
    if (!getTUIInteger(input_win, "New Vertex 1 X1 (0-79): ", &x1) ||
        !getTUIInteger(input_win, "New Vertex 1 Y1 (0-24): ", &y1) ||
        !getTUIInteger(input_win, "New Vertex 2 X2 (0-79): ", &x2) ||
        !getTUIInteger(input_win, "New Vertex 2 Y2 (0-24): ", &y2) ||
        !getTUIInteger(input_win, "New Vertex 3 X3 (0-79): ", &x3) ||
        !getTUIInteger(input_win, "New Vertex 3 Y3 (0-24): ", &y3)) {
      return;
    }
    s->data.triangle.x1 = x1;
    s->data.triangle.y1 = y1;
    s->data.triangle.x2 = x2;
    s->data.triangle.y2 = y2;
    s->data.triangle.x3 = x3;
    s->data.triangle.y3 = y3;
  }
}

// --- Main Window Loop ---

int main() {
  // Initialize standard TUI mode
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);

  // Validate screen layout bounds
  int max_y = 0, max_x = 0;
  getmaxyx(stdscr, max_y, max_x);
  if (max_y < 30 || max_x < 120) {
    endwin();
    printf("\n[ERROR] Current Terminal is too small (%dx%d)!\n", max_x, max_y);
    printf("Please resize your terminal window to at least 120x30 columns and run the program again.\n\n");
    return 1;
  }

  // Setup color indices
  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Shape stars
    init_pair(2, COLOR_CYAN, COLOR_BLACK);    // Frames & panels
    init_pair(3, COLOR_BLUE, COLOR_BLACK);    // Empty dots '_'
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);  // Hotkeys & prompts
  }

  // Initialize panes
  canvas_win = newwin(27, 82, 0, 0);
  sidebar_win = newwin(27, 38, 0, 82);
  input_win = newwin(3, 120, 27, 0);

  // Set default initial state of the input bar
  werase(input_win);
  drawWinBorder(input_win, "PROMPT & INPUT BAR", 2);
  wrefresh(input_win);

  int running = 1;
  while (running) {
    // 1. Redraw grid logic
    redrawCanvas(grid, shapes, shapeCount);
    
    // 2. Render Left Canvas Panel
    werase(canvas_win);
    drawWinBorder(canvas_win, "2D VECTOR CANVAS (80x25)", 2);
    renderCanvasWin(canvas_win, grid);
    
    // 3. Render Right Panel
    renderSidebarWin(sidebar_win, shapes, shapeCount);
    
    // 4. Capture Keyboard Key
    int ch = wgetch(stdscr);
    switch (ch) {
      case 'a':
      case 'A':
        addShapeTUI();
        break;
      case 'd':
      case 'D':
        deleteShapeTUI();
        break;
      case 'm':
      case 'M':
        modifyShapeTUI();
        break;
      case 'c':
      case 'C':
        shapeCount = 0;
        break;
      case 'q':
      case 'Q':
        running = 0;
        break;
    }
  }

  // Clean-up and restore screen terminal mode
  endwin();
  printf("TUI Vector Graphics Editor V2 closed successfully!\n");
  return 0;
}
