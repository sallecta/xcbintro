# xcbintro

An Introduction to XCB Programming by Matt Scarpino 

[Based on codeproject.com article](https://www.codeproject.com/Articles/1089819/An-Introduction-to-XCB-Programming).

Athor: [Matt Scarpino](https://www.codeproject.com/script/Membership/View.aspx?mid=10170939)

[The Code Project Open License (CPOL) 1.02](http://www.codeproject.com/info/cpol10.aspx)


# An Introduction to XCB Programming
Developing Low-Level Linux Applications with XCB (X protocol C-language Binding)


## 0. Introduction

Years ago, I wrote graphical user interfaces (GUIs) for various platforms. For a Linux application, I tried to work with the X Window System library, commonly called Xlib. I was stunned by the amount of code required to perform simple tasks, and I decided to use a different toolset.

Today, developers can develop low-level GUIs for the X Window System using XCB, which stands for X protocol C-language Binding. An XCB program can perform the same operations as one coded with Xlib, but requires substantially less code. In addition, XCB provides "latency hiding, direct access to the protocol, improved threading support, and extensibility."

The goal of this article is to explain the basics of XCB application development. There are three central topics: creating a window, handling events, and drawing graphics. For each topic, I've provided a source file that demonstrates how the functions and data structures can be used in practice.


## 1. Preliminary Requirements

Understanding the content of this article requires a basic familiarity with C programming. To compile the example code, you need to have the XCB library (libxcb.so) and header file (xcb.h) installed on your development system.
On Ubuntu and dreivatives, you can check it by following command in terminal.

```terminal
sudo apt install libx11-xcb-dev
```


## 2. The X Window System

Though Wayland and Mir are growing in popularity, the X Window System (also called X Windows or just X) remains the most common framework for managing windows on Linux computers. Originally developed by MIT in the 1980s, it has become widely adopted on UNIX and Linux computers, and it can be accessed on Windows computers through Cygwin. The goal of XCB and Xlib is to enable programmers to access the X Window System in their applications.

When dealing with X, the first thing to understand is the difference between displays, screens, and windows. According to the official documentation, a display is a "collection of monitors that share a common keyboard and pointer." A screen identifies a physical monitor and a window is a graphical element drawn on a monitor. It's important to understand that a display can have multiple screens and each screen can have multiple windows.

The second thing to understand about X is its use of client-server communication. X supports remote displays, and given the usual client-server model, you might expect that the user's system receives display information from a remote X server. But that's not how X works. The X server runs on the user's computer and receives display information from client applications. In most cases, the client and server run on the same system.

For each screen, the X server manages a window that occupies the entire area. This is called the root window. When client applications connect to the server, the server allows them to create child windows. If a user performs an action while a child window has focus, the X server will provide the client application with information about the user's action. This information is provided in the form of an event.

Each resource managed by an X server has an identifier. Each display has a name and each screen has a number. Before an application can create a new window or resource, it needs to obtain a suitable ID.


## 3. Creating a Window

Most XCB applications start by performing three fundamental tasks:

- Connect to the X Server
- Access a screen
- Create and display a window in the screen

The following discussion explains how these tasks can be accomplished. The last part of the section presents an application that displays a simple window for five seconds.


### 3.1  Connect to the X Server

Whether you program in Xlib or XCB, an application's first step usually involves connecting to the X server. In XCB, the function to use is xcb_connect:

```c
xcb_connection_t* xcb_connect(const char *display, int *screen);
````

The first argument identifies the X server's display name. If this is set to NULL, the function will use the value of the DISPLAY environment variable.

The second argument points to a number for the screen that should be connected. If this is set to NULL, the function will set the screen number to 0. This represents the first available screen.

The xcb_connection_t structure is opaque, so there's no way to know whether the connection was created successfully. For this reason, XCB provides the function xcb_connection_has_error, whose declaration is given as follows:

```c
int xcb_connection_has_error(xcb_connection_t *c);
````

This accepts the xcb_connection_t* returned by xcb_connect and returns a value that identifies if the connection was established. An error will result in a non-zero value.

The following code shows how these two functions can be used in practice:

```c
xcb_connection_t *conn;

conn = xcb_connect(NULL, NULL);

if (xcb_connection_has_error(conn)) {
  printf("Error opening display.\n");
  exit(1);
}
````
This code creates a connection to the X server whose name is given by the DISPLAY environment variable. The screen number is 0.


### 3.2  Access the Screen

Before an application can create a window, it needs to access a screen. To do this, the first step is to access properties of the X server and its display environment. This is made possible by xcb_get_setup:

const xcb_setup_t* xcb_get_setup(xcb_connection_t *c);

The return value is an xcb_setup_t, which provides a number of useful fields, including the following:

 - roots_len — the number of root windows managed by the X server
 - bitmap_format_scanline_unit — the number of bits in a scanline unit
 - bitmap_format_scanline_pad — the number of bits used to pad each scanline
 - bitmap_format_bit_order — identifies whether the leftmost bit in the screen is the least significant bit or most significant bit
 - protocol_major_version — major version of the supported X Window System protocol
 - protocol_minor_version — minor version or the supported X Window System protocol

This xcb_setup_t is important because it allows us to access the X server's screens. This access is made possible by another function called xcb_setup_roots_iterator:

xcb_screen_iterator_t xcb_setup_roots_iterator(xcb_setup_t *set);

The xcb_screen_iterator_t structure has a data field of type xcb_screen_t*, which represents a screen. The fields of this structure include the following:

- root — the root window ID
- root_depth — bits per pixel in the screen
- root_visual — pointer to a xcb_visualid_t structure containing the screen's color mapping
- width_in_pixels — screen width in pixels
- height_in_pixels — screen height in pixels
- width_in_millimeters — screen width in millimeters
- height_in_millimeters — screen height in millimeters
- black_pixel — value corresponding to the screen's black pixel
- white_pixel — value corresponding to the screen's white

The following code shows how these structures are used. It obtains an xcb_setup_t structure from an existing connection (conn), accesses the first screen, and prints the screen's dimensions in pixels:

const xcb_setup_t* setup;
xcb_screen_t* screen;

setup = xcb_get_setup(conn);
screen = xcb_setup_roots_iterator(setup).data;
printf("Screen dimensions: %d, %d\n", screen->width_in_pixels, screen->height_in_pixels);


Obtaining the screen is necessary to create the application's window. The following discussion explains how this can be done.


### 3.3  Create and Display the Window

Now that we've accessed the screen, the next step is to create a window. The main function to use is xcb_create_window:

```c
xcb_create_window(xcb_connection_t *conn,          // Connection to X server
                  uint8_t           depth,         // Screen depth
                  xcb_window_t      window_id,     // ID of the window
                  xcb_window_t      parent_id,     // ID of the parent window
                  int16_t           x,             // Top-left x-coordinate
                  int16_t           y,             // Top-left y-coordinate
                  uint16_t          width,         // Window width in pixels
                  uint16_t          height,        // Window height in pixels
                  uint16_t          border_width,  // Window border width in pixels
                  uint16_t          class,
                  xcb_visualid_t    visual,
                  uint32_t          value_mask,
                  const uint32_t   *value_list);
```

Most of the function's arguments are straightforward. window_id is a unique identifier for the window, and it can be obtained by calling xcb_generate_id, whose signature is as follows:

```c
uint32_t xcb_generate_id(xcb_connection_t* conn)
````

parent_id can be set to the root field of the xcb_screen_t structure discussed earlier. Similarly, the visual argument can be set to the screen's root_visual field.

The function's class argument is set to a value of the xcb_window_class_t enumerated type. This is undocumented, but it's usually safe to set class equal to XCB_WINDOW_CLASS_INPUT_OUTPUT.

The value_mask and value_list arguments are related. value_mask identifies an OR'ed series of property names and value_list contains their values. The property identifiers are taken from the xcb_cw_t type, whose values can be found here. One important property is XCB_CW_BACK_PIXEL, which sets the window's background color.

After creating a window, xcb_map_window must be called to make it visible. The signature of this function is given as:

```c
xcb_void_cookie_t xcb_map_window(xcb_connection_t *conn, xcb_window_t window) 
```

After calling xcb_map_window, it's common to call xcb_flush to force the window request to be sent to the server. The code in the following discussion demonstrates how this is used.


### 3.4  Example - Simple Window

The code in the simple_window.c file creates and displays a simple 100x100 pixel window. The main function is given as follows:

```c
#include <xcb/xcb.h>

int main(void) {

  xcb_connection_t   *conn;
  const xcb_setup_t  *setup;
  xcb_screen_t       *screen;
  xcb_window_t       window_id;
  uint32_t           prop_name;
  uint32_t           prop_value;

  /* Connect to X server */
  conn = xcb_connect(NULL, NULL);
  if(xcb_connection_has_error(conn)) {
    printf("Error opening display.\n");
    exit(1);
  }

  /* Obtain setup info and access the screen */
  setup = xcb_get_setup(conn);
  screen = xcb_setup_roots_iterator(setup).data;

  /* Create window */
  window_id = xcb_generate_id(conn);
  prop_name = XCB_CW_BACK_PIXEL;
  prop_value = screen->white_pixel;

  xcb_create_window(conn, screen->root_depth, 
     window_id, screen->root, 0, 0, 100, 100, 1,
     XCB_WINDOW_CLASS_INPUT_OUTPUT, 
     screen->root_visual, prop_name, &prop_value);

  /* Display the window */
  xcb_map_window(conn, window_id);
  xcb_flush(conn);

  /* Wait for 5 seconds */
  sleep(5);

  /* Disconnect from X server */
  xcb_disconnect(conn);
  return 0;
}
```

There are at least three interesting points to notice about this code:

- The window's position is (0, 0) and its size is 100x100 pixels. This is configured by the arguments in xcb_create_window.
- The window's background color is set to white by associating the XCB_CW_BACK_PIXEL property with the value screen->white_pixel.
- This code doesn't explicitly close the window. Instead, it calls xcb_disconnect to terminate the connection to the X server.

If XCB is installed on the development system, the source file can be compiled with the following command:

```console
mkdir build; gcc -o build/simple_window src/simple_window.c -lxcb
```

Unless the user clicks the window's close button, the window will stay open for five seconds because of the sleep function. Without this function, the window will close immediately. Rather than call sleep, most XCB applications have an event loop. The following discussion explains how events work.


## 4. Handling Events

When the user performs an action involving a child window, such as clicking the mouse or pressing a key, the X server sends a message to the client application. The application can retrieve this message by calling xcb_wait_for_event:

```c
xcb_generic_event_t *xcb_wait_for_event(xcb_connection_t *c);
```

The xcb_generic_event_t structure has a field, response_type, that identifies which event took place. XCB supports over thirty different types of events, and six event codes are given as follows:

- XCB_BUTTON_PRESS — mouse button pressed down
- XCB_BUTTON_RELEASE — mouse button raised
- XCB_MOTION_NOTIFY — mouse motion
- XCB_KEY_PRESS — key pressed down
- XCB_KEY_RELEASE — key released
- XCB_EXPOSE — window displays content

To configure event handling, the value_mask and value_list arguments of xcb_create_window must identify events of interest. To be specific, value_mask must be set to an OR'ed combination that contains XCB_CW_EVENT_MASK. The corresponding entry in value_list must contain an OR'ed combination that identifies which events the application should receive.

For example, the following code configures the new window to pay attention to mouse clicks, key presses, and exposure events:

```c
value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

value_list[0] = screen->white_pixel;

value_list[1] = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_PRESS | 
                XCB_EVENT_MASK_EXPOSURE;
```

The first entry in value_list identifies the background color (which corresponds to XCB_BACK_PIXEL). The second entry contains an OR'ed combination of values that identify the events of interest.

After a window has been configured to receive events, xcb_wait_for_event can be called to receive messages from the X server. This function halts the application until an event occurs, and is usually invoked in a loop:

```c
xcb_generic_event_t event;

while((event = xcb_wait_for_event(conn)) ) {
  switch (event->response_type) {

  // Respond if the event is a key press
  case XCB_KEY_PRESS:
    ...
    break;
  
 // Respond if the event is a mouse click
  case XCB_BUTTON_PRESS:
    ...
    break;
  
  // Respond if the event is something else
  default:
    ...
    break;

  free (event);
}
```

In this code, the while loop calls xcb_wait_for_event after each event is received and processed. This means the loop never terminates. It would be nice to have a "special quit event," as promised in the official XCB documentation. But I've never seen this used.

Instead, it's common to insert a boolean value in the condition of the while loop. The loop will terminate when the value returns true. The example code at the end of this section will demonstrate how this works.


### 4.1  Responding to Mouse Clicks

If a user clicks a mouse button, the message from the X server can be accessed as a xcb_button_press_event_t, which contains fields related to the mouse event. Its fields include the following:

- detail — the button pressed
- time — the timestamp of the mouse click
- event_x, event_y — event coordinates relative to the client window
- root_x, root_y — event coordinates relative to the root window
- root — ID of the root window
- child — ID of the child window

detail is a value of the xcb_button_index_t enumerated type, which has six values:

1. XCB_BUTTON_INDEX_ANY — any mouse button
2. XCB_BUTTON_INDEX_1 — the left mouse button
3. XCB_BUTTON_INDEX_2 — the middle mouse button
4. XCB_BUTTON_INDEX_3 — the right mouse button
5. XCB_BUTTON_INDEX_4 — mouse scroll up
6. XCB_BUTTON_INDEX_5 — mouse scroll down

The following code shows how an application can access information about a mouse event:

```c
while((event = xcb_wait_for_event(conn)) ) {
  switch(event->response_type) {
    ...
    case XCB_BUTTON_PRESS:
      printf("Button pressed: %d\n", ((xcb_button_press_event_t*)event)->detail);
      break;
  }
}
```


### 4.2  Responding to Key Presses

If the user presses a key, the X server's event message can be accessed as a xcb_key_press_event_t, whose fields provide information related to the keystroke. These fields include the following:

- detail — a value identifying the pressed key
- time — the timestamp of the keypress
- root — ID of the root window
- child — ID of the child window

You might expect that detail would identify the key using ASCII or Unicode. Alas, this is not the case. To determine the pressed key, an application needs to call functions from the xcb_keysyms.h header. These functions aren't documented and I can find few examples of their use. Therefore, this article won't explain them in any detail.


### 4.3  Example - Event Window

The code in event_window.c creates the same white window as in simple_window.c. The difference is that the application configures the window to receive events and then handles events in an event loop. The following code shows how the window is configured:

```c
  /* Create window */
  window_id = xcb_generate_id(conn);
  value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  value_list[0] = screen->white_pixel;
  value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | 
                  XCB_EVENT_MASK_KEY_PRESS;

  xcb_create_window(conn, screen->root_depth, 
     window_id, screen->root, 0, 0, 100, 100, 1,
     XCB_WINDOW_CLASS_INPUT_OUTPUT, 
     screen->root_visual, value_mask, value_list);
```


As shown, the values in value_mask specify that the window's background and event handling will be configured. The values in value_list specify that the background should be white and that the application should receive events related to mouse clicks, key presses, and exposure. The following code shows how these events are handled in an event loop.

```c
  /* Execute the event loop */
  while (!finished && (event = xcb_wait_for_event(conn))) {

    switch(event->response_type) {

      /* Respond to key presses */
      case XCB_KEY_PRESS:
        printf("Keycode: %d\n", ((xcb_key_press_event_t*)event)->detail);
        finished = 1;
        break;

      /* Respond to button presses */
      case XCB_BUTTON_PRESS:
        printf("Button pressed: %u\n", ((xcb_button_press_event_t*)event)->detail);
        printf("X-coordinate: %u\n", ((xcb_button_press_event_t*)event)->event_x);
        printf("Y-coordinate: %u\n", ((xcb_button_press_event_t*)event)->event_y);
        break;

      case XCB_EXPOSE:
        break;
    }
    free(event);
  }
```

If the user clicks a mouse button, the application prints the button number and the click's coordinates. If the user presses a key, the application prints the key code and sets finished to 1. When finished changes from 0 to 1, the while loop terminates and the application disconnects from the X server.


## 5. Drawing Graphics

Blank windows aren't particularly interesting. XCB makes it straightforward to add shapes to a window, and the process involves two steps:

1. Create a graphics context
2. Call one or more drawing primitives.

This section discusses both steps. Then we'll look at an application that adds shapes to the simple window presented earlier.

It's important to note that XCB doesn't have any functions that create GUI widgets like buttons or text boxes. If you want to build a traditional user interface, you're better off using tools like GTK+, FLTK, or Qt.


### 5.1  Creating a Graphics Context

A graphics context holds a window's graphic configuration, such as the font, line style, and foreground color. This structure can be created with xcb_create_gc:

```c
xcb_create_gc(xcb_connection_t *conn,
              xcb_gcontext_t    context_id,
              xcb_drawable_t    drawable,
              uint32_t          value_mask,
              const uint32_t   *value_list);
```

The context_id is a unique identifier for the context. It can be obtained by calling xcb_generate_id.

The drawable parameter can be set equal to the screen's root window. This is provided by the root field of the xcb_screen_t structure discussed earlier.

The last two arguments, value_mask and value_list, are similar to the value_mask and value_list arguments of xcb_create_window. The difference is that the property names are taken from the xcb_gc_t enumerated type, and the full list can be found here. 

Two important properties of a graphic context involve the foreground color and exposure events. The context's foreground color can be set by adding XCB_GC_FOREGROUND to value_mask and by setting the first value of value_list to the appropriate color. In addition, the XCB_GC_GRAPHICS_EXPOSURES property configures whether the context generates exposure events. This can be turned on or off by setting the second value of value_list to 0 or 1. The following code shows how this works:

```c
value_mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
value_list[0] = s->black_pixel;
value_list[1] = 0;
```

This sets the context's foreground color to black and turns off generation of exposure events. These settings will be used throughout this article.


### 5.2  Drawing Primitives

XCB provides eight functions that draw shapes in a window. These are called drawing primitives and Table 1 lists each of their signatures:

|                                                                                         Function                                                                                        |                 Description                 |
|:-------------------------------------------------------------------------------------------------------------------------------------------------------:|:----------------------------------------:|
| xcb_poly_point(  xcb_connection_t *conn,  uint8_t coord_mode,  xcb_drawable_t window,  xcb_gcontext_t context_id,  uint32_t num_points,  const xcb_point_t *points)  | Draws one or more points                    |
| xcb_poly_line(   xcb_connection_t *conn,   uint8_t coord_mode,   xcb_drawable_t window,   xcb_gcontext_t context_id,   uint32_t num_points,   const xcb_point_t *points)  | Draws connected lines between points        |
| xcb_poly_segment(   xcb_connection_t *conn,  xcb_drawable_t window,   xcb_gcontext_t context_id,   uint32_t num_segments,   const xcb_segment_t *segments) | Draws distinct line segments                |
| xcb_poly_rectangle(   xcb_connection_t *conn,   xcb_drawable_t window,   xcb_gcontext_t context_id,   uint32_t num_rects,   const xcb_rectangle_t *rects) | Draws one or more rectangles                |
| xcb_poly_arc(   xcb_connection_t *conn,   xcb_drawable_t window,   xcb_gcontext_t context_id,   uint32_t num_arcs,   const xcb_segment_t *arcs)  | Draws one or more elliptical arcs           |
| xcb_fill_poly(   xcb_connection_t *conn,   xcb_drawable_t window,   xcb_gcontext_t context_id,   uint8_t shape,   uint8_t coord_mode,   uint32_t num_points   const xcb_point_t *points) | Draws and fills a polygon                   |
| xcb_poly_fill_rectangle(   xcb_connection_t *conn,   xcb_drawable_t window,   xcb_gcontext_t context_id,   uint32_t num_rects,   const xcb_rectangle_t *rects)  | Draws and fills one or more rectangles      |
| xcb_poly_fill_arc(   xcb_connection_t *conn,   xcb_drawable_t window,   xcb_gcontext_t context_id,   uint32_t num_arcs,   const xcb_arc_t *arcs)   | Draws and fills one or more elliptical arcs |

Many of these functions have a coord_mode argument that specifies how the shape's coordinates should be interpreted. This can be set to one of two values:

- XCB_COORD_MODE_ORIGIN — All coordinate pairs are given relative to the origin (0, 0)
- XCB_COORD_MODE_PREVIOUS — Each coordinate pair is given relative to the preceding coordinate pair

The last argument of each drawing primitive contains an array of structures that represent different shapes (points, segments, rectangles, and arcs). The following discussion explores each of these shapes.


#### 5.2.1  Points, Lines, and Polygons

Many of the functions define their shapes using points. Each point must be given as an xcb_point_t structure, which has two integer fields: x and y. For example, the following code shows how xcb_poly_point can be called.

```c
xcb_point_t points[4] = { {40, 40}, {40, 80}, {80, 40}, {80, 80} };
xcb_poly_point(conn, XCB_COORD_MODE_ORIGIN, window_id, context_id, 4, points);
```

The xcb_poly_line and xcb_fill_poly functions work in essentially the same way. The following code calls xcb_fill_poly to create a filled pentagon:

```c
xcb_point_t points[5] = { {11, 24}, {30, 10}, {49, 24}, {42, 46}, {18, 46} }; 
xcb_fill_poly(conn, window_id, context_id, XCB_POLY_SHAPE_CONVEX, 
              XCB_COORD_MODE_ORIGIN, 5, points);
```

This code sets the shape argument of xcb_fill_poly to XCB_POLY_SHAPE_CONVEX. Other values include XCB_POLY_SHAPE_NONCONVEX and XCB_POLY_SHAPE_COMPLEX.


#### 5.2.2  Line Segments

It's important to understand the difference between xcb_poly_line and xcb_poly_segment. The first accepts an array of points and draws a line from each point to the next. In contrast, the lines drawn by xcb_poly_segment aren't necessarily connected.

The last argument of xcb_poly_segment is an array of xcb_segment_t structures. This structure has four integer fields: x1, y1, x2, y2. Therefore, each segment connects (x1, y1) to (x2, y2). The following code shows how this works:

```c
xcb_segment_t segments[2] = { {60, 20, 90, 40}, {60, 40, 90, 20} }; 
xcb_poly_segment(conn, window_id, context_id, 2, segments);
```


#### 5.2.3  Rectangles

xcb_poly_rectangle and xcb_poly_fill_rectangle both accept an array of xcb_rectangle_t structures. This structure has four fields: x, y, width, and height. The x and y fields identify the coordinates of the rectangle's upper-left corner. For example, the following code draws and fills a 30x20 pixel rectangle whose upper left corner is at (15, 65).

```c
xcb_rectangle_t rect = {15, 65, 30, 20};
xcb_poly_fill_rectangle(conn, window_id, context_id, 1, &rect);
```

#### 5.2.4  Arcs

xcb_poly_arc and xcb_poly_fill_arc both accept xcb_arc_t structures. The xcb_arc_t structure represents an arc of an ellipse, and has six fields:

- x — the x-coordinate of the upper-left corner of the rectangle containing the ellipse
- y — the y-coordinate of the upper-left corner of the rectangle containing the ellipse
- width — the width of the ellipse
- height — the height of the ellipse
- angle1 — the starting angle of the arc, given in 1/64s of a degree
- angle2 — the ending angle of the arc, given in 1/64s of a degree

The following code draws the upper-half of an ellipse whose rectangle has an upper-left corner at (60, 70) and has dimensions 30x20 pixels:

```c
xcb_arc_t arc = {60, 70, 30, 20, 0, 180 << 6};
xcb_poly_arc(conn, window_id, context_id, 1, &arc);
```
It's important to see that the angles are given in 1/64s of a degree. This is why the second angle, which is meant to equal 180 degrees, is given as 180 << 6.

#### 5.2.5  Drawing Primitives and Exposure Events

To work properly, drawing primitives must be executed after the window has been drawn. This can be accomplished by calling the functions after an exposure event takes place. The code in the following discussion will demonstrate how this works.


### 5.3  Example - Graphic Window

The code in graphic_window.c expands on the code in event_window.c by defining a series of shape structures and calling drawing primitives. The shapes are defined with the following code:

```c
xcb_point_t points[5] = { {11, 24}, {30, 10}, {49, 24}, {42, 46}, {18, 46} }; 
xcb_segment_t segments[2] = { {60, 20, 90, 40}, {60, 40, 90, 20} };
xcb_rectangle_t rect = {15, 65, 30, 20};
xcb_arc_t arc = {60, 70, 30, 20, 0, 180 << 6};
```

The following code in graphic_window.c shows how drawing primitives can be called inside of the event loop. Each exposure event draws a pentagon, two line segments, a rectangle, and an elliptical arc.

```c
  /* Execute the event loop */
  while (!finished && (event = xcb_wait_for_event(conn))) {

    switch(event->response_type) {

      case XCB_KEY_PRESS:
        printf("Keycode: %d\n", ((xcb_key_press_event_t*)event)->detail);
        finished = 1;
        break;

      case XCB_BUTTON_PRESS:
        printf("Button pressed: %u\n", ((xcb_button_press_event_t*)event)->detail);
        printf("X-coordinate: %u\n", ((xcb_button_press_event_t*)event)->event_x);
        printf("Y-coordinate: %u\n", ((xcb_button_press_event_t*)event)->event_y);
        break;

      case XCB_EXPOSE:

        /* Draw polygon */
        xcb_fill_poly(conn, window_id, context_id, XCB_POLY_SHAPE_CONVEX, 
                      XCB_COORD_MODE_ORIGIN, 5, points);

        /* Draw line segments */
        xcb_poly_segment(conn, window_id, context_id, 2, segments);

        /* Draw rectangle */
        xcb_poly_fill_rectangle(conn, window_id, context_id, 1, &rect);

        /* Draw arc */
        xcb_poly_arc(conn, window_id, context_id, 1, &arc);

        xcb_flush(conn);
        break;

    }
    free(event);
  }
```

After calling the drawing primitives, the event loop calls xcb_flush to force the drawing requests to be sent to the X server.


## Using the Code

The src folder contains the three source files mentioned in this article. The code can be compiled with the following commands:

```console
mkdir build; gcc -o build/simple_window src/simple_window.c -lxcb
mkdir build; gcc -o build/event_window src/event_window.c -lxcb
mkdir build; gcc -o build/graphic_window src/graphic_window.c -lxcb
```

Of course, the applications will only execute properly if XCB has been installed on the system.


## History

4/5/2016 - Initial article submission


## License

This article, along with any associated source code and files, is licensed under [The Code Project Open License (CPOL)](http://www.codeproject.com/info/cpol10.aspx)


## About the Author

[Matt Scarpino](https://www.codeproject.com/Members/mattscar)

United States 









